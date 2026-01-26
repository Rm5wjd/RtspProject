#include "MainWindow.h"
#include "MediaPipeProcessor.h"
#include <QApplication>
#include <QMessageBox>
#include <QDir>
#include <iostream>
#include <fstream>
#include <algorithm>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , isConnected(false)
    , isWebcamMode(false)  // 웹캠 모드 초기값
    , selectedCharacterIndex(-1)
    , targetFPS(30)  // 기본 FPS: 30
    , mediaPipeProcessor(nullptr)
    , hasFaceData(false)
{
    setupUI();
    setupVideoWidget();
    setupCharacterSelector();
    
    // MediaPipe 프로세서 초기화
    mediaPipeProcessor = new MediaPipeProcessor(this);
    mediaPipeProcessor->setProcessInterval(5);  // 5프레임마다 처리
    connect(mediaPipeProcessor, &MediaPipeProcessor::faceDetected,
            this, &MainWindow::onFaceDetected);
    connect(mediaPipeProcessor, &MediaPipeProcessor::errorOccurred,
            this, [this](const QString &error) {
                std::cerr << "MediaPipe error: " << error.toStdString() << std::endl;
            });
    
    // 비디오 프레임 업데이트 타이머 설정
    videoTimer = new QTimer(this);
    setFPS(targetFPS);  // FPS 설정
    
    connect(videoTimer, &QTimer::timeout, this, &MainWindow::updateVideoFrame);
    
    // 기본적으로 웹캠 모드로 시작 (UI가 완전히 로드된 후)
    QTimer::singleShot(1000, this, [this]() {
        onWebcamModeClicked();
    });
}

MainWindow::~MainWindow()
{
    if (videoCapture.isOpened()) {
        videoCapture.release();
    }
    
// QGst는 현재 사용하지 않음
// #ifdef QGST_AVAILABLE
//     if (pipeline) {
//         pipeline->setState(QGst::StateNull);
//         pipeline.clear();
//     }
// #endif
}

void MainWindow::setupUI()
{
    centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    mainLayout = new QVBoxLayout(centralWidget);
    
    // 상단 컨트롤 패널
    QWidget *controlPanel = new QWidget(this);
    QHBoxLayout *controlLayout = new QHBoxLayout(controlPanel);
    
    serverUrlEdit = new QLineEdit(this);
    serverUrlEdit->setPlaceholderText("UDP 포트 번호 (기본: 5000) 또는 rtsp://서버주소:포트/스트림");
    serverUrlEdit->setText("5000");
    
    connectButton = new QPushButton("서버 연결", this);
    disconnectButton = new QPushButton("연결 해제", this);
    disconnectButton->setEnabled(false);
    
    QPushButton *webcamButton = new QPushButton("웹캠 모드", this);
    webcamButton->setStyleSheet("background-color: #4CAF50; color: white; font-weight: bold;");
    
    statusLabel = new QLabel("연결 안 됨", this);
    statusLabel->setStyleSheet("color: red; font-weight: bold;");
    
    controlLayout->addWidget(new QLabel("서버 주소:", this));
    controlLayout->addWidget(serverUrlEdit);
    controlLayout->addWidget(connectButton);
    controlLayout->addWidget(disconnectButton);
    controlLayout->addWidget(webcamButton);
    controlLayout->addWidget(statusLabel);
    controlLayout->addStretch();
    
    // 웹캠 모드 버튼 연결
    connect(webcamButton, &QPushButton::clicked, this, &MainWindow::onWebcamModeClicked);
    
    mainLayout->addWidget(controlPanel);
    
    // 비디오 컨테이너 (Qt Quick 3D 사용)
    videoContainer = new QWidget(this);
    videoContainer->setMinimumSize(640, 480);
    videoContainer->setStyleSheet("background-color: black;");
    QVBoxLayout *videoLayout = new QVBoxLayout(videoContainer);
    videoLayout->setContentsMargins(0, 0, 0, 0);
    
    // Qt Quick 3D 위젯 생성 (권장)
    videoQuick3DWidget = new VideoQuick3DWidget(this);
    videoQuick3DWidget->setMinimumSize(640, 480);
    videoLayout->addWidget(videoQuick3DWidget);
    std::cout << "Qt Quick 3D widget created" << std::endl;
    
    // Fallback QLabel
    videoLabel = new QLabel(this);
    videoLabel->setAlignment(Qt::AlignCenter);
    videoLabel->setText("비디오가 표시됩니다");
    videoLabel->setStyleSheet("color: white; font-size: 16px;");
    videoLabel->hide();
    
    mainLayout->addWidget(videoContainer, 1);
    
    // 연결 버튼 시그널
    connect(connectButton, &QPushButton::clicked, this, &MainWindow::onConnectClicked);
    connect(disconnectButton, &QPushButton::clicked, this, &MainWindow::onDisconnectClicked);
    
    setWindowTitle("RTSP 클라이언트 - 아바타 렌더링");
    resize(800, 700);
}

void MainWindow::setupVideoWidget()
{
// QGst는 현재 사용하지 않음
// #ifdef QGST_AVAILABLE
//     // QGst VideoWidget 사용
//     videoWidget = new QGst::Ui::VideoWidget(videoContainer);
//     QVBoxLayout *videoLayout = qobject_cast<QVBoxLayout*>(videoContainer->layout());
//     if (videoLayout) {
//         videoLayout->removeWidget(videoLabel);
//         videoLabel->hide();
//         videoLayout->addWidget(videoWidget);
//     }
// #endif
}

// MediaPipe 초기화 (주석 처리)
/*
void MainWindow::initializeMediaPipe()
{
    try {
        // MediaPipe 모델 경로 찾기
        std::vector<std::string> possiblePaths = {
            "../thirdparty/mediapipe/models/face_landmarker.task",
            "../../thirdparty/mediapipe/models/face_landmarker.task",
            "thirdparty/mediapipe/models/face_landmarker.task",
            "/Users/jincheol/Desktop/VEDA/RtspProject/client_user/thirdparty/mediapipe/models/face_landmarker.task"
        };
        
        std::string modelPath;
        bool modelFound = false;
        for (const auto& path : possiblePaths) {
            std::ifstream file(path);
            if (file.good()) {
                modelPath = path;
                modelFound = true;
                std::cout << "Found MediaPipe model at: " << path << std::endl;
                break;
            }
        }
        
        if (!modelFound) {
            QMessageBox::warning(this, "경고", "MediaPipe 모델 파일을 찾을 수 없습니다.\n"
                                               "face_landmarker.task 파일을 확인하세요.");
            return;
        }
        
        // MediaPipe FaceLandmarker 옵션 설정
        auto options = std::make_unique<mediapipe::tasks::vision::face_landmarker::FaceLandmarkerOptions>();
        options->base_options.model_asset_path = modelPath;
        options->running_mode = mediapipe::tasks::vision::core::RunningMode::VIDEO;
        options->num_faces = 1;
        options->min_face_detection_confidence = 0.5f;
        options->min_face_presence_confidence = 0.5f;
        options->min_tracking_confidence = 0.5f;
        
        // FaceLandmarker 생성
        auto landmarkerOr = mediapipe::tasks::vision::face_landmarker::FaceLandmarker::Create(std::move(options));
        
        if (landmarkerOr.ok()) {
            faceLandmarker = std::move(*landmarkerOr);
            mediaPipeInitialized = true;
            std::cout << "MediaPipe FaceLandmarker initialized successfully" << std::endl;
        } else {
            QMessageBox::critical(this, "오류", QString("MediaPipe 초기화 실패: %1")
                                 .arg(QString::fromStdString(landmarkerOr.status().ToString())));
        }
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "오류", QString("MediaPipe 초기화 중 예외 발생: %1").arg(e.what()));
    }
}
*/


void MainWindow::setFPS(int fps)
{
    if (fps < 1) fps = 1;
    if (fps > 120) fps = 120;  // 최대 120FPS 제한
    
    targetFPS = fps;
    
    // 타이머 간격 계산 (밀리초)
    int intervalMs = 1000 / fps;
    videoTimer->setInterval(intervalMs);
    
    std::cout << "FPS 설정: " << targetFPS << " (간격: " << intervalMs << "ms)" << std::endl;
}

void MainWindow::setupCharacterSelector()
{
    // 캐릭터 선택 위젯
    characterSelectorWidget = new QWidget(this);
    QHBoxLayout *charLayout = new QHBoxLayout(characterSelectorWidget);
    
    charLayout->addWidget(new QLabel("캐릭터 선택:", this));
    
    characterList = new QListWidget(this);
    characterList->setViewMode(QListWidget::IconMode);
    characterList->setIconSize(QSize(80, 80));
    characterList->setResizeMode(QListWidget::Adjust);
    characterList->setFlow(QListWidget::LeftToRight);
    
    // 캐릭터 목록 추가 (임시로 텍스트로 표시, 나중에 이미지로 변경 가능)
    QStringList characters = {"캐릭터 1", "캐릭터 2", "캐릭터 3", "캐릭터 4"};
    for (int i = 0; i < characters.size(); ++i) {
        QListWidgetItem *item = new QListWidgetItem(characters[i], characterList);
        item->setTextAlignment(Qt::AlignCenter);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Unchecked);
        // 나중에 아이콘 추가: item->setIcon(QIcon("path/to/character.png"));
    }
    
    charLayout->addWidget(characterList);
    mainLayout->addWidget(characterSelectorWidget);
    
    connect(characterList, &QListWidget::itemClicked, this, &MainWindow::onCharacterSelected);
}

void MainWindow::onConnectClicked()
{
    if (isConnected) {
        return;
    }
    
    QString url = serverUrlEdit->text();
    if (url.isEmpty()) {
        QMessageBox::warning(this, "경고", "서버 주소를 입력하세요.");
        return;
    }
    
    // RTSP/UDP 연결 시도
    // QGst는 현재 사용하지 않음, OpenCV로 UDP 직접 수신
    // #ifdef QGST_AVAILABLE
    // ... QGst 코드 주석 처리 ...
    // #else
    // QGst가 없는 경우 OpenCV로 UDP 직접 수신 (서버가 UDP로 전송하는 경우)
    try {
        // URL에서 포트 번호 추출 (기본값: 5000)
        int port = 5000;
        if (url.contains(":")) {
            QString portStr = url.split(":").last();
            bool ok;
            int parsedPort = portStr.toInt(&ok);
            if (ok) {
                port = parsedPort;
            }
        }
        
        // GStreamer 파이프라인으로 UDP 수신
        QString gstPipeline = QString("udpsrc port=%1 ! application/x-rtp, encoding-name=H264, payload=96 ! rtph264depay ! avdec_h264 ! videoconvert ! appsink")
                              .arg(port);
        
        videoCapture.open(gstPipeline.toStdString(), cv::CAP_GSTREAMER);
        
        if (!videoCapture.isOpened()) {
            // RTSP로 시도
            videoCapture.open(url.toStdString());
        }
        
        if (videoCapture.isOpened()) {
            isConnected = true;
            statusLabel->setText("연결됨");
            statusLabel->setStyleSheet("color: green; font-weight: bold;");
            connectButton->setEnabled(false);
            disconnectButton->setEnabled(true);
            serverUrlEdit->setEnabled(false);
            
            // 비디오 타이머 시작
            videoTimer->start(33); // ~30 FPS
        } else {
            QMessageBox::critical(this, "오류", QString("비디오 스트림을 열 수 없습니다.\n"
                                                        "서버 주소와 포트를 확인하세요.\n"
                                                        "UDP 포트: %1").arg(port));
        }
    } catch (const cv::Exception& e) {
        QMessageBox::critical(this, "오류", QString("OpenCV 오류: %1").arg(e.what()));
    }
}

void MainWindow::onDisconnectClicked()
{
    if (!isConnected) {
        return;
    }
    
// QGst는 현재 사용하지 않음
// #ifdef QGST_AVAILABLE
//     if (pipeline) {
//         pipeline->setState(QGst::StateNull);
//         pipeline.clear();
//     }
// #endif
    if (videoCapture.isOpened()) {
        videoCapture.release();
    }
    
    videoTimer->stop();
    isConnected = false;
    isWebcamMode = false;  // 웹캠 모드 해제
    
    // MediaPipe 프로세서 중지
    if (mediaPipeProcessor && mediaPipeProcessor->isRunning()) {
        mediaPipeProcessor->stop();
    }
    
    statusLabel->setText("연결 안 됨");
    statusLabel->setStyleSheet("color: red; font-weight: bold;");
    connectButton->setEnabled(true);
    disconnectButton->setEnabled(false);
    serverUrlEdit->setEnabled(true);
    
    videoLabel->clear();
    videoLabel->setText("비디오가 표시됩니다");
}

void MainWindow::onWebcamModeClicked()
{
    if (isConnected && isWebcamMode) {
        // 이미 웹캠 모드인 경우 해제
        onDisconnectClicked();
        return;
    }
    
    // 기존 연결이 있으면 해제
    if (isConnected) {
        onDisconnectClicked();
    }
    
    // 웹캠 열기
    try {
        // 기본 웹캠 인덱스 0 사용
        videoCapture.open(0);
        
        if (!videoCapture.isOpened()) {
            QMessageBox::critical(this, "오류", "웹캠을 열 수 없습니다.\n"
                                                "다른 프로그램에서 웹캠을 사용 중이거나\n"
                                                "웹캠이 연결되어 있지 않습니다.");
            return;
        }
        
        // 웹캠 해상도 설정 (선택사항)
        videoCapture.set(cv::CAP_PROP_FRAME_WIDTH, 640);
        videoCapture.set(cv::CAP_PROP_FRAME_HEIGHT, 480);
        videoCapture.set(cv::CAP_PROP_FPS, 30);
        
        isConnected = true;
        isWebcamMode = true;  // 웹캠 모드 활성화
        
        statusLabel->setText("웹캠 모드");
        statusLabel->setStyleSheet("color: green; font-weight: bold;");
        connectButton->setEnabled(false);
        disconnectButton->setEnabled(true);
        serverUrlEdit->setEnabled(false);
        
        // MediaPipe 프로세서 시작
        if (mediaPipeProcessor && !mediaPipeProcessor->isRunning()) {
            if (!mediaPipeProcessor->start()) {
                std::cerr << "Failed to start MediaPipe processor" << std::endl;
            }
        }
        
        // 비디오 타이머 시작
        videoTimer->start(33); // ~30 FPS
        
        std::cout << "Webcam mode activated - FPS: " << targetFPS << std::endl;
        std::cout << "Webcam resolution: " << videoCapture.get(cv::CAP_PROP_FRAME_WIDTH) 
                  << "x" << videoCapture.get(cv::CAP_PROP_FRAME_HEIGHT) << std::endl;
    } catch (const cv::Exception& e) {
        QMessageBox::critical(this, "오류", QString("웹캠 오류: %1").arg(e.what()));
    }
}

void MainWindow::onCharacterSelected(QListWidgetItem *item)
{
    // 다른 항목들의 체크 해제
    for (int i = 0; i < characterList->count(); ++i) {
        QListWidgetItem *otherItem = characterList->item(i);
        if (otherItem != item) {
            otherItem->setCheckState(Qt::Unchecked);
        }
    }
    
    // 선택된 항목 체크
    item->setCheckState(Qt::Checked);
    selectedCharacterIndex = characterList->row(item);
    
    std::cout << "캐릭터 " << selectedCharacterIndex << " 선택됨" << std::endl;
    
    // GLB 파일 로드 시도 (캐릭터별)
    QStringList glbPaths = {
        QString("../resource/character%1.glb").arg(selectedCharacterIndex),
        QString("../../resource/character%1.glb").arg(selectedCharacterIndex),
        QString("resource/character%1.glb").arg(selectedCharacterIndex),
        QString("/Users/jincheol/Desktop/VEDA/RtspProject/resource/character%1.glb").arg(selectedCharacterIndex)
    };
    
    bool loaded = false;
    QString glbPath;
    
    for (const auto& path : glbPaths) {
        QFileInfo fileInfo(path);
        if (fileInfo.exists()) {
            glbPath = QFileInfo(path).absoluteFilePath();
            loaded = true;
            std::cout << "GLB 파일 찾음: " << glbPath.toStdString() << std::endl;
            break;
        }
    }
    
    if (videoQuick3DWidget) {
        // Qt Quick 3D는 QML에서 직접 GLB 로드
        if (loaded) {
            videoQuick3DWidget->setGlbModelPath(glbPath);
        } else {
            videoQuick3DWidget->setGlbModelPath("");  // 기본 큐브 사용
            std::cout << "GLB 파일을 찾을 수 없습니다. 기본 큐브를 사용합니다." << std::endl;
        }
    }
}

void MainWindow::updateVideoFrame()
{
    if (!isConnected) {
        return;
    }
    
    cv::Mat frame;
    
    // 웹캠 모드 또는 서버 연결 모드 모두 OpenCV VideoCapture 사용
    if (videoCapture.isOpened()) {
        bool success = videoCapture.read(frame);  // read() 사용 (더 안정적)
        if (!success || frame.empty()) {
            // 프레임 읽기 실패 시 재시도하지 않고 그냥 리턴
            return;
        }
    } else {
        return;
    }
    
    // 프레임 유효성 확인
    if (frame.cols <= 0 || frame.rows <= 0) {
        return;
    }
    
    // MediaPipe 프로세서에 프레임 전달 (비동기 처리)
    if (mediaPipeProcessor && mediaPipeProcessor->isRunning()) {
        mediaPipeProcessor->processFrame(frame);
    }
    
    // 얼굴 데이터가 있으면 프레임에 그리기
    if (hasFaceData && !currentFaceData.landmarks.isEmpty()) {
        // 랜드마크 그리기
        for (const auto &landmark : currentFaceData.landmarks) {
            int x = static_cast<int>(landmark.x * frame.cols);
            int y = static_cast<int>(landmark.y * frame.rows);
            cv::circle(frame, cv::Point(x, y), 2, cv::Scalar(0, 255, 0), -1);
        }
        
        // 얼굴 영역 박스 그리기
        float minX = 1.0f, minY = 1.0f, maxX = 0.0f, maxY = 0.0f;
        for (const auto &landmark : currentFaceData.landmarks) {
            minX = std::min(minX, landmark.x);
            minY = std::min(minY, landmark.y);
            maxX = std::max(maxX, landmark.x);
            maxY = std::max(maxY, landmark.y);
        }
        
        cv::Point pt1(static_cast<int>(minX * frame.cols), static_cast<int>(minY * frame.rows));
        cv::Point pt2(static_cast<int>(maxX * frame.cols), static_cast<int>(maxY * frame.rows));
        cv::rectangle(frame, pt1, pt2, cv::Scalar(0, 255, 255), 2);
    }
    
    // 영상은 실시간으로 바로 표시
    if (videoQuick3DWidget) {
        // 프레임이 유효한지 확인
        if (!frame.empty() && frame.cols > 0 && frame.rows > 0) {
            videoQuick3DWidget->updateFrame(frame);
        }
    } else if (videoLabel) {
        // Fallback: QLabel에 표시
        QImage qimg = matToQImage(frame);
        if (!qimg.isNull()) {
            QPixmap pixmap = QPixmap::fromImage(qimg);
            
            QSize labelSize = videoLabel->size();
            if (labelSize.width() > 0 && labelSize.height() > 0) {
                pixmap = pixmap.scaled(labelSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            }
            
            videoLabel->setPixmap(pixmap);
        }
    }
}

void MainWindow::onFaceDetected(const QVector<MediaPipeProcessor::FaceData> &faces)
{
    if (faces.isEmpty()) {
        // 얼굴이 없으면 초기화
        hasFaceData = false;
        if (videoQuick3DWidget) {
            videoQuick3DWidget->setFaceData(0.0, 0.0, 0.0, 0.0);
        }
        return;
    }
    
    // 첫 번째 얼굴 데이터 사용
    currentFaceData = faces[0];
    hasFaceData = true;
    
    // Blendshape 데이터 출력 (예시)
    std::cout << "=== Face Detected ===" << std::endl;
    std::cout << "Landmarks: " << currentFaceData.landmarks.size() << " points" << std::endl;
    std::cout << "Blendshapes: " << currentFaceData.blendshapes.size() << " values" << std::endl;
    
    // 주요 Blendshape 출력 (예시)
    for (const auto &blendshape : currentFaceData.blendshapes) {
        if (blendshape.score > 0.1f) {  // 임계값 이상만 출력
            std::cout << "  " << blendshape.category.toStdString() 
                      << ": " << blendshape.score << std::endl;
        }
    }
    
    // 랜드마크에서 얼굴 영역 계산 (정규화된 좌표 0-1)
    if (!currentFaceData.landmarks.isEmpty()) {
        float minX = 1.0f, minY = 1.0f, maxX = 0.0f, maxY = 0.0f;
        
        for (const auto &landmark : currentFaceData.landmarks) {
            minX = std::min(minX, landmark.x);
            minY = std::min(minY, landmark.y);
            maxX = std::max(maxX, landmark.x);
            maxY = std::max(maxY, landmark.y);
        }
        
        // 얼굴 중심 및 크기 계산 (정규화된 좌표)
        double faceX = (minX + maxX) / 2.0;
        double faceY = (minY + maxY) / 2.0;
        double faceWidth = maxX - minX;
        double faceHeight = maxY - minY;
        
        // VideoQuick3DWidget에 얼굴 데이터 전달
        if (videoQuick3DWidget) {
            videoQuick3DWidget->setFaceData(faceX, faceY, faceWidth, faceHeight);
        }
    }
}


// MediaPipe 렌더링 함수 (주석 처리)
/*
void MainWindow::renderAvatar(cv::Mat &frame, const mediapipe::tasks::vision::face_landmarker::FaceLandmarkerResult &result)
{
    if (selectedCharacterIndex < 0 || result.face_landmarks.empty()) {
        return;
    }
    
    // 첫 번째 얼굴의 랜드마크 사용
    const auto& landmarks = result.face_landmarks[0];
    if (landmarks.empty()) {
        return;
    }
    
    // 얼굴 영역 계산 (랜드마크의 최소/최대 좌표)
    float minX = 1.0f, minY = 1.0f, maxX = 0.0f, maxY = 0.0f;
    for (const auto& landmark : landmarks) {
        minX = std::min(minX, landmark.x);
        minY = std::min(minY, landmark.y);
        maxX = std::max(maxX, landmark.x);
        maxY = std::max(maxY, landmark.y);
    }
    
    int frameWidth = frame.cols;
    int frameHeight = frame.rows;
    
    int faceX = static_cast<int>(minX * frameWidth);
    int faceY = static_cast<int>(minY * frameHeight);
    int faceWidth = static_cast<int>((maxX - minX) * frameWidth);
    int faceHeight = static_cast<int>((maxY - minY) * frameHeight);
    
    cv::Point center(faceX + faceWidth / 2, faceY + faceHeight / 2);
    int radius = std::min(faceWidth, faceHeight) / 2;
    
    // 캐릭터별 색상
    cv::Scalar colors[] = {
        cv::Scalar(255, 0, 0),    // 캐릭터 1: 빨강
        cv::Scalar(0, 255, 0),    // 캐릭터 2: 초록
        cv::Scalar(0, 0, 255),    // 캐릭터 3: 파랑
        cv::Scalar(255, 255, 0)   // 캐릭터 4: 노랑
    };
    
    if (selectedCharacterIndex < 4) {
        // 원형 아바타 렌더링
        cv::circle(frame, center, radius, colors[selectedCharacterIndex], -1);
        cv::circle(frame, center, radius, cv::Scalar(255, 255, 255), 2);
        
        // 눈 그리기 (랜드마크 인덱스 사용 가능)
        cv::Point leftEye(center.x - radius/3, center.y - radius/4);
        cv::Point rightEye(center.x + radius/3, center.y - radius/4);
        cv::circle(frame, leftEye, radius/6, cv::Scalar(255, 255, 255), -1);
        cv::circle(frame, rightEye, radius/6, cv::Scalar(255, 255, 255), -1);
        
        // 입 그리기
        cv::ellipse(frame, cv::Point(center.x, center.y + radius/3), 
                   cv::Size(radius/3, radius/4), 0, 0, 180, cv::Scalar(255, 255, 255), 2);
        
        // 랜드마크 점 그리기 (디버그용, 선택사항)
        // for (const auto& landmark : landmarks) {
        //     int x = static_cast<int>(landmark.x * frameWidth);
        //     int y = static_cast<int>(landmark.y * frameHeight);
        //     cv::circle(frame, cv::Point(x, y), 2, cv::Scalar(0, 255, 255), -1);
        // }
    }
    
    // 나중에 여기에 3D 모델이나 이미지 기반 아바타 렌더링 추가 가능
    // 랜드마크 좌표를 사용하여 더 정확한 위치에 아바타 배치 가능
}
*/

QImage MainWindow::matToQImage(const cv::Mat &mat)
{
    if (mat.empty()) {
        return QImage();
    }
    
    cv::Mat rgbMat;
    if (mat.channels() == 3) {
        cv::cvtColor(mat, rgbMat, cv::COLOR_BGR2RGB);
    } else if (mat.channels() == 1) {
        cv::cvtColor(mat, rgbMat, cv::COLOR_GRAY2RGB);
    } else {
        rgbMat = mat.clone();
    }
    
    return QImage(rgbMat.data, rgbMat.cols, rgbMat.rows, 
                 rgbMat.step, QImage::Format_RGB888).copy();
}

// MediaPipe 이미지 변환 함수 (주석 처리)
/*
cv::Mat MainWindow::matToMediaPipeImage(const cv::Mat &mat)
{
    cv::Mat rgbMat;
    if (mat.channels() == 3) {
        cv::cvtColor(mat, rgbMat, cv::COLOR_BGR2RGB);
    } else if (mat.channels() == 1) {
        cv::cvtColor(mat, rgbMat, cv::COLOR_GRAY2RGB);
    } else {
        rgbMat = mat.clone();
    }
    return rgbMat;
}
*/
