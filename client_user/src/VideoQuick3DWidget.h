#ifndef VIDEOQUICK3DWIDGET_H
#define VIDEOQUICK3DWIDGET_H

#include <QtQuickWidgets/QQuickWidget>
#include <QQmlContext>
#include <QQuickItem>
#include <QTimer>
#include <QMutex>
#include <opencv2/opencv.hpp>
// MediaPipe 관련 (주석 처리)
// #include "face_landmarker_result.h"
#include <dlib/image_processing.h>

class VideoQuick3DWidget : public QQuickWidget
{
    Q_OBJECT
    
    Q_PROPERTY(int avatarIndex READ avatarIndex WRITE setAvatarIndex NOTIFY avatarIndexChanged)
    Q_PROPERTY(QString glbModelPath READ glbModelPath WRITE setGlbModelPath NOTIFY glbModelPathChanged)
    Q_PROPERTY(QImage videoImage READ videoImage NOTIFY videoImageChanged)
    Q_PROPERTY(double faceX READ faceX NOTIFY faceDataChanged)
    Q_PROPERTY(double faceY READ faceY NOTIFY faceDataChanged)
    Q_PROPERTY(double faceWidth READ faceWidth NOTIFY faceDataChanged)
    Q_PROPERTY(double faceHeight READ faceHeight NOTIFY faceDataChanged)

public:
    explicit VideoQuick3DWidget(QWidget *parent = nullptr);
    ~VideoQuick3DWidget();
    
    void updateFrame(const cv::Mat &frame);
    // void updateLandmarks(const mediapipe::tasks::vision::face_landmarker::FaceLandmarkerResult &landmarks);  // MediaPipe (주석 처리)
    void updateLandmarks(const std::vector<dlib::full_object_detection> &landmarks);  // dlib
    void setAvatarIndex(int index);
    void setGlbModelPath(const QString &path);
    void setFaceData(double x, double y, double width, double height);
    
    // Property getters
    int avatarIndex() const { return m_avatarIndex; }
    QString glbModelPath() const { return m_glbModelPath; }
    QImage videoImage() const { return m_videoImage; }
    double faceX() const { return m_faceX; }
    double faceY() const { return m_faceY; }
    double faceWidth() const { return m_faceWidth; }
    double faceHeight() const { return m_faceHeight; }

signals:
    void avatarIndexChanged();
    void glbModelPathChanged();
    void videoImageChanged();
    void faceDataChanged();

private slots:
    void updateFaceData();

private:
    void calculateFacePosition();
    
    int m_avatarIndex;
    QString m_glbModelPath;
    QImage m_videoImage;  // 비디오 프레임 이미지
    
    QMutex landmarkMutex;
    // mediapipe::tasks::vision::face_landmarker::FaceLandmarkerResult currentLandmarks;  // MediaPipe (주석 처리)
    std::vector<dlib::full_object_detection> currentLandmarks;  // dlib
    bool landmarksUpdated;
    
    // Face position data (normalized 0-1)
    double m_faceX;
    double m_faceY;
    double m_faceWidth;
    double m_faceHeight;
};

#endif // VIDEOQUICK3DWIDGET_H
