#include "VideoGLWidget.h"
#include <QOpenGLContext>
#include <QFile>
#include <QFileInfo>
#include <iostream>
#include <fstream>

// tinygltf 헤더 포함 (사용자가 다운로드해야 함)
#ifdef TINYGLTF_AVAILABLE
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "thirdparty/tinygltf/tiny_gltf.h"
#endif

// Vertex shader for video
static const char *videoVertexShaderSource =
    "#version 330 core\n"
    "layout (location = 0) in vec2 aPos;\n"
    "layout (location = 1) in vec2 aTexCoord;\n"
    "out vec2 TexCoord;\n"
    "void main() {\n"
    "   gl_Position = vec4(aPos, 0.0, 1.0);\n"
    "   TexCoord = aTexCoord;\n"
    "}\n";

// Fragment shader for video
static const char *videoFragmentShaderSource =
    "#version 330 core\n"
    "in vec2 TexCoord;\n"
    "out vec4 FragColor;\n"
    "uniform sampler2D videoTexture;\n"
    "void main() {\n"
    "   FragColor = texture(videoTexture, TexCoord);\n"
    "}\n";

// Vertex shader for 3D avatar
static const char *avatarVertexShaderSource =
    "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "uniform mat4 model;\n"
    "uniform mat4 view;\n"
    "uniform mat4 projection;\n"
    "void main() {\n"
    "   gl_Position = projection * view * model * vec4(aPos, 1.0);\n"
    "}\n";

// Fragment shader for 3D avatar
static const char *avatarFragmentShaderSource =
    "#version 330 core\n"
    "out vec4 FragColor;\n"
    "uniform vec3 avatarColor;\n"
    "void main() {\n"
    "   FragColor = vec4(avatarColor, 1.0);\n"
    "}\n";

VideoGLWidget::VideoGLWidget(QWidget *parent)
    : QOpenGLWidget(parent)
    , videoShaderProgram(nullptr)
    , avatarShaderProgram(nullptr)
    , videoTexture(nullptr)
    , videoVAO(nullptr)
    , videoVBO(nullptr)
    , videoEBO(nullptr)
    , avatarVAO(nullptr)
    , avatarVBO(nullptr)
    , avatarEBO(nullptr)
    , frameUpdated(false)
    , landmarksUpdated(false)
    , avatarIndex(-1)
    , viewportWidth(640)
    , viewportHeight(480)
{
}

VideoGLWidget::~VideoGLWidget()
{
    makeCurrent();
    
    if (videoTexture) {
        delete videoTexture;
    }
    if (videoVAO) delete videoVAO;
    if (videoVBO) delete videoVBO;
    if (videoEBO) delete videoEBO;
    if (avatarVAO) delete avatarVAO;
    if (avatarVBO) delete avatarVBO;
    if (avatarEBO) delete avatarEBO;
    if (videoShaderProgram) delete videoShaderProgram;
    if (avatarShaderProgram) delete avatarShaderProgram;
    
    // GLB 모델 메시 정리
    for (auto& mesh : glbModel.meshes) {
        if (mesh.vao) delete mesh.vao;
        if (mesh.vbo) delete mesh.vbo;
        if (mesh.ebo) delete mesh.ebo;
    }
    glbModel.meshes.clear();
    
    doneCurrent();
}

void VideoGLWidget::initializeGL()
{
    initializeOpenGLFunctions();
    
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    setupShaders();
    setupVideoQuad();
    setupAvatarModel();
    
    // 초기 행렬 설정
    projectionMatrix.setToIdentity();
    viewMatrix.setToIdentity();
}

void VideoGLWidget::resizeGL(int w, int h)
{
    viewportWidth = w;
    viewportHeight = h;
    glViewport(0, 0, w, h);
    
    // Orthographic projection for video (2D)
    projectionMatrix.setToIdentity();
    projectionMatrix.ortho(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);
}

void VideoGLWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // 비디오 렌더링
    renderVideo();
    
    // 아바타 렌더링 (랜드마크가 있을 때만)
    QMutexLocker landmarkLock(&landmarkMutex);
    if (landmarksUpdated && !currentLandmarks.face_landmarks.empty() && avatarIndex >= 0) {
        renderAvatar(currentLandmarks);
    }
}

void VideoGLWidget::setupShaders()
{
    // Video shader
    videoShaderProgram = new QOpenGLShaderProgram(this);
    videoShaderProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, videoVertexShaderSource);
    videoShaderProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, videoFragmentShaderSource);
    videoShaderProgram->link();
    
    if (!videoShaderProgram->isLinked()) {
        std::cerr << "Video shader link error: " << videoShaderProgram->log().toStdString() << std::endl;
    }
    
    // Avatar shader
    avatarShaderProgram = new QOpenGLShaderProgram(this);
    avatarShaderProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, avatarVertexShaderSource);
    avatarShaderProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, avatarFragmentShaderSource);
    avatarShaderProgram->link();
    
    if (!avatarShaderProgram->isLinked()) {
        std::cerr << "Avatar shader link error: " << avatarShaderProgram->log().toStdString() << std::endl;
    }
}

void VideoGLWidget::setupVideoQuad()
{
    // Full screen quad for video
    float vertices[] = {
        // positions   // tex coords
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f,
        -1.0f,  1.0f,  0.0f, 1.0f
    };
    
    unsigned int indices[] = {
        0, 1, 2,
        2, 3, 0
    };
    
    videoVAO = new QOpenGLVertexArrayObject(this);
    videoVBO = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    videoEBO = new QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
    
    videoVAO->create();
    videoVAO->bind();
    
    videoVBO->create();
    videoVBO->bind();
    videoVBO->allocate(vertices, sizeof(vertices));
    
    videoEBO->create();
    videoEBO->bind();
    videoEBO->allocate(indices, sizeof(indices));
    
    // Position attribute
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Texture coordinate attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    videoVAO->release();
}

void VideoGLWidget::setupAvatarModel()
{
    // 간단한 큐브 모델 (나중에 더 복잡한 모델로 교체 가능)
    float vertices[] = {
        // Front face
        -0.1f, -0.1f,  0.1f,
         0.1f, -0.1f,  0.1f,
         0.1f,  0.1f,  0.1f,
        -0.1f,  0.1f,  0.1f,
        // Back face
        -0.1f, -0.1f, -0.1f,
         0.1f, -0.1f, -0.1f,
         0.1f,  0.1f, -0.1f,
        -0.1f,  0.1f, -0.1f
    };
    
    unsigned int indices[] = {
        // Front
        0, 1, 2, 2, 3, 0,
        // Back
        4, 5, 6, 6, 7, 4,
        // Left
        0, 3, 7, 7, 4, 0,
        // Right
        1, 2, 6, 6, 5, 1,
        // Top
        3, 2, 6, 6, 7, 3,
        // Bottom
        0, 1, 5, 5, 4, 0
    };
    
    avatarVAO = new QOpenGLVertexArrayObject(this);
    avatarVBO = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    avatarEBO = new QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
    
    avatarVAO->create();
    avatarVAO->bind();
    
    avatarVBO->create();
    avatarVBO->bind();
    avatarVBO->allocate(vertices, sizeof(vertices));
    
    avatarEBO->create();
    avatarEBO->bind();
    avatarEBO->allocate(indices, sizeof(indices));
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    avatarVAO->release();
}

void VideoGLWidget::updateFrame(const cv::Mat &frame)
{
    QMutexLocker lock(&frameMutex);
    if (!frame.empty()) {
        currentFrame = frame.clone();
        frameUpdated = true;
    }
}

void VideoGLWidget::updateLandmarks(const mediapipe::tasks::vision::face_landmarker::FaceLandmarkerResult &landmarks)
{
    QMutexLocker lock(&landmarkMutex);
    currentLandmarks = landmarks;
    landmarksUpdated = true;
}

void VideoGLWidget::setAvatarIndex(int index)
{
    avatarIndex = index;
}

void VideoGLWidget::setFPS(int fps)
{
    // FPS는 타이머에서 처리되므로 여기서는 필요 없음
    Q_UNUSED(fps);
}

void VideoGLWidget::updateVideoTexture(const cv::Mat &frame)
{
    if (frame.empty()) return;
    
    cv::Mat rgbFrame;
    cv::cvtColor(frame, rgbFrame, cv::COLOR_BGR2RGB);
    cv::flip(rgbFrame, rgbFrame, 0);  // OpenGL 좌표계에 맞게 뒤집기
    
    if (!videoTexture) {
        videoTexture = new QOpenGLTexture(QOpenGLTexture::Target2D);
        videoTexture->setFormat(QOpenGLTexture::RGB8_UNorm);
        videoTexture->setMinificationFilter(QOpenGLTexture::Linear);
        videoTexture->setMagnificationFilter(QOpenGLTexture::Linear);
        videoTexture->setWrapMode(QOpenGLTexture::ClampToEdge);
    }
    
    videoTexture->setSize(rgbFrame.cols, rgbFrame.rows);
    videoTexture->allocateStorage();
    videoTexture->bind();
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, rgbFrame.cols, rgbFrame.rows,
                    GL_RGB, GL_UNSIGNED_BYTE, rgbFrame.data);
    videoTexture->release();
}

void VideoGLWidget::renderVideo()
{
    QMutexLocker lock(&frameMutex);
    
    if (frameUpdated && !currentFrame.empty()) {
        updateVideoTexture(currentFrame);
        frameUpdated = false;
    }
    
    if (!videoTexture) return;
    
    videoShaderProgram->bind();
    videoVAO->bind();
    videoTexture->bind();
    
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    
    videoTexture->release();
    videoVAO->release();
    videoShaderProgram->release();
}

void VideoGLWidget::renderAvatar(const mediapipe::tasks::vision::face_landmarker::FaceLandmarkerResult &landmarks)
{
    if (landmarks.face_landmarks.empty()) return;
    
    const auto& faceLandmarks = landmarks.face_landmarks[0];
    if (faceLandmarks.empty()) return;
    
    // 얼굴 중심 및 크기 계산
    float minX = 1.0f, minY = 1.0f, maxX = 0.0f, maxY = 0.0f, minZ = 1.0f, maxZ = 0.0f;
    for (const auto& landmark : faceLandmarks) {
        minX = std::min(minX, landmark.x);
        minY = std::min(minY, landmark.y);
        maxX = std::max(maxX, landmark.x);
        maxY = std::max(maxY, landmark.y);
        minZ = std::min(minZ, landmark.z);
        maxZ = std::max(maxZ, landmark.z);
    }
    
    // 정규화된 좌표를 OpenGL 좌표계로 변환
    float centerX = (minX + maxX) / 2.0f;
    float centerY = 1.0f - (minY + maxY) / 2.0f;  // Y축 뒤집기
    float width = maxX - minX;
    float height = maxY - minY;
    float depth = maxZ - minZ;
    
    // OpenGL 좌표계로 변환 (-1 ~ 1)
    float glX = centerX * 2.0f - 1.0f;
    float glY = centerY * 2.0f - 1.0f;
    float glWidth = width * 2.0f;
    float glHeight = height * 2.0f;
    
    // 아바타 색상 (캐릭터별)
    QVector3D colors[] = {
        QVector3D(1.0f, 0.0f, 0.0f),  // 빨강
        QVector3D(0.0f, 1.0f, 0.0f),  // 초록
        QVector3D(0.0f, 0.0f, 1.0f),  // 파랑
        QVector3D(1.0f, 1.0f, 0.0f)   // 노랑
    };
    
    QVector3D avatarColor = (avatarIndex >= 0 && avatarIndex < 4) ? colors[avatarIndex] : QVector3D(1.0f, 1.0f, 1.0f);
    
    // 모델 행렬 생성 (얼굴 위치에 배치)
    QMatrix4x4 modelMatrix;
    modelMatrix.setToIdentity();
    modelMatrix.translate(glX, glY, 0.0f);
    modelMatrix.scale(glWidth, glHeight, 1.0f);
    
    // 아바타 렌더링
    if (glbModel.loaded) {
        // GLB 모델 렌더링
        renderGLBModel(modelMatrix, avatarColor);
    } else {
        // 기본 큐브 렌더링
        avatarShaderProgram->bind();
        avatarVAO->bind();
        
        avatarShaderProgram->setUniformValue("model", modelMatrix);
        avatarShaderProgram->setUniformValue("view", viewMatrix);
        avatarShaderProgram->setUniformValue("projection", projectionMatrix);
        avatarShaderProgram->setUniformValue("avatarColor", avatarColor);
        
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
        
        avatarVAO->release();
        avatarShaderProgram->release();
    }
}

bool VideoGLWidget::loadGLBModel(const QString &filePath)
{
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) {
        std::cerr << "GLB file not found: " << filePath.toStdString() << std::endl;
        return false;
    }
    
#ifdef TINYGLTF_AVAILABLE
    return loadGLBWithTinyGLTF(filePath);
#else
    std::cerr << "TinyGLTF not available. Please download tiny_gltf.h to thirdparty/tinygltf/" << std::endl;
    std::cerr << "Download from: https://github.com/syoyo/tinygltf/releases" << std::endl;
    std::cerr << "And add -DTINYGLTF_AVAILABLE to CMakeLists.txt" << std::endl;
    return false;
#endif
}

#ifdef TINYGLTF_AVAILABLE
bool VideoGLWidget::loadGLBWithTinyGLTF(const QString &filePath)
{
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;
    
    bool ret = loader.LoadBinaryFromFile(&model, &err, &warn, filePath.toStdString());
    
    if (!warn.empty()) {
        std::cout << "TinyGLTF warning: " << warn << std::endl;
    }
    
    if (!err.empty()) {
        std::cerr << "TinyGLTF error: " << err << std::endl;
    }
    
    if (!ret) {
        std::cerr << "Failed to load GLB file: " << filePath.toStdString() << std::endl;
        return false;
    }
    
    makeCurrent();
    
    // 기존 메시 정리
    for (auto& mesh : glbModel.meshes) {
        if (mesh.vao) delete mesh.vao;
        if (mesh.vbo) delete mesh.vbo;
        if (mesh.ebo) delete mesh.ebo;
    }
    glbModel.meshes.clear();
    
    // 모델의 모든 메시 로드
    for (size_t i = 0; i < model.meshes.size(); ++i) {
        const auto& mesh = model.meshes[i];
        
        for (size_t j = 0; j < mesh.primitives.size(); ++j) {
            const auto& primitive = mesh.primitives[j];
            
            GLBMesh glMesh;
            glMesh.vao = new QOpenGLVertexArrayObject(this);
            glMesh.vbo = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
            glMesh.ebo = new QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
            
            glMesh.vao->create();
            glMesh.vao->bind();
            
            // 인덱스 데이터 로드
            if (primitive.indices >= 0) {
                const auto& accessor = model.accessors[primitive.indices];
                const auto& bufferView = model.bufferViews[accessor.bufferView];
                const auto& buffer = model.buffers[bufferView.buffer];
                
                const unsigned char* data = buffer.data.data() + bufferView.byteOffset + accessor.byteOffset;
                int indexCount = accessor.count;
                
                glMesh.ebo->create();
                glMesh.ebo->bind();
                
                if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
                    glMesh.ebo->allocate(data, indexCount * sizeof(unsigned short));
                    glMesh.indexCount = indexCount;
                } else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
                    glMesh.ebo->allocate(data, indexCount * sizeof(unsigned int));
                    glMesh.indexCount = indexCount;
                }
            }
            
            // 정점 데이터 로드
            std::vector<float> vertices;
            int vertexCount = 0;
            
            for (const auto& attrib : primitive.attributes) {
                const auto& accessor = model.accessors[attrib.second];
                const auto& bufferView = model.bufferViews[accessor.bufferView];
                const auto& buffer = model.buffers[bufferView.buffer];
                
                const unsigned char* data = buffer.data.data() + bufferView.byteOffset + accessor.byteOffset;
                
                if (attrib.first == "POSITION") {
                    vertexCount = accessor.count;
                    // 위치 데이터 추출
                    for (int k = 0; k < vertexCount; ++k) {
                        float x = *reinterpret_cast<const float*>(data + k * 12 + 0);
                        float y = *reinterpret_cast<const float*>(data + k * 12 + 4);
                        float z = *reinterpret_cast<const float*>(data + k * 12 + 8);
                        vertices.push_back(x);
                        vertices.push_back(y);
                        vertices.push_back(z);
                    }
                }
            }
            
            if (!vertices.empty()) {
                glMesh.vbo->create();
                glMesh.vbo->bind();
                glMesh.vbo->allocate(vertices.data(), vertices.size() * sizeof(float));
                
                glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
                glEnableVertexAttribArray(0);
            }
            
            glMesh.vao->release();
            glbModel.meshes.push_back(glMesh);
        }
    }
    
    glbModel.loaded = true;
    doneCurrent();
    
    std::cout << "GLB model loaded successfully: " << filePath.toStdString() << std::endl;
    std::cout << "Loaded " << glbModel.meshes.size() << " meshes" << std::endl;
    
    return true;
}
#endif

void VideoGLWidget::renderGLBModel(const QMatrix4x4 &modelMatrix, const QVector3D &color)
{
    if (!glbModel.loaded || glbModel.meshes.empty()) {
        return;
    }
    
    avatarShaderProgram->bind();
    
    avatarShaderProgram->setUniformValue("model", modelMatrix);
    avatarShaderProgram->setUniformValue("view", viewMatrix);
    avatarShaderProgram->setUniformValue("projection", projectionMatrix);
    avatarShaderProgram->setUniformValue("avatarColor", color);
    
    // 모든 메시 렌더링
    for (const auto& mesh : glbModel.meshes) {
        mesh.vao->bind();
        if (mesh.ebo && mesh.indexCount > 0) {
            glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_SHORT, 0);
        }
        mesh.vao->release();
    }
    
    avatarShaderProgram->release();
}
