#include "VideoQuick3DWidget.h"
#include <QFileInfo>
#include <QDir>
#include <QQuickItem>
#include <QQuickView>
#include <QQmlContext>
#include <QQmlEngine>
#include <QImage>
#include <QPixmap>
#include <QPainter>
#include <QQuickImageProvider>
#include <QMutex>
#include <QMutexLocker>
#include <iostream>

// 비디오 이미지 제공자 (QML에서 접근 가능하도록)
class VideoImageProvider : public QQuickImageProvider
{
public:
    VideoImageProvider() : QQuickImageProvider(QQuickImageProvider::Image) {}
    
    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override
    {
        Q_UNUSED(id);
        Q_UNUSED(requestedSize);
        
        QMutexLocker lock(&mutex);
        
        // 이미지가 없거나 비어있으면 기본 검은색 이미지 반환
        if (currentImage.isNull() || currentImage.size().isEmpty()) {
            // 기본 이미지 크기는 요청된 크기 또는 640x480
            int width = requestedSize.width() > 0 ? requestedSize.width() : 640;
            int height = requestedSize.height() > 0 ? requestedSize.height() : 480;
            QImage defaultImage(width, height, QImage::Format_RGB888);
            defaultImage.fill(Qt::black);
            if (size) {
                *size = defaultImage.size();
            }
            return defaultImage;
        }
        
        if (size) {
            *size = currentImage.size();
        }
        // 항상 복사본 반환 (원본이 변경되지 않도록)
        return currentImage.copy();
    }
    
    void setImage(const QImage &image) {
        QMutexLocker lock(&mutex);
        if (!image.isNull() && !image.size().isEmpty()) {
            currentImage = image.copy();  // 복사본 저장
        }
    }
    
private:
    QImage currentImage;
    QMutex mutex;
};

static VideoImageProvider *g_videoImageProvider = nullptr;

VideoQuick3DWidget::VideoQuick3DWidget(QWidget *parent)
    : QQuickWidget(parent)
    , m_avatarIndex(-1)
    , m_faceX(0.0)
    , m_faceY(0.0)
    , m_faceWidth(0.0)
    , m_faceHeight(0.0)
    , landmarksUpdated(false)
{
    // QML 엔진 설정
    QQmlEngine *engine = this->engine();
    QQmlContext *context = engine->rootContext();
    
    // 비디오 이미지 제공자 등록
    if (!g_videoImageProvider) {
        g_videoImageProvider = new VideoImageProvider();
        engine->addImageProvider("video", g_videoImageProvider);
        std::cout << "VideoImageProvider registered: image://video/frame" << std::endl;
    } else {
        std::cout << "VideoImageProvider already exists" << std::endl;
    }
    
    // C++ 객체를 QML에 노출
    context->setContextProperty("videoWidget", this);
    
    // QML 파일 로드
    QString qmlPath = QString("%1/../qml/VideoScene.qml").arg(QDir::currentPath());
    QFileInfo qmlFile(qmlPath);
    if (!qmlFile.exists()) {
        qmlPath = QString("%1/qml/VideoScene.qml").arg(QDir::currentPath());
        qmlFile.setFile(qmlPath);
    }
    if (!qmlFile.exists()) {
        qmlPath = QString("/Users/jincheol/Desktop/VEDA/RtspProject/client_user/qml/VideoScene.qml");
    }
    
    if (QFileInfo::exists(qmlPath)) {
        setSource(QUrl::fromLocalFile(qmlPath));
        std::cout << "QML file loaded: " << qmlPath.toStdString() << std::endl;
    } else {
        std::cerr << "QML file not found: " << qmlPath.toStdString() << std::endl;
    }
    
    setResizeMode(QQuickWidget::SizeRootObjectToView);
}

VideoQuick3DWidget::~VideoQuick3DWidget()
{
}

void VideoQuick3DWidget::updateFrame(const cv::Mat &frame)
{
    if (frame.empty() || frame.cols <= 0 || frame.rows <= 0) {
        // 프레임이 비어있으면 조용히 리턴 (너무 많은 로그 방지)
        return;
    }
    
    try {
        // OpenCV Mat을 QImage로 변환
        cv::Mat rgbFrame;
        cv::cvtColor(frame, rgbFrame, cv::COLOR_BGR2RGB);
        
        // 연속된 메모리로 변환 (필요한 경우)
        cv::Mat continuousFrame;
        if (!rgbFrame.isContinuous()) {
            rgbFrame.copyTo(continuousFrame);
        } else {
            continuousFrame = rgbFrame;
        }
        
        // QImage 생성 (데이터 복사 - 원본 Mat이 사라질 수 있으므로)
        // QImage는 데이터를 참조하므로, 반드시 깊은 복사 필요
        QImage qimg(continuousFrame.data, continuousFrame.cols, continuousFrame.rows, 
                    continuousFrame.step, QImage::Format_RGB888);
        
        if (qimg.isNull()) {
            return;
        }
        
        // QImage 깊은 복사 (원본 데이터가 사라지지 않도록)
        // copy()는 깊은 복사를 수행하지만, detach()를 명시적으로 호출하여 안전하게 처리
        m_videoImage = qimg.copy();
        if (m_videoImage.isNull()) {
            return;
        }
        m_videoImage.detach();  // 데이터 분리 보장
        
        // ImageProvider에 이미지 업데이트 (유효한 이미지일 때만)
        if (g_videoImageProvider && !m_videoImage.isNull() && !m_videoImage.size().isEmpty()) {
            g_videoImageProvider->setImage(m_videoImage);
        }
        
        // QML에 변경 알림
        emit videoImageChanged();
    } catch (const std::exception& e) {
        std::cerr << "VideoQuick3DWidget::updateFrame exception: " << e.what() << std::endl;
    }
}

// MediaPipe 랜드마크 업데이트 (주석 처리)
/*
void VideoQuick3DWidget::updateLandmarks(const mediapipe::tasks::vision::face_landmarker::FaceLandmarkerResult &landmarks)
{
    QMutexLocker lock(&landmarkMutex);
    currentLandmarks = landmarks;
    landmarksUpdated = true;
    
    calculateFacePosition();
}
*/

// dlib 랜드마크 업데이트
void VideoQuick3DWidget::updateLandmarks(const std::vector<dlib::full_object_detection> &landmarks)
{
    QMutexLocker lock(&landmarkMutex);
    currentLandmarks = landmarks;
    landmarksUpdated = true;
    
    calculateFacePosition();
}

void VideoQuick3DWidget::setAvatarIndex(int index)
{
    if (m_avatarIndex != index) {
        m_avatarIndex = index;
        if (rootObject()) {
            rootObject()->setProperty("avatarIndex", m_avatarIndex);
        }
        emit avatarIndexChanged();
    }
}

void VideoQuick3DWidget::setGlbModelPath(const QString &path)
{
    QString absolutePath = QFileInfo(path).absoluteFilePath();
    if (m_glbModelPath != absolutePath) {
        m_glbModelPath = absolutePath;
        if (rootObject()) {
            rootObject()->setProperty("glbModelPath", m_glbModelPath);
        }
        emit glbModelPathChanged();
    }
}

void VideoQuick3DWidget::setFaceData(double x, double y, double width, double height)
{
    if (m_faceX != x || m_faceY != y || m_faceWidth != width || m_faceHeight != height) {
        m_faceX = x;
        m_faceY = y;
        m_faceWidth = width;
        m_faceHeight = height;
        emit faceDataChanged();
    }
}
void VideoQuick3DWidget::setBlendshapes(const QList<qreal> &values)
{
    if (m_blendshapes != values) {
        m_blendshapes = values;
        emit blendshapesChanged();
    }
}

void VideoQuick3DWidget::calculateFacePosition()
{
    QMutexLocker lock(&landmarkMutex);
    
    // dlib 결과 처리
    if (currentLandmarks.empty()) {
        m_faceX = 0.0;
        m_faceY = 0.0;
        m_faceWidth = 0.0;
        m_faceHeight = 0.0;
        emit faceDataChanged();
        return;
    }
    
    // 첫 번째 얼굴의 랜드마크 사용
    const auto& face = currentLandmarks[0];
    if (face.num_parts() == 0) {
        return;
    }
    
    // 얼굴 영역 계산 (dlib 좌표는 픽셀 단위이므로 정규화 필요)
    // dlib::full_object_detection의 rect를 사용하거나 랜드마크로 계산
    dlib::rectangle rect = face.get_rect();
    
    // 정규화된 좌표 (0-1 범위로 변환하려면 프레임 크기가 필요하지만, 
    // 여기서는 rect의 좌표를 사용)
    // 실제로는 프레임 크기를 전달받아야 하지만, 일단 rect 사용
    m_faceX = (rect.left() + rect.right()) / 2.0;
    m_faceY = (rect.top() + rect.bottom()) / 2.0;
    m_faceWidth = rect.width();
    m_faceHeight = rect.height();
    
    emit faceDataChanged();
}

void VideoQuick3DWidget::updateFaceData()
{
    if (landmarksUpdated) {
        calculateFacePosition();
        landmarksUpdated = false;
    }
}
