#ifndef VIDEOGLWIDGET_H
#define VIDEOGLWIDGET_H

#include <QtOpenGL/QOpenGLWidget>
#include <QtOpenGL/QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QMatrix4x4>
#include <QTimer>
#include <QMutex>
#include <QString>
#include <vector>
#include <opencv2/opencv.hpp>
#include "face_landmarker_result.h"

// GLB 모델 데이터 구조
struct GLBMesh {
    QOpenGLVertexArrayObject *vao = nullptr;
    QOpenGLBuffer *vbo = nullptr;
    QOpenGLBuffer *ebo = nullptr;
    int indexCount = 0;
    QMatrix4x4 transform;
};

struct GLBModel {
    std::vector<GLBMesh> meshes;
    bool loaded = false;
};

class VideoGLWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    explicit VideoGLWidget(QWidget *parent = nullptr);
    ~VideoGLWidget();

public:
    void updateFrame(const cv::Mat &frame);
    void updateLandmarks(const mediapipe::tasks::vision::face_landmarker::FaceLandmarkerResult &landmarks);
    void setAvatarIndex(int index);
    void setFPS(int fps);
    bool loadGLBModel(const QString &filePath);  // GLB 파일 로드

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

private:
    void setupShaders();
    void setupVideoQuad();
    void setupAvatarModel();
    void updateVideoTexture(const cv::Mat &frame);
    void renderVideo();
    void renderAvatar(const mediapipe::tasks::vision::face_landmarker::FaceLandmarkerResult &landmarks);
    bool loadGLBWithTinyGLTF(const QString &filePath);  // tinygltf를 사용한 로딩
    void renderGLBModel(const QMatrix4x4 &modelMatrix, const QVector3D &color);  // GLB 모델 렌더링

    // OpenGL resources
    QOpenGLShaderProgram *videoShaderProgram;
    QOpenGLShaderProgram *avatarShaderProgram;
    QOpenGLTexture *videoTexture;
    
    // Video quad
    QOpenGLVertexArrayObject *videoVAO;
    QOpenGLBuffer *videoVBO;
    QOpenGLBuffer *videoEBO;
    
    // Avatar model - GLB 모델 또는 기본 큐브
    GLBModel glbModel;  // GLB 모델 데이터
    QOpenGLVertexArrayObject *avatarVAO;  // 기본 큐브 (GLB가 없을 때)
    QOpenGLBuffer *avatarVBO;
    QOpenGLBuffer *avatarEBO;
    
    // Matrices
    QMatrix4x4 projectionMatrix;
    QMatrix4x4 viewMatrix;
    
    // Frame data
    QMutex frameMutex;
    cv::Mat currentFrame;
    bool frameUpdated;
    
    // Landmark data
    QMutex landmarkMutex;
    mediapipe::tasks::vision::face_landmarker::FaceLandmarkerResult currentLandmarks;
    bool landmarksUpdated;
    
    // Avatar settings
    int avatarIndex;
    
    // Viewport
    int viewportWidth;
    int viewportHeight;
};

#endif // VIDEOGLWIDGET_H
