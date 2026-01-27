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
#include "MediaPipeProcessor.h"
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
    void onFaceDetected(const QVector<MediaPipeProcessor::FaceData> &faces);

private:
    void setupUI();
    void setupVideoWidget();
    void setupCharacterSelector();
    void setFPS(int fps);  // FPS 설정 함수
    QImage matToQImage(const cv::Mat &mat);
    
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
    
    // Avatar rendering
    int selectedCharacterIndex;
    
    // MediaPipe Python 사이드카 프로세서
    MediaPipeProcessor *mediaPipeProcessor;
    
// QGst는 현재 사용하지 않음
// #ifdef QGST_AVAILABLE
//     QGst::Ui::VideoWidget *videoWidget = nullptr;
//     QGst::PipelinePtr pipeline;
// #endif
};

#endif // MAINWINDOW_H
