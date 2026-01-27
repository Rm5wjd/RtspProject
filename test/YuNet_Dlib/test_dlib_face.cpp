#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <chrono>
#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <opencv2/objdetect.hpp>  // FaceDetectorYN (OpenCV 4.5.1+)
#include <dlib/opencv.h>
#include <dlib/image_processing.h>
#include <dlib/image_processing/frontal_face_detector.h>
#include <dlib/image_processing/shape_predictor.h>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <image_path> [output_path]" << std::endl;
        std::cerr << "Example: " << argv[0] << " test_image.jpg output.jpg" << std::endl;
        return 1;
    }
    
    std::string imagePath = argv[1];
    std::string outputPath = (argc >= 3) ? argv[2] : "output_detected.jpg";
    
    // 이미지 로드
    cv::Mat img = cv::imread(imagePath);
    if (img.empty()) {
        std::cerr << "Error: Could not load image from " << imagePath << std::endl;
        return 1;
    }
    
    std::cout << "Image loaded: " << imagePath << std::endl;
    std::cout << "Original image size: " << img.cols << "x" << img.rows << std::endl;
    
    // 이미지 크기 강제 축소 (속도 최적화 - 가장 큰 효과)
    cv::Mat processedImg;
    double scale = 1.0;
    const int TARGET_WIDTH = 640;   // 목표 너비 (더 작게 설정)
    const int TARGET_HEIGHT = 960;  // 목표 높이
    
    // 비율 유지하며 리사이즈
    if (img.cols > TARGET_WIDTH || img.rows > TARGET_HEIGHT) {
        double scaleX = static_cast<double>(TARGET_WIDTH) / img.cols;
        double scaleY = static_cast<double>(TARGET_HEIGHT) / img.rows;
        scale = std::min(scaleX, scaleY);  // 더 작은 스케일 사용
        
        int newWidth = static_cast<int>(img.cols * scale);
        int newHeight = static_cast<int>(img.rows * scale);
        
        cv::resize(img, processedImg, cv::Size(newWidth, newHeight), 0, 0, cv::INTER_LINEAR);
        std::cout << "Resized for processing: " << newWidth << "x" << newHeight 
                  << " (scale: " << scale << ", original: " << img.cols << "x" << img.rows << ")" << std::endl;
    } else {
        processedImg = img.clone();
        std::cout << "Image size is small enough, using original size" << std::endl;
    }
    
    // OpenCV YuNet 얼굴 탐지 모델 로드 (하이브리드 방식 - 빠르고 정확한 detection)
    cv::Ptr<cv::FaceDetectorYN> faceDetector;
    std::vector<std::string> yunetPaths = {
        "../../resource/face_detection_yunet_2023mar.onnx",
        "../resource/face_detection_yunet_2023mar.onnx",
        "/Users/jincheol/Desktop/VEDA/RtspProject/resource/face_detection_yunet_2023mar.onnx",
        "face_detection_yunet_2023mar.onnx"
    };
    
    bool yunetLoaded = false;
    std::string yunetModelPath;
    for (const auto& path : yunetPaths) {
        std::ifstream file(path, std::ios::binary);
        if (file.good()) {
            // 파일 크기 확인 (최소 크기 체크)
            file.seekg(0, std::ios::end);
            size_t fileSize = file.tellg();
            file.close();
            
            if (fileSize < 1000) {  // 1KB 미만이면 손상된 파일
                std::cout << "Warning: YuNet model file too small (" << fileSize << " bytes), skipping: " << path << std::endl;
                continue;
            }
            
            yunetModelPath = path;
            try {
                // FaceDetectorYN 생성 (OpenCV 4.5.1+)
                faceDetector = cv::FaceDetectorYN::create(path, "", cv::Size(processedImg.cols, processedImg.rows));
                if (!faceDetector.empty()) {
                    yunetLoaded = true;
                    std::cout << "YuNet model loaded from: " << path << " (" << fileSize << " bytes)" << std::endl;
                    break;
                }
            } catch (const cv::Exception& e) {
                std::cout << "Warning: Failed to load YuNet model from " << path << std::endl;
                std::cout << "  Error: " << e.what() << std::endl;
                std::cout << "  File size: " << fileSize << " bytes" << std::endl;
                std::cout << "  The model file may be corrupted. Please re-download it." << std::endl;
                continue;
            } catch (const std::exception& e) {
                std::cout << "Warning: Exception while loading YuNet model: " << e.what() << std::endl;
                continue;
            }
        }
    }
    
    if (!yunetLoaded) {
        std::cout << "Warning: YuNet model not available or corrupted." << std::endl;
        std::cout << "Please download from:" << std::endl;
        std::cout << "https://github.com/opencv/opencv_zoo/raw/master/models/face_detection_yunet/face_detection_yunet_2023mar.onnx" << std::endl;
        std::cout << "Using dlib detector as fallback..." << std::endl;
    }
    
    // dlib 얼굴 탐지기 초기화 (fallback용)
    dlib::frontal_face_detector detector = dlib::get_frontal_face_detector();
    
    // shape predictor 모델 경로 찾기
    std::vector<std::string> possiblePaths = {
        "../client_user/thirdparty/dlib/models/shape_predictor_68_face_landmarks.dat",
        "../../client_user/thirdparty/dlib/models/shape_predictor_68_face_landmarks.dat",
        "thirdparty/dlib/models/shape_predictor_68_face_landmarks.dat",
        "/Users/jincheol/Desktop/VEDA/RtspProject/client_user/thirdparty/dlib/models/shape_predictor_68_face_landmarks.dat",
        "/usr/local/share/dlib/shape_predictor_68_face_landmarks.dat",
        "/opt/homebrew/share/dlib/shape_predictor_68_face_landmarks.dat"
    };
    
    std::string modelPath;
    bool modelFound = false;
    for (const auto& path : possiblePaths) {
        std::ifstream file(path);
        if (file.good()) {
            modelPath = path;
            modelFound = true;
            std::cout << "Found dlib model at: " << path << std::endl;
            break;
        }
    }
    
    if (!modelFound) {
        std::cerr << "Error: dlib model file not found!" << std::endl;
        std::cerr << "Please download shape_predictor_68_face_landmarks.dat from:" << std::endl;
        std::cerr << "http://dlib.net/files/shape_predictor_68_face_landmarks.dat.bz2" << std::endl;
        return 1;
    }
    
    // shape predictor 로드
    dlib::shape_predictor sp;
    try {
        dlib::deserialize(modelPath) >> sp;
        std::cout << "dlib model loaded successfully" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error loading dlib model: " << e.what() << std::endl;
        return 1;
    }
    
    // OpenCV Mat을 dlib image로 변환 (리사이즈된 이미지 사용)
    cv::Mat gray;
    cv::cvtColor(processedImg, gray, cv::COLOR_BGR2GRAY);
    
    dlib::cv_image<dlib::bgr_pixel> dlibImg(processedImg);
    dlib::cv_image<unsigned char> dlibGray(gray);
    
    // 전체 처리 시간 측정 시작
    auto totalStart = std::chrono::steady_clock::now();
    
    // 얼굴 탐지 (YuNet 우선 사용, 실패 시 dlib fallback)
    std::cout << "Detecting faces..." << std::endl;
    auto start = std::chrono::steady_clock::now();
    std::vector<dlib::rectangle> faces;
    
    if (yunetLoaded) {
        // YuNet으로 얼굴 탐지
        cv::Mat detections;
        faceDetector->setInputSize(cv::Size(processedImg.cols, processedImg.rows));
        faceDetector->detect(processedImg, detections);
        
        // YuNet 결과 파싱
        // detections: [N, 15] 행렬
        // 각 행: [x1, y1, w, h, x_re, y_re, x_le, y_le, x_nt, y_nt, x_rcm, y_rcm, x_lcm, y_lcm, score]
        for (int i = 0; i < detections.rows; ++i) {
            float confidence = detections.at<float>(i, 14);
            
            // confidence threshold (0.5 이상만 사용)
            if (confidence > 0.5f) {
                float x1 = detections.at<float>(i, 0);
                float y1 = detections.at<float>(i, 1);
                float w = detections.at<float>(i, 2);
                float h = detections.at<float>(i, 3);
                
                // dlib rectangle로 변환
                dlib::rectangle drect(
                    static_cast<long>(x1),
                    static_cast<long>(y1),
                    static_cast<long>(x1 + w),
                    static_cast<long>(y1 + h)
                );
                faces.push_back(drect);
            }
        }
        
        std::cout << "  Method: OpenCV YuNet (DNN-based, fast & accurate)" << std::endl;
    } else {
        // dlib detector 사용 (fallback, upsample 0으로 설정 - 더 빠름)
        faces = detector(dlibGray, 0);  // 0 = upsample 안 함 (속도 향상)
        std::cout << "  Method: dlib detector (upsample=0, fallback)" << std::endl;
    }
    
    auto end = std::chrono::steady_clock::now();
    auto detectionMs = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    auto detectionUs = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    std::cout << "Face detection completed:" << std::endl;
    std::cout << "  Time: " << detectionMs << " ms (" << detectionUs << " us)" << std::endl;
    std::cout << "  Found " << faces.size() << " face(s)" << std::endl;
    
    if (faces.empty()) {
        auto totalEnd = std::chrono::steady_clock::now();
        auto totalMs = std::chrono::duration_cast<std::chrono::milliseconds>(totalEnd - totalStart).count();
        std::cout << "\n=== Total Processing Time ===" << std::endl;
        std::cout << "Total: " << totalMs << " ms" << std::endl;
        std::cout << "No faces detected in the image." << std::endl;
        cv::imwrite(outputPath, img);
        std::cout << "Original image saved to: " << outputPath << std::endl;
        return 0;
    }
    
    // 탐지된 얼굴에 대해 특징점 예측
    std::cout << "\nPredicting facial landmarks..." << std::endl;
    start = std::chrono::steady_clock::now();
    std::vector<dlib::full_object_detection> shapes;
    for (const auto& face : faces) {
        dlib::full_object_detection shape = sp(dlibImg, face);
        shapes.push_back(shape);
    }
    end = std::chrono::steady_clock::now();
    auto landmarkMs = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    auto landmarkUs = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    std::cout << "Landmark prediction completed:" << std::endl;
    std::cout << "  Time: " << landmarkMs << " ms (" << landmarkUs << " us)" << std::endl;
    std::cout << "  Per face: " << (landmarkMs / faces.size()) << " ms" << std::endl;
    
    // 결과 이미지 생성 (원본 크기로)
    cv::Mat resultImg = img.clone();
    
    // 리사이즈된 좌표를 원본 크기로 변환
    for (size_t i = 0; i < faces.size(); ++i) {
        const auto& face = faces[i];
        const auto& shape = shapes[i];
        
        // 좌표를 원본 크기로 스케일링
        int origLeft = static_cast<int>(face.left() / scale);
        int origTop = static_cast<int>(face.top() / scale);
        int origWidth = static_cast<int>(face.width() / scale);
        int origHeight = static_cast<int>(face.height() / scale);
        
        // 얼굴 영역 그리기 (초록색 사각형) - 원본 크기
        cv::Rect faceRect(origLeft, origTop, origWidth, origHeight);
        cv::rectangle(resultImg, faceRect, cv::Scalar(0, 255, 0), 2);
        
        // 특징점 그리기 (68개 점) - 원본 크기로 스케일링
        for (unsigned long j = 0; j < shape.num_parts(); ++j) {
            const auto& part = shape.part(j);
            int origX = static_cast<int>(part.x() / scale);
            int origY = static_cast<int>(part.y() / scale);
            cv::Point pt(origX, origY);
            int radius = std::max(1, static_cast<int>(2 / scale));  // 스케일에 맞게 점 크기 조정
            cv::circle(resultImg, pt, radius, cv::Scalar(0, 0, 255), -1);  // 빨간색 점
        }
        
        // 얼굴 번호 표시
        cv::putText(resultImg, 
                   "Face " + std::to_string(i + 1),
                   cv::Point(origLeft, origTop - 10),
                   cv::FONT_HERSHEY_SIMPLEX,
                   0.7 * (1.0 / scale),  // 폰트 크기도 스케일 조정
                   cv::Scalar(0, 255, 0),
                   2);
        
        std::cout << "Face " << (i + 1) << ": " << origWidth << "x" << origHeight 
                  << " at (" << origLeft << ", " << origTop << ")" << std::endl;
        std::cout << "  Landmarks: " << shape.num_parts() << " points" << std::endl;
    }
    
    // 결과 이미지 저장
    auto saveStart = std::chrono::steady_clock::now();
    cv::imwrite(outputPath, resultImg);
    auto saveEnd = std::chrono::steady_clock::now();
    auto saveMs = std::chrono::duration_cast<std::chrono::milliseconds>(saveEnd - saveStart).count();
    
    // 전체 처리 시간 계산
    auto totalEnd = std::chrono::steady_clock::now();
    auto totalMs = std::chrono::duration_cast<std::chrono::milliseconds>(totalEnd - totalStart).count();
    auto totalUs = std::chrono::duration_cast<std::chrono::microseconds>(totalEnd - totalStart).count();
    
    // 시간 요약 출력
    std::cout << "\n=== Processing Time Summary ===" << std::endl;
    std::cout << "Face Detection:      " << detectionMs << " ms (" << detectionUs << " us)" << std::endl;
    std::cout << "Landmark Prediction: " << landmarkMs << " ms (" << landmarkUs << " us)" << std::endl;
    std::cout << "  - Per face:        " << (faces.size() > 0 ? landmarkMs / faces.size() : 0) << " ms" << std::endl;
    std::cout << "Image Saving:        " << saveMs << " ms" << std::endl;
    std::cout << "--------------------------------" << std::endl;
    std::cout << "Total Processing:     " << totalMs << " ms (" << totalUs << " us)" << std::endl;
    std::cout << "Estimated FPS:       " << (totalMs > 0 ? 1000.0 / totalMs : 0.0) << " fps" << std::endl;
    std::cout << "================================" << std::endl;
    
    std::cout << "\nResult saved to: " << outputPath << std::endl;
    
    // 결과 이미지 표시 (선택사항)
    cv::imshow("Face Detection Result", resultImg);
    std::cout << "Press any key to close the window..." << std::endl;
    cv::waitKey(0);
    cv::destroyAllWindows();
    
    return 0;
}
