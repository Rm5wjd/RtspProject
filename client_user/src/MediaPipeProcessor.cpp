// IMPORTANT:
// Qt는 'slots' 매크로를 정의합니다. Python/pybind11 내부에서도 'slots'라는 멤버명이
// 등장하기 때문에, Qt 헤더(<QObject> 등)보다 pybind11/Python 헤더를 먼저 include해야
// 충돌(특히 PyType_Slot *slots) 없이 컴파일됩니다.

// pybind11 embed (Python.h 포함)
#include <pybind11/embed.h>
#include <pybind11/numpy.h>

#include "MediaPipeProcessor.h"
#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include <iostream>
#include <mutex>
#include <cstring>

namespace py = pybind11;

namespace {
// Python 인터프리터는 프로세스당 1개만 존재해야 합니다.
std::once_flag g_pyInitOnce;
std::unique_ptr<py::scoped_interpreter> g_interpreter;

// mediapipe_module 캐시 (인터프리터 생존 동안 유지)
bool g_moduleImported = false;
py::object g_module;
py::object g_init_func;
py::object g_reset_func;
py::object g_process_func;

void ensureInterpreterOnce()
{
    std::call_once(g_pyInitOnce, []() {
        g_interpreter = std::make_unique<py::scoped_interpreter>();
    });
}
} // namespace

MediaPipeProcessor::MediaPipeProcessor(QObject *parent)
    : QObject(parent)
    , m_processTimer(new QTimer(this))
    , m_processInterval(5)  // 기본값: 5프레임마다 처리
    , m_frameCounter(0)
    , m_isProcessing(false)
    , m_running(false)
    , m_pythonReady(false)
    , m_needsReset(true)
{
    m_processTimer->setSingleShot(false);
    m_processTimer->setInterval(100);  // 100ms마다 큐 확인
    connect(m_processTimer, &QTimer::timeout, this, &MediaPipeProcessor::processNextFrame);
}

MediaPipeProcessor::~MediaPipeProcessor()
{
    stop();
}

bool MediaPipeProcessor::start()
{
    if (m_running) {
        // 이미 실행 중이면 그대로 둡니다.
        return true;
    }

    if (!ensurePythonReady()) {
        emit errorOccurred("Failed to initialize embedded Python/MediaPipe");
        return false;
    }

    m_frameCounter = 0;
    m_isProcessing = false;
    m_processTimer->start();
    m_running = true;
    
    qDebug() << "MediaPipe processor started";
    return true;
}

void MediaPipeProcessor::stop()
{
    if (m_processTimer) {
        m_processTimer->stop();
    }

    {
        QMutexLocker lock(&m_queueMutex);
        m_frameQueue.clear();
    }
    
    m_isProcessing = false;
    m_running = false;
    // 다음 start() 때는 무조건 reset()→initialize() 하도록 표시
    m_needsReset = true;
    m_pythonReady = false;
    qDebug() << "MediaPipe processor stopped";
}

void MediaPipeProcessor::processFrame(const cv::Mat &frame)
{
    // ========================================================================
    // 이 함수는 MainWindow에서 읽은 프레임을 받아서 처리합니다.
    // MediaPipeProcessor는 카메라를 직접 열지 않습니다.
    // 카메라 관리는 MainWindow에서만 수행합니다.
    // ========================================================================
    
    if (!m_running) {
        return;
    }
    
    m_frameCounter++;
    
    // 처리 간격에 따라 스킵
    if (m_frameCounter % m_processInterval != 0) {
        return;
    }
    
    // 큐에 추가
    {
        QMutexLocker lock(&m_queueMutex);
        
        // 큐가 너무 크면 오래된 프레임 제거
        while (m_frameQueue.size() >= MAX_QUEUE_SIZE) {
            m_frameQueue.dequeue();
        }
        
        // 최신 프레임만 큐에 추가 (깊은 복사)
        m_frameQueue.enqueue(frame.clone());
    }
}

void MediaPipeProcessor::setProcessInterval(int interval)
{
    if (interval < 1) interval = 1;
    m_processInterval = interval;
    qDebug() << "MediaPipe process interval set to:" << interval;
}

bool MediaPipeProcessor::isRunning() const
{
    return m_running;
}

void MediaPipeProcessor::processNextFrame()
{
    if (m_isProcessing) {
        return;  // 이미 처리 중이면 스킵
    }
    
    processQueuedFrame();
}

void MediaPipeProcessor::processQueuedFrame()
{
    cv::Mat frame;
    
    {
        QMutexLocker lock(&m_queueMutex);
        if (m_frameQueue.isEmpty()) {
            return;
        }
        frame = m_frameQueue.dequeue();
    }
    
    if (frame.empty()) {
        return;
    }
    
    m_isProcessing = true;

    try {
        ensureInterpreterOnce();
        py::gil_scoped_acquire gil;

        // 1) Python/MediaPipe 준비 (필요 시에만 reset/initialize)
        if (!ensurePythonReady()) {
            m_isProcessing = false;
            return;
        }

        // 2) BGR 프레임을 numpy 배열로 넘김 (항상 3채널)
        cv::Mat bgr;
        if (frame.channels() == 3) {
            bgr = frame;
        } else if (frame.channels() == 4) {
            cv::cvtColor(frame, bgr, cv::COLOR_BGRA2BGR);
        } else if (frame.channels() == 1) {
            cv::cvtColor(frame, bgr, cv::COLOR_GRAY2BGR);
        } else {
            m_isProcessing = false;
            return;
        }

        cv::Mat continuous = bgr.isContinuous() ? bgr : bgr.clone();
        if (!continuous.isContinuous()) {
            continuous = bgr.clone();
        }

        const int rows = continuous.rows;
        const int cols = continuous.cols;
        const int ch = continuous.channels();
        if (rows <= 0 || cols <= 0 || ch != 3) {
            m_isProcessing = false;
            return;
        }

        py::array_t<uint8_t> arr({rows, cols, ch});
        std::memcpy(arr.mutable_data(), continuous.data, static_cast<size_t>(rows) * cols * ch);

        py::object result = g_process_func(arr, cols, rows);
        if (result.is_none()) {
            m_isProcessing = false;
            return;
        }

        // 3) dict 파싱
        QVector<FaceData> faces;
        FaceData faceData;

        py::dict d = result.cast<py::dict>();
        if (d.contains("landmarks")) {
            py::list landmarks = d["landmarks"].cast<py::list>();
            for (auto item : landmarks) {
                py::list pt = item.cast<py::list>();
                if (pt.size() >= 3) {
                    FaceData::Landmark lm;
                    lm.x = pt[0].cast<float>();
                    lm.y = pt[1].cast<float>();
                    lm.z = pt[2].cast<float>();
                    faceData.landmarks.append(lm);
                }
            }
        }

        if (d.contains("blendshapes")) {
            py::list blendshapes = d["blendshapes"].cast<py::list>();
            for (auto item : blendshapes) {
                py::dict bs = item.cast<py::dict>();
                FaceData::Blendshape b;
                if (bs.contains("category")) {
                    b.category = QString::fromStdString(bs["category"].cast<std::string>());
                }
                if (bs.contains("score")) {
                    b.score = bs["score"].cast<float>();
                }
                faceData.blendshapes.append(b);
            }
        }

        if (!faceData.landmarks.isEmpty() || !faceData.blendshapes.isEmpty()) {
            faces.append(faceData);
        }

        m_isProcessing = false;
        if (!faces.isEmpty()) {
            emit faceDetected(faces);
        }
        return;
    } catch (const std::exception &e) {
        m_isProcessing = false;
        emit errorOccurred(QString("pybind11 exception: %1").arg(e.what()));
        return;
    }
}

QString MediaPipeProcessor::findPythonModuleDir() const
{
    // client_user/python/mediapipe_module.py가 있는 디렉토리
    QStringList possible = {
        "../python",
        "../../python",
        "python",
        QDir::currentPath() + "/python",
        "/Users/jincheol/Desktop/VEDA/RtspProject/client_user/python"
    };

    for (const QString &dir : possible) {
        QFileInfo fi(dir + "/mediapipe_module.py");
        if (fi.exists() && fi.isFile()) {
            return QFileInfo(dir).absoluteFilePath();
        }
    }
    return QString();
}

QString MediaPipeProcessor::findModelPath() const
{
    QStringList possible = {
        "../thirdparty/mediapipe/models/face_landmarker.task",
        "../../thirdparty/mediapipe/models/face_landmarker.task",
        "thirdparty/mediapipe/models/face_landmarker.task",
        QDir::currentPath() + "/thirdparty/mediapipe/models/face_landmarker.task",
        "/Users/jincheol/Desktop/VEDA/RtspProject/client_user/thirdparty/mediapipe/models/face_landmarker.task"
    };

    for (const QString &p : possible) {
        QFileInfo fi(p);
        if (fi.exists() && fi.isFile()) {
            return fi.absoluteFilePath();
        }
    }
    return QString();
}

bool MediaPipeProcessor::ensurePythonReady()
{
    // interpreter는 한번만 생성
    ensureInterpreterOnce();

    py::gil_scoped_acquire gil;
    py::module_ sys = py::module_::import("sys");
    py::list sysPath = sys.attr("path");

    // -------------------------
    // 1) 최초 1회: 모듈 임포트를 위한 sys.path 세팅 + 함수 캐싱
    // -------------------------
    if (!g_moduleImported) {
        const QString moduleDir = findPythonModuleDir();
        if (moduleDir.isEmpty()) {
            return false;
        }
        sysPath.append(moduleDir.toStdString());

        // venv site-packages를 가능한 경우 우선순위 높게 추가 (mediapipe 설치 위치)
        try {
            py::object vinfo = sys.attr("version_info");
            const int major = vinfo.attr("major").cast<int>();
            const int minor = vinfo.attr("minor").cast<int>();
            const QString ver = QString("%1.%2").arg(major).arg(minor);

            QStringList venvCandidates = {
                "../venv/lib/python" + ver + "/site-packages",
                "../../venv/lib/python" + ver + "/site-packages",
                "venv/lib/python" + ver + "/site-packages",
                QDir::currentPath() + "/venv/lib/python" + ver + "/site-packages",
                "/Users/jincheol/Desktop/VEDA/RtspProject/client_user/venv/lib/python" + ver + "/site-packages"
            };
            for (const QString &vp : venvCandidates) {
                QFileInfo fi(vp);
                if (fi.exists() && fi.isDir()) {
                    sysPath.insert(0, fi.absoluteFilePath().toStdString());
                    break;
                }
            }
        } catch (...) {
            // ignore
        }

        g_module = py::module_::import("mediapipe_module");
        g_init_func = g_module.attr("initialize");
        g_process_func = g_module.attr("process_frame");
        if (py::hasattr(g_module, "reset")) {
            g_reset_func = g_module.attr("reset");
        } else {
            g_reset_func = py::none();
        }

        g_moduleImported = true;
    }

    // -------------------------
    // 2) start/재시작 시에만 reset()→initialize()
    // -------------------------
    if (!m_pythonReady || m_needsReset) {
        if (!g_reset_func.is_none()) {
            g_reset_func();
        }

        const QString modelPath = findModelPath();
        if (modelPath.isEmpty()) {
            return false;
        }
        g_init_func(modelPath.toStdString());

        m_needsReset = false;
        m_pythonReady = true;
    }

    return true;
}
