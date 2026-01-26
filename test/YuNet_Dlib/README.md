# dlib 얼굴 탐지 테스트

이 디렉토리는 YuNet + dlib을 사용한 얼굴 탐지 및 landmark 예측 기능을 테스트하는 프로그램입니다.

## 빌드 방법

### 기본 빌드
```bash
cd /Users/jincheol/Desktop/VEDA/RtspProject/test
mkdir build
cd build
cmake ..
make
```

### Haar Cascade 활성화하여 빌드 (사용 안 함)
```bash
cd /Users/jincheol/Desktop/VEDA/RtspProject/test
mkdir build
cd build
cmake -DUSE_HAAR_CASCADE=ON ..
make
```

## 사용 방법

```bash
# 기본 사용 (출력 파일: output_detected.jpg)
./bin/test_dlib_face <이미지_경로>

# 출력 파일 지정
./bin/test_dlib_face <이미지_경로> <출력_경로>

# 예시
./bin/test_dlib_face ../../resource/res.jpg result.jpg
```

## 필요한 파일

### 1. YuNet 얼굴 탐지 모델 (권장)
- `face_detection_yunet_2023mar.onnx`: OpenCV YuNet 얼굴 탐지 모델
  - 다운로드: https://github.com/opencv/opencv_zoo/raw/master/models/face_detection_yunet/face_detection_yunet_2023mar.onnx
  - 다음 경로 중 하나에 배치:
    - `../../resource/`
    - `../resource/`
    - `/Users/jincheol/Desktop/VEDA/RtspProject/resource/`
    - 현재 디렉토리

### 2. dlib 얼굴 특징점 모델 (필수)
- `shape_predictor_68_face_landmarks.dat`: dlib 얼굴 특징점 모델 파일
  - 다운로드: http://dlib.net/files/shape_predictor_68_face_landmarks.dat.bz2
  - 압축 해제 후 다음 경로 중 하나에 배치:
    - `../client_user/thirdparty/dlib/models/`
    - `../../client_user/thirdparty/dlib/models/`
    - `/usr/local/share/dlib/`
    - `/opt/homebrew/share/dlib/`

**참고**: YuNet 모델이 없으면 dlib detector를 fallback으로 사용합니다.

## 동작 방식

1. **YuNet으로 얼굴 탐지** (빠르고 정확)
2. **dlib shape predictor로 68개 landmark 예측** (정확한 특징점)

이 하이브리드 방식으로 실시간 처리에 최적화되어 있습니다.

## 출력

- 탐지된 얼굴은 초록색 사각형으로 표시됩니다
- 얼굴 특징점(68개)은 빨간색 점으로 표시됩니다
- 결과 이미지가 저장되고 화면에 표시됩니다
- 각 단계별 처리 시간이 출력됩니다
