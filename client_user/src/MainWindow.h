#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QTimer>
#include <QImage>
#include <QPixmap>
#include <QPainter>
#include <QListWidget>
#include <QListWidgetItem>
#include "VideoQuick3DWidget.h"
// VideoGLWidget은 현재 사용하지 않음 (Qt Quick 3D 사용)

// QGst는 현재 사용하지 않음 (OpenCV fallback 사용)
// #ifdef QGST_AVAILABLE
// #include <QGst/Ui/VideoWidget>
// #include <QGst/Pipeline>
// #include <QGst/ElementFactory>
// #include <QGst/Bus>
// #include <QGst/Message>
// #include <QGst/Parse>
// #endif

// OpenCV for video processing
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>

// MediaPipe for face landmark detection (주석 처리 - dlib 사용)
// #include "face_landmarker.h"
// #include "face_landmarker_result.h"
// #include "mediapipe/framework/formats/image.h"
// #include "mediapipe/tasks/cc/vision/core/running_mode.h"

// dlib for face landmark detection
#include <dlib/opencv.h>
#include <dlib/image_processing.h>
#include <dlib/image_processing/frontal_face_detector.h>
#include <dlib/image_processing/shape_predictor.h>
#include <memory>
#include <mutex>
#include <thread>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onConnectClicked();
    void onDisconnectClicked();
    void onWebcamModeClicked();  // 웹캠 모드 버튼
    void onCharacterSelected(QListWidgetItem *item);
    void updateVideoFrame();

private:
    void setupUI();
    void setupVideoWidget();
    void setupCharacterSelector();
    // void initializeMediaPipe();  // MediaPipe 초기화 (주석 처리)
    void initializeDlib();  // dlib 초기화
    void setFPS(int fps);  // FPS 설정 함수
    // void detectFacesWithMediaPipe(cv::Mat &frame);  // MediaPipe 얼굴 탐지 (주석 처리)
    void detectFacesWithDlib(cv::Mat &frame);  // dlib 얼굴 특징점 탐지
    // void renderAvatar(cv::Mat &frame, const mediapipe::tasks::vision::face_landmarker::FaceLandmarkerResult &result);  // MediaPipe 렌더링 (주석 처리)
    QImage matToQImage(const cv::Mat &mat);
    // cv::Mat matToMediaPipeImage(const cv::Mat &mat);  // MediaPipe 변환 (주석 처리)
    
    // UI Components
    QWidget *centralWidget;
    QVBoxLayout *mainLayout;
    QHBoxLayout *topLayout;
    QPushButton *connectButton;
    QPushButton *disconnectButton;
    QLineEdit *serverUrlEdit;
    QLabel *statusLabel;
    
    QWidget *videoContainer;
    VideoQuick3DWidget *videoQuick3DWidget = nullptr;  // Qt Quick 3D 위젯
    QLabel *videoLabel = nullptr;  // 비디오를 표시할 QLabel (fallback)
    
    QWidget *characterSelectorWidget;
    QListWidget *characterList;
    
    // Video streaming
    bool isConnected;
    bool isWebcamMode;  // 웹캠 모드 여부
    QTimer *videoTimer;
    cv::VideoCapture videoCapture;
    
    // FPS 설정 (기본값: 30)
    int targetFPS;  // 목표 FPS
    int dlibProcessInterval;  // dlib 처리 간격 (프레임 수)
    
    // MediaPipe face landmarker (주석 처리)
    // std::unique_ptr<mediapipe::tasks::vision::face_landmarker::FaceLandmarker> faceLandmarker;
    // std::mutex landmarkerMutex;
    // bool mediaPipeInitialized;
    
    // dlib face detector and shape predictor
    dlib::frontal_face_detector faceDetector;
    dlib::shape_predictor shapePredictor;
    std::mutex dlibMutex;
    bool dlibInitialized;
    
    // Frame processing
    int frameCounter;  // 프레임 카운터
    std::mutex resultMutex;
    std::vector<dlib::full_object_detection> lastLandmarkResults;  // 마지막 dlib 결과 저장
    
    // Avatar rendering
    int selectedCharacterIndex;
    
// QGst는 현재 사용하지 않음
// #ifdef QGST_AVAILABLE
//     QGst::Ui::VideoWidget *videoWidget = nullptr;
//     QGst::PipelinePtr pipeline;
// #endif
};

#endif // MAINWINDOW_H
