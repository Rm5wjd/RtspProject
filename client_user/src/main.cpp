#include <iostream>
#include <vector>
#include <thread>
#include <chrono>

// OpenCV for face detection
#include <opencv2/opencv.hpp>
#include <opencv2/objdetect.hpp>
#include <opencv2/imgproc.hpp>

// OpenGL
#include <GL/glew.h>
#include <GLFW/glfw3.h>

// Mode switch: 1 = local video file, 0 = server stream
#define USE_LOCAL_VIDEO 1

class FaceAvatarRenderer {
private:
    GLFWwindow* window;
    GLuint textureID;
    GLuint shaderProgram;
    GLuint VAO, VBO, EBO;
    
    cv::CascadeClassifier faceCascade;
    cv::VideoCapture videoCapture;
    
    int windowWidth;
    int windowHeight;
    
    // Avatar rendering data
    struct AvatarData {
        float x, y, width, height;
        bool detected;
    };
    std::vector<AvatarData> detectedFaces;
    
public:
    FaceAvatarRenderer() : window(nullptr), textureID(0), windowWidth(1280), windowHeight(720) {
    }
    
    ~FaceAvatarRenderer() {
        cleanup();
    }
    
    bool initialize() {
        // Initialize GLFW
        if (!glfwInit()) {
            std::cerr << "Failed to initialize GLFW" << std::endl;
            return false;
        }
        
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        
        window = glfwCreateWindow(windowWidth, windowHeight, "Face Avatar Renderer", nullptr, nullptr);
        if (!window) {
            std::cerr << "Failed to create GLFW window" << std::endl;
            glfwTerminate();
            return false;
        }
        
        glfwMakeContextCurrent(window);
        
        // Initialize GLEW
        glewExperimental = GL_TRUE;
        if (glewInit() != GLEW_OK) {
            std::cerr << "Failed to initialize GLEW" << std::endl;
            return false;
        }
        
        // Load face cascade - 여러 경로 시도
        std::vector<std::string> possiblePaths = {
            "../resource/haarcascade_frontalface_alt.xml",  // build/bin에서 실행 시
            "../../resource/haarcascade_frontalface_alt.xml", // build에서 실행 시
            "resource/haarcascade_frontalface_alt.xml",     // 프로젝트 루트에서 실행 시
            "/Users/jincheol/Desktop/RtspProject/resource/haarcascade_frontalface_alt.xml" // 절대 경로
        };
        
        bool cascadeLoaded = false;
        for (const auto& path : possiblePaths) {
            if (faceCascade.load(path)) {
                std::cout << "Loaded face cascade from: " << path << std::endl;
                cascadeLoaded = true;
                break;
            }
        }
        
        if (!cascadeLoaded) {
            std::cerr << "Failed to load face cascade from any of the following paths:" << std::endl;
            for (const auto& path : possiblePaths) {
                std::cerr << "  - " << path << std::endl;
            }
            std::cerr << "Please ensure haarcascade_frontalface_alt.xml exists in resource folder" << std::endl;
            return false;
        }
        
        // Initialize video source
        #if USE_LOCAL_VIDEO
        // MacBook 내장 카메라 사용 (인덱스 0)
        // 비디오 파일을 사용하려면 경로를 지정: videoCapture.open("path/to/video.mp4")
        //videoCapture.open(0);
        videoCapture.open("/Users/jincheol/Desktop/RtspProject/resource/hitto.h264");
        if (!videoCapture.isOpened()) {
            std::cerr << "Failed to open camera. Please check:" << std::endl;
            std::cerr << "1. Camera permissions in System Settings > Privacy & Security > Camera" << std::endl;
            std::cerr << "2. Camera is not being used by another application" << std::endl;
            std::cerr << "3. Try using a video file instead: videoCapture.open(\"path/to/video.mp4\")" << std::endl;
            return false;
        }
        
        // 카메라 해상도 설정 (선택사항)
        videoCapture.set(cv::CAP_PROP_FRAME_WIDTH, 1280);
        videoCapture.set(cv::CAP_PROP_FRAME_HEIGHT, 720);
        #else
        // TODO: Initialize server connection for RTSP stream
        #endif
        
        // Setup OpenGL
        setupOpenGL();
        
        return true;
    }
    
    void setupOpenGL() {
        // Create texture for video frame
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        
        // Create shader program
        const char* vertexShaderSource = R"(
            #version 330 core
            layout (location = 0) in vec2 aPos;
            layout (location = 1) in vec2 aTexCoord;
            out vec2 TexCoord;
            void main() {
                gl_Position = vec4(aPos, 0.0, 1.0);
                TexCoord = aTexCoord;
            }
        )";
        
        const char* fragmentShaderSource = R"(
            #version 330 core
            in vec2 TexCoord;
            out vec4 FragColor;
            uniform sampler2D videoTexture;
            void main() {
                FragColor = texture(videoTexture, TexCoord);
            }
        )";
        
        shaderProgram = createShaderProgram(vertexShaderSource, fragmentShaderSource);
        
        // Setup quad for video rendering
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
        
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);
        
        glBindVertexArray(VAO);
        
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
        
        // Position attribute
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        
        // Texture coordinate attribute
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        glEnableVertexAttribArray(1);
        
        glBindVertexArray(0);
    }
    
    GLuint createShaderProgram(const char* vertexSource, const char* fragmentSource) {
        GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vertexSource, nullptr);
        glCompileShader(vertexShader);
        
        GLint success;
        glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
        if (!success) {
            char infoLog[512];
            glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
            std::cerr << "Vertex shader compilation failed: " << infoLog << std::endl;
        }
        
        GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fragmentSource, nullptr);
        glCompileShader(fragmentShader);
        
        glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
        if (!success) {
            char infoLog[512];
            glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
            std::cerr << "Fragment shader compilation failed: " << infoLog << std::endl;
        }
        
        GLuint program = glCreateProgram();
        glAttachShader(program, vertexShader);
        glAttachShader(program, fragmentShader);
        glLinkProgram(program);
        
        glGetProgramiv(program, GL_LINK_STATUS, &success);
        if (!success) {
            char infoLog[512];
            glGetProgramInfoLog(program, 512, nullptr, infoLog);
            std::cerr << "Shader program linking failed: " << infoLog << std::endl;
        }
        
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        
        return program;
    }
    
    void detectFaces(cv::Mat& frame) {
        detectedFaces.clear();
        
        cv::Mat gray;
        cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
        cv::equalizeHist(gray, gray);
        
        std::vector<cv::Rect> faces;
        faceCascade.detectMultiScale(gray, faces, 1.1, 3, 0, cv::Size(30, 30));
        
        // 얼굴 검출 결과를 프레임에 직접 그리기 (바운딩 박스)
        for (const auto& face : faces) {
            // 빨간색 바운딩 박스 그리기
            cv::rectangle(frame, face, cv::Scalar(0, 0, 255), 3); // BGR 형식이므로 (0,0,255) = 빨간색
            
            // 얼굴 위치 정보 저장 (나중에 아바타 렌더링용)
            AvatarData avatar;
            avatar.x = (float)face.x / frame.cols * 2.0f - 1.0f;
            avatar.y = 1.0f - (float)face.y / frame.rows * 2.0f;
            avatar.width = (float)face.width / frame.cols * 2.0f;
            avatar.height = (float)face.height / frame.rows * 2.0f;
            avatar.detected = true;
            detectedFaces.push_back(avatar);
        }
        
        // 얼굴 검출 개수 출력 (디버그용)
        if (faces.size() > 0) {
            static int frameCount = 0;
            if (frameCount % 30 == 0) { // 30프레임마다 출력 (약 1초마다)
                std::cout << "Detected " << faces.size() << " face(s)" << std::endl;
            }
            frameCount++;
        }
    }
    
    void renderVideoFrame(cv::Mat& frame) {
        // Convert BGR to RGB
        cv::Mat rgbFrame;
        cv::cvtColor(frame, rgbFrame, cv::COLOR_BGR2RGB);
        cv::flip(rgbFrame, rgbFrame, 0); // Flip vertically for OpenGL
        
        // Upload texture
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, rgbFrame.cols, rgbFrame.rows, 0, GL_RGB, GL_UNSIGNED_BYTE, rgbFrame.data);
        
        // Render video quad
        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
    
    void renderAvatars() {
        // TODO: Render 3D avatar models on detected face positions
        // 바운딩 박스는 detectFaces()에서 OpenCV로 직접 그려지므로
        // 여기서는 나중에 3D 아바타를 렌더링할 예정
        // 현재는 바운딩 박스가 비디오 프레임에 직접 그려져서 표시됨
    }
    
    void run() {
        cv::Mat frame;
        
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            
            // Capture frame
            #if USE_LOCAL_VIDEO
            videoCapture >> frame;
            if (frame.empty()) {
                break;
            }
            #else
            // TODO: Receive frame from server
            #endif
            
            // Detect faces
            detectFaces(frame);
            
            // Render
            glClear(GL_COLOR_BUFFER_BIT);
            
            renderVideoFrame(frame);
            renderAvatars();
            
            glfwSwapBuffers(window);
            
            std::this_thread::sleep_for(std::chrono::milliseconds(33)); // ~30 FPS
        }
    }
    
    void cleanup() {
        if (textureID) {
            glDeleteTextures(1, &textureID);
        }
        if (shaderProgram) {
            glDeleteProgram(shaderProgram);
        }
        if (VAO) {
            glDeleteVertexArrays(1, &VAO);
        }
        if (VBO) {
            glDeleteBuffers(1, &VBO);
        }
        if (EBO) {
            glDeleteBuffers(1, &EBO);
        }
        if (window) {
            glfwDestroyWindow(window);
        }
        glfwTerminate();
        
        if (videoCapture.isOpened()) {
            videoCapture.release();
        }
    }
};

int main() {
    FaceAvatarRenderer renderer;
    
    if (!renderer.initialize()) {
        std::cerr << "Failed to initialize renderer" << std::endl;
        return -1;
    }
    
    renderer.run();
    
    return 0;
}
