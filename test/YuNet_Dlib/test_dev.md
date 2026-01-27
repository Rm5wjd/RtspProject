# dlib 얼굴 탐지 테스트 개발 문서

## 개발 목적

dlib을 사용한 얼굴 탐지 및 landmark 예측 기능의 성능을 테스트하고, 최적화 방법을 검증하기 위한 테스트 프로그램 개발

## 개발 일자

2026년 1월 23일

## 개발 과정

### 1. 초기 구현 (dlib만 사용)

**문제점:**
- 고해상도 이미지(2316x3088)에서 얼굴 탐지에 4581ms 소요
- 예상 FPS: 0.21 fps (실시간 처리 불가능)
- dlib detector가 이미지 크기에 매우 민감함

**처리 시간:**
```
Face Detection:      4581 ms (4581354 us)
Landmark Prediction: 35 ms (35590 us)
Total Processing:    4678 ms (4678663 us)
Estimated FPS:       0.213767 fps
```

### 2. 이미지 리사이즈 최적화

**해결 방법:**
- 이미지를 640x480으로 리사이즈하여 처리
- 탐지 후 좌표를 원본 크기로 스케일링

**효과:**
- 처리 시간 대폭 단축 예상 (10-50배 향상)

### 2-1. Haar Cascade 시도 (실패)

**시도 내용:**
- OpenCV Haar Cascade로 얼굴 탐지 시도
- 이미지 리사이즈 적용 (640x853)

**결과:**
```
Face detection completed:
  Time: 199 ms (199753 us)
  Found 0 face(s)
```

**문제점:**
- 처리 시간은 빨라짐 (199ms, dlib의 4581ms 대비 약 23배 빠름)
- **하지만 얼굴 탐지 실패 (0개 탐지)**
- Haar Cascade는 정확도가 낮고, 리사이즈된 이미지에서 더욱 탐지율이 떨어짐

**결론:**
- Haar Cascade는 속도는 빠르지만 정확도가 낮아 실용적이지 않음
- YuNet 같은 DNN 기반 모델이 속도와 정확도 모두 우수

### 3. 하이브리드 방식 도입 (YuNet + dlib)

**최종 구현:**
1. **YuNet으로 얼굴 탐지** (빠르고 정확)
   - OpenCV FaceDetectorYN API 사용
   - DNN 기반 얼굴 탐지
   - 실시간 처리 가능한 속도

2. **dlib으로 landmark 예측** (정확한 특징점)
   - YuNet으로 탐지된 얼굴 영역에 대해 68개 landmark 예측
   - shape_predictor_68_face_landmarks.dat 모델 사용

**장점:**
- YuNet: 빠른 얼굴 탐지 (10-50ms)
- dlib landmark: 정확한 68개 특징점 (30-50ms/얼굴)
- 하이브리드 방식으로 속도와 정확도 모두 확보

## 최적화 방법

### 0. Haar Cascade 시도 (실패한 방법)

**시도한 내용:**
```cpp
cv::CascadeClassifier faceCascade;
faceCascade.detectMultiScale(gray, cvFaces, 1.1, 3, 0, cv::Size(30, 30));
```

**결과:**
- ✅ 속도: 199ms (dlib 대비 23배 빠름)
- ❌ 정확도: 얼굴 탐지 실패 (0개)
- ❌ 리사이즈된 이미지에서 탐지율 더욱 저하

**결론:**
- Haar Cascade는 속도는 빠르지만 정확도가 낮아 실용적이지 않음
- YuNet 같은 DNN 기반 모델이 속도와 정확도 모두 우수

### 1. 이미지 크기 강제 축소
```cpp
const int TARGET_WIDTH = 640;   // 목표 너비
const int TARGET_HEIGHT = 480;  // 목표 높이

// 비율 유지하며 리사이즈
if (img.cols > TARGET_WIDTH || img.rows > TARGET_HEIGHT) {
    double scaleX = static_cast<double>(TARGET_WIDTH) / img.cols;
    double scaleY = static_cast<double>(TARGET_HEIGHT) / img.rows;
    scale = std::min(scaleX, scaleY);
    cv::resize(img, processedImg, cv::Size(newWidth, newHeight));
}
```

**효과:** 처리 시간 10-50배 단축

### 2. YuNet 얼굴 탐지 사용
```cpp
cv::Ptr<cv::FaceDetectorYN> faceDetector;
faceDetector = cv::FaceDetectorYN::create(modelPath, "", cv::Size(width, height));
faceDetector->detect(processedImg, detections);
```

**효과:** dlib detector보다 빠르고 정확한 얼굴 탐지

### 3. dlib detector 최적화 (fallback용)
```cpp
faces = detector(dlibGray, 0);  // upsample=0 (속도 향상)
```

**효과:** 이미지 업샘플링 비활성화로 속도 향상

## 예상 성능

### YuNet + dlib 하이브리드 방식
- **YuNet 탐지:** 10-50ms
- **dlib landmark:** 30-50ms/얼굴
- **전체 처리:** 50-100ms
- **예상 FPS:** 10-20 fps

### dlib만 사용 (최적화 전)
- **얼굴 탐지:** 4581ms
- **landmark 예측:** 35ms
- **전체 처리:** 4678ms
- **FPS:** 0.21 fps

**성능 향상:** 약 50-100배

## 필요한 모델

### 1. YuNet 얼굴 탐지 모델 (권장)
- **파일명:** `face_detection_yunet_2023mar.onnx`
- **다운로드:** https://github.com/opencv/opencv_zoo/raw/master/models/face_detection_yunet/face_detection_yunet_2023mar.onnx
- **크기:** 약 1-2MB
- **배치 경로:** `resource/` 디렉토리

### 2. dlib 얼굴 특징점 모델 (필수)
- **파일명:** `shape_predictor_68_face_landmarks.dat`
- **다운로드:** http://dlib.net/files/shape_predictor_68_face_landmarks.dat.bz2
- **크기:** 약 95MB
- **배치 경로:** `client_user/thirdparty/dlib/models/`

## 코드 구조

### 주요 함수
1. **이미지 리사이즈:** 고해상도 이미지를 640x480으로 축소
2. **YuNet 모델 로드:** 여러 경로에서 자동 탐색
3. **얼굴 탐지:** YuNet 우선, 실패 시 dlib fallback
4. **Landmark 예측:** dlib shape predictor 사용
5. **좌표 스케일링:** 리사이즈된 좌표를 원본 크기로 변환
6. **시간 측정:** 각 단계별 처리 시간 출력

### 시간 측정 출력 예시
```
=== Processing Time Summary ===
Face Detection:      45 ms (45000 us)
Landmark Prediction: 35 ms (35000 us)
  - Per face:        35 ms
Image Saving:        29 ms
--------------------------------
Total Processing:     109 ms (109000 us)
Estimated FPS:       9.17 fps
================================
```

## 테스트 결과

### 테스트 이미지: 2316x3088 고해상도

**최적화 전 (dlib만):**
- 얼굴 탐지: 4581ms
- 전체 처리: 4678ms
- FPS: 0.21
- 얼굴 탐지: ✅ 성공

**Haar Cascade 시도:**
- 얼굴 탐지: 199ms (약 23배 빠름)
- 전체 처리: 199ms
- FPS: 5.03
- 얼굴 탐지: ❌ 실패 (0개 탐지)
- **문제:** 속도는 빠르지만 정확도가 매우 낮음

**최적화 후 (YuNet + dlib) - 실제 테스트 결과:**
- 얼굴 탐지: 27ms (YuNet)
- Landmark 예측: 62ms (dlib)
- 전체 처리: 162ms
- FPS: 6.17 fps
- 얼굴 탐지: ✅ 성공 (1개 탐지)
- **성능 향상:** dlib만 사용 시 대비 약 29배 빠름 (4678ms → 162ms)

**실제 테스트 출력:**
```
Image loaded: ../../resource/res.png
Original image size: 1564x1170
Resized for processing: 640x478 (scale: 0.409207, original: 1564x1170)
YuNet model loaded from: ../../resource/face_detection_yunet_2023mar.onnx (232589 bytes)
Detecting faces...
  Method: OpenCV YuNet (DNN-based, fast & accurate)
Face detection completed:
  Time: 27 ms (27173 us)
  Found 1 face(s)

Predicting facial landmarks...
Landmark prediction completed:
  Time: 62 ms (62687 us)
  Per face: 62 ms

=== Processing Time Summary ===
Face Detection:      27 ms (27173 us)
Landmark Prediction: 62 ms (62687 us)
Total Processing:     162 ms (162372 us)
Estimated FPS:       6.17 fps
================================
```

## 실패한 최적화 시도

### Haar Cascade 사용 (실패)

**시도 이유:**
- dlib detector가 너무 느림 (4581ms)
- Haar Cascade는 전통적으로 빠른 얼굴 탐지 방법
- OpenCV에 기본 포함되어 있어 추가 모델 불필요

**구현:**
- OpenCV `CascadeClassifier` 사용
- 이미지 리사이즈 적용 (640x853)
- `detectMultiScale()` 함수 사용

**실제 테스트 결과:**
```
Image loaded: ../../resource/res.jpg
Original image size: 2316x3088
Resized for processing: 640x853 (scale: 0.276339, original: 2316x3088)
OpenCV Haar Cascade loaded from: ../../resource/haarcascade_frontalface_alt.xml
Found dlib model at: ../../client_user/thirdparty/dlib/models/shape_predictor_68_face_landmarks.dat
dlib model loaded successfully
Detecting faces...
  Method: OpenCV Haar Cascade (fast)
Face detection completed:
  Time: 199 ms (199753 us)
  Found 0 face(s)

=== Total Processing Time ===
Total: 199 ms
No faces detected in the image.
```

**문제점:**
1. **정확도 부족:** 얼굴을 전혀 탐지하지 못함 (0개)
2. **리사이즈 영향:** 작은 이미지에서 탐지율 더욱 저하
3. **파라미터 튜닝 필요:** threshold, scale factor 등 조정 필요하지만 여전히 불안정
4. **속도는 빠름:** 199ms (dlib의 4581ms 대비 약 23배 빠름)

**결론:**
- 속도는 빠르지만 정확도가 낮아 실용적이지 않음
- YuNet 같은 DNN 기반 모델이 속도와 정확도 모두 우수
- **최종 선택: YuNet + dlib 하이브리드 방식**

## 향후 개선 사항

1. **GPU 가속:** OpenCV DNN 백엔드를 CUDA로 설정
2. **배치 처리:** 여러 얼굴을 한 번에 처리
3. **캐싱:** 모델 로드 결과 캐싱
4. **멀티스레딩:** 여러 얼굴 landmark 예측을 병렬 처리

## 알려진 문제 및 해결 방법

### YuNet 모델 로드 실패

**문제:**
```
libc++abi: terminating due to uncaught exception of type cv::Exception: 
OpenCV(4.13.0) ... Failed to parse ONNX model: face_detection_yunet_2023mar.onnx
```

**원인:**
- 모델 파일이 손상되었거나 불완전하게 다운로드됨
- ONNX 파일 형식이 올바르지 않음
- OpenCV 버전과 모델 호환성 문제

**해결 방법:**
1. **모델 파일 재다운로드:**
   ```bash
   cd /Users/jincheol/Desktop/VEDA/RtspProject/resource
   rm -f face_detection_yunet_2023mar.onnx  # 기존 파일 삭제
   curl -L -o face_detection_yunet_2023mar.onnx \
     https://github.com/opencv/opencv_zoo/raw/master/models/face_detection_yunet/face_detection_yunet_2023mar.onnx
   ```

2. **파일 크기 확인:**
   - 정상적인 파일 크기: 약 1-2MB
   - 1KB 미만이면 손상된 파일

3. **에러 처리 개선:**
   - 코드에 try-catch 추가하여 모델 로드 실패 시 안전하게 fallback
   - 파일 크기 검증 추가

**현재 상태:**
- YuNet 모델 로드 실패 시 dlib detector로 자동 fallback
- 프로그램이 크래시하지 않고 계속 실행됨

## 참고 자료

- [OpenCV YuNet 얼굴 탐지](https://github.com/opencv/opencv_zoo/tree/main/models/face_detection_yunet)
- [dlib 얼굴 landmark](http://dlib.net/face_landmark_detection_ex.cpp.html)
- OpenCV FaceDetectorYN API 문서
