# Client User 개발 일지

## 개발 일자
- 2026년 1월 23일: 초기 프로젝트 구조 및 OpenGL 렌더링 파이프라인 구현
- 2026년 1월 26일: MediaPipe Python 임베딩 및 실시간 얼굴 탐지 구현

## 개발 목표
RTSP 서버로부터 전송되는 영상 스트림을 받아서 실시간으로 얼굴을 인식하고, OpenGL을 사용하여 얼굴 위에 3D 아바타를 렌더링하는 클라이언트 프로그램 개발

## 현재 문제점 및 개선 필요 사항

### 1. dlib 얼굴 탐지 성능 문제
**문제:**
- dlib detector가 고해상도 이미지에서 매우 느림
- 테스트 결과: 2316x3088 이미지에서 4581ms 소요 (0.21 fps)
- 실시간 비디오 스트림 처리에 부적합
- 현재 별도 스레드로 분리했지만, 여전히 느린 탐지로 인한 지연 가능

**해결 방안:**
1. **이미지 리사이즈:** 처리 전에 640x480으로 축소 (10-50배 속도 향상)
2. **YuNet 도입:** OpenCV YuNet 얼굴 탐지 모델 사용 (더 빠르고 정확)
3. **하이브리드 방식:** YuNet으로 탐지 + dlib으로 landmark만 예측
4. **dlib detector 최적화:** `upsample=0` 설정으로 속도 향상

**참고:** `test/test_dev.md`에 상세한 최적화 방법과 성능 비교가 문서화되어 있음

### 2. 프레임 처리 속도
**현재 상태:**
- dlib 탐지가 별도 스레드에서 실행되지만, 여전히 느림
- 10프레임마다 처리하도록 설정했지만, 각 처리 시간이 길어 지연 발생 가능
- 고해상도 웹캠 프레임(1280x720 등)에서 성능 저하

**현재 구현:**
- 별도 스레드로 dlib 처리 분리 (`dlibProcessingThread`)
- 프레임 큐를 통한 비동기 처리
- 10프레임마다 처리 (`dlibProcessInterval = 10`)

**개선 필요:**
- YuNet으로 얼굴 탐지 속도 향상 (10-50ms)
- 이미지 리사이즈로 처리 시간 단축
- 예상 성능: 10-20 fps 달성 가능

### 3. 성능 측정 부재
**문제:**
- 현재 프레임 처리 시간을 측정하지 않음
- 실제 FPS를 모니터링하지 않음
- 성능 병목 지점 파악 어려움

**개선 필요:**
- 각 단계별 처리 시간 측정 추가
- FPS 모니터링 및 표시
- 성능 로그 출력

## 오늘 개발한 내용

### 1. 프로젝트 구조 생성
- `client_user/` 디렉토리 생성
- `client_user/src/main.cpp` - 메인 프로그램
- `client_user/CMakeLists.txt` - 빌드 설정 파일
- `resource/` 폴더 추가 (렌더링 관련 리소스 저장용)

### 2. OpenGL 렌더링 파이프라인 구현
- **GLFW 윈도우 생성**: 1280x720 해상도
- **GLEW 초기화**: OpenGL 확장 로더
- **셰이더 프로그램**: 
  - Vertex Shader: 화면 전체를 덮는 쿼드 렌더링
  - Fragment Shader: 비디오 텍스처 샘플링
- **VAO/VBO/EBO 설정**: 비디오 프레임 렌더링을 위한 버퍼 설정
- **텍스처 업로드**: OpenCV Mat을 OpenGL 텍스처로 변환

### 3. 얼굴 인식 기능 구현
- **OpenCV Haar Cascade 사용**: `haarcascade_frontalface_alt.xml` 모델 로드
- **실시간 얼굴 검출**: 
  - 그레이스케일 변환
  - 히스토그램 균등화
  - `detectMultiScale()` 함수 사용
- **바운딩 박스 표시**: 
  - OpenCV `cv::rectangle()` 사용하여 빨간색 박스 그리기
  - 두께 3픽셀
  - BGR 형식 (0, 0, 255) = 빨간색
- **검출 정보 출력**: 콘솔에 얼굴 검출 개수 출력 (30프레임마다)

### 4. 비디오 소스 처리
- **로컬 비디오 모드** (`USE_LOCAL_VIDEO = 1`):
  - MacBook 내장 카메라 지원 (인덱스 0)
  - 비디오 파일 지원 (H.264 등)
  - 카메라 해상도 설정 (1280x720)
- **서버 스트림 모드** (`USE_LOCAL_VIDEO = 0`):
  - TODO: RTSP 서버 스트림 연동 (향후 구현 예정)

### 5. 리소스 파일 경로 처리
- **다중 경로 지원**: 여러 가능한 경로에서 자동으로 리소스 파일 찾기
  - `../resource/` (build/bin에서 실행 시)
  - `../../resource/` (build에서 실행 시)
  - `resource/` (프로젝트 루트에서 실행 시)
  - 절대 경로 (백업)
- **에러 메시지 개선**: 모든 시도한 경로를 표시하여 디버깅 용이

### 6. 빌드 시스템 구성
- **CMakeLists.txt 작성**:
  - OpenCV, OpenGL, GLFW, GLEW 의존성 설정
  - Mac M1 (Apple Silicon) 지원
  - Ubuntu 지원
- **플랫폼별 경로 처리**:
  - Mac M1: `/opt/homebrew` 경로 자동 검색
  - `/usr/local`의 오래된 OpenCV 설정 무시
  - Homebrew prefix 자동 감지

### 7. 문서화
- `docs/client_user.md`: 개발 가이드 및 구현 단계 문서
- `resource/README.md`: 리소스 폴더 설명

## 기술 스택

### 사용된 라이브러리
- **OpenCV 4.13.0**: 비디오 처리 및 얼굴 인식
- **OpenGL 3.3 Core Profile**: 3D 렌더링
- **GLFW**: 윈도우 관리 및 이벤트 처리
- **GLEW**: OpenGL 확장 로더

### 개발 환경
- **플랫폼**: Mac M1 (Apple Silicon)
- **컴파일러**: AppleClang 17.0.0
- **C++ 표준**: C++17

## 현재 구현 상태

### ✅ 완료된 기능
1. OpenGL 윈도우 생성 및 초기화
2. 비디오 프레임 캡처 (카메라/비디오 파일)
3. 비디오 프레임을 OpenGL 텍스처로 렌더링
4. 실시간 얼굴 인식 (Haar Cascade)
5. 얼굴 바운딩 박스 표시 (빨간색 사각형)
6. 얼굴 검출 정보 콘솔 출력
7. Mac M1 및 Ubuntu 빌드 지원
8. 리소스 파일 자동 경로 검색

### 🔄 진행 중
- 기본 렌더링 파이프라인 안정화
- 성능 최적화

### ⏳ 예정된 기능
1. **3D 아바타 렌더링** (Phase 3)
   - 3D 모델 로더 구현
   - 얼굴 위치에 아바타 배치
   - 얼굴 크기에 맞춰 스케일 조정
   - 텍스처 매핑 및 조명 처리

2. **RTSP 서버 스트림 연동** (Phase 4)
   - RTSP 클라이언트 구현
   - H.264 스트림 수신 및 디코딩
   - 네트워크 오류 처리 및 재연결 로직

3. **성능 최적화** (Phase 5)
   - 멀티스레딩을 통한 렌더링 성능 향상
   - 얼굴 인식 정확도 개선
   - 아바타 애니메이션 추가

## 주요 코드 구조

### 클래스: FaceAvatarRenderer
```cpp
class FaceAvatarRenderer {
    // OpenGL 관련
    GLFWwindow* window;
    GLuint textureID, shaderProgram;
    GLuint VAO, VBO, EBO;
    
    // OpenCV 관련
    cv::CascadeClassifier faceCascade;
    cv::VideoCapture videoCapture;
    
    // 얼굴 검출 데이터
    std::vector<AvatarData> detectedFaces;
    
    // 주요 메서드
    bool initialize();           // 초기화
    void setupOpenGL();         // OpenGL 설정
    void detectFaces();         // 얼굴 인식
    void renderVideoFrame();    // 비디오 프레임 렌더링
    void renderAvatars();       // 아바타 렌더링 (향후 구현)
    void run();                 // 메인 루프
    void cleanup();             // 리소스 정리
};
```

## 빌드 및 실행

### 빌드
```bash
cd client_user
mkdir build
cd build
cmake ..
make
```

### 실행
```bash
cd build/bin
./client_user
```

### 실행 전 확인 사항
1. `resource/haarcascade_frontalface_alt.xml` 파일 존재 확인
2. 카메라 권한 확인 (macOS)
3. 비디오 파일 경로 확인 (비디오 파일 사용 시)

## 테스트 결과

### 성공한 기능
- ✅ OpenGL 윈도우 생성
- ✅ 비디오 프레임 렌더링
- ✅ 얼굴 인식 및 바운딩 박스 표시
- ✅ Mac M1 빌드 성공

### 테스트 환경
- **비디오 소스**: 
  - MacBook 내장 카메라
  - H.264 비디오 파일 (`hitto.h264`)
- **해상도**: 1280x720
- **프레임레이트**: ~30 FPS

## 알려진 이슈

1. **OpenGL 3.3 Core Profile 호환성**
   - `glBegin/glEnd` 사용 불가 (이미 제거됨)
   - 바운딩 박스는 OpenCV로 직접 그리도록 변경

2. **리소스 파일 경로**
   - 실행 위치에 따라 경로가 달라질 수 있음
   - 다중 경로 검색으로 해결

3. **카메라 권한**
   - macOS에서 처음 실행 시 권한 요청 필요
   - 시스템 설정에서 수동으로 권한 부여 가능

## 다음 개발 계획

### 단기 (다음 세션)
1. 3D 아바타 모델 로더 구현
2. 기본 3D 아바타 렌더링 (간단한 큐브나 구체)
3. 얼굴 위치에 아바타 배치

### 중기
1. RTSP 서버 스트림 연동
2. 네트워크 오류 처리
3. 성능 최적화

### 장기
1. 더 정교한 얼굴 인식 모델 (DNN 기반)
2. 아바타 애니메이션
3. 멀티 얼굴 지원
4. UI 개선

## 참고 자료

- OpenCV 공식 문서: https://docs.opencv.org/
- OpenGL 튜토리얼: https://learnopengl.com/
- GLFW 문서: https://www.glfw.org/docs/latest/
- 프로젝트 문서: `docs/client_user.md`

## 개발 노트

### 2026-01-23 주요 변경사항
- 초기 프로젝트 구조 생성
- OpenGL 렌더링 파이프라인 구현
- 얼굴 인식 기능 구현
- 바운딩 박스 표시 기능 추가
- Mac M1 빌드 지원 완료
- 리소스 파일 경로 처리 개선

### 기술적 결정사항
1. **OpenCV로 바운딩 박스 그리기**: OpenGL 3.3 Core Profile의 제약을 피하기 위해 OpenCV의 `cv::rectangle()` 사용
2. **다중 경로 리소스 검색**: 실행 위치에 독립적으로 작동하도록 여러 경로 시도
3. **Homebrew 자동 감지**: Mac M1에서 OpenCV 경로를 자동으로 찾도록 구현

---

## 2026년 1월 26일 개발 내용

### 주요 성과
✅ **MediaPipe Python 임베딩 구현 완료**
- Python C API를 사용하여 같은 프로세스 내에서 MediaPipe 직접 호출
- JSON 파싱 없이 직접 값으로 반환 (성능 향상)
- venv 경로 자동 감지 및 Python 경로 설정
- 실시간 얼굴 탐지 및 화면 표시 구현

### 구현된 기능

#### 1. Python 사이드카 패턴 → Python C API 임베딩으로 전환
**초기 계획:**
- Python 프로세스를 별도로 실행하고 JSON으로 통신하는 사이드카 패턴

**최종 구현:**
- Python C API를 사용하여 같은 프로세스 내에서 직접 호출
- `QProcess` 대신 `PyObject`로 직접 함수 호출
- JSON 파싱 오버헤드 제거
- 프로세스 간 통신 오버헤드 제거

**장점:**
- 더 빠른 통신 (프로세스 간 통신 없음)
- 메모리 효율적 (cv::Mat을 numpy array로 직접 변환)
- 간단한 에러 처리 (Python 예외 직접 처리)

#### 2. MediaPipe 얼굴 탐지 및 Blendshape 추출
**구현 내용:**
- MediaPipe FaceLandmarker 모델 사용
- 478개 얼굴 랜드마크 추출 (정규화된 좌표 0-1)
- 52개 Blendshape 값 추출 (눈 깜빡임, 입 벌림, 표정 등)
- 5프레임마다 비동기 처리 (설정 가능)

**Python 모듈 (`mediapipe_module.py`):**
```python
def initialize(model_path) -> bool
def process_frame(image_array, width, height) -> dict | None
```

**C++ 클래스 (`MediaPipeProcessor`):**
- `start()`: Python 초기화 및 MediaPipe 모델 로드
- `processFrame(frame)`: 프레임을 큐에 추가하여 비동기 처리
- `faceDetected` 시그널: 얼굴 탐지 결과 전달

#### 3. venv 경로 자동 감지
**문제:**
- 시스템 Python을 사용하면 venv에 설치된 mediapipe를 찾을 수 없음

**해결:**
- Python 버전을 동적으로 감지 (`Py_GetVersion()`)
- 여러 Python 버전 경로 시도 (3.14, 3.13, 3.12 등)
- venv의 site-packages 경로를 `sys.path`에 자동 추가
- 상대/절대 경로 모두 지원

#### 4. 실시간 얼굴 표시
**OpenCV로 프레임에 직접 그리기:**
- 478개 랜드마크를 초록색 점으로 표시
- 얼굴 영역을 노란색 박스로 표시

**QML에서 얼굴 영역 표시:**
- 정규화된 좌표(0-1)로 얼굴 위치 계산
- 반투명 Rectangle로 얼굴 영역 표시
- `faceWidth > 0 && faceHeight > 0`일 때만 표시

**데이터 흐름:**
```
MediaPipe Python → FaceData 구조체 → MainWindow::onFaceDetected()
→ VideoQuick3DWidget::setFaceData() → QML property 업데이트
→ 화면에 실시간 표시
```

### 기술 스택 추가

#### Python 관련
- **Python3 C API**: Python 임베딩
- **NumPy**: cv::Mat ↔ numpy array 변환
- **MediaPipe**: 얼굴 탐지 및 Blendshape 추출

#### CMake 설정
- `find_package(Python3 COMPONENTS Interpreter Development NumPy REQUIRED)`
- 시스템 Python 사용 (venv는 site-packages만 추가)
- NumPy 헤더 경로 자동 감지

### 해결한 문제들

#### 1. Python.h와 numpy 헤더 충돌
**문제:**
- `PyType_Slot` 중복 정의 오류
- 헤더 포함 순서 문제

**해결:**
- Python.h를 다른 모든 헤더보다 먼저 포함
- numpy 헤더는 Python.h 이후에 포함
- 헤더 파일에서는 전방 선언만 사용

#### 2. venv의 mediapipe 모듈을 찾을 수 없음
**문제:**
- 시스템 Python을 사용하여 venv의 패키지를 찾지 못함

**해결:**
- venv의 site-packages 경로를 `sys.path`에 동적으로 추가
- Python 버전을 자동으로 감지하여 경로 생성
- 여러 가능한 경로를 순차적으로 시도

#### 3. 얼굴 탐지는 되지만 화면에 표시되지 않음
**문제:**
- `onFaceDetected()`에서 데이터를 받지만 화면에 그리지 않음

**해결:**
- 랜드마크에서 얼굴 영역 계산 (min/max 좌표)
- OpenCV로 프레임에 직접 그리기
- `VideoQuick3DWidget::setFaceData()`로 QML에 전달
- QML에서 `faceWidth > 0` 조건으로 표시

### 성능

**처리 속도:**
- MediaPipe 얼굴 탐지: ~30-50ms (5프레임마다 처리)
- 프레임 표시: 실시간 (30 FPS)
- Blendshape 추출: MediaPipe 내부 처리 (추가 오버헤드 없음)

**메모리:**
- numpy array로 직접 변환 (복사 최소화)
- 큐 크기 제한 (최대 5개 프레임)

### 파일 구조

```
client_user/
├── python/
│   ├── mediapipe_module.py      # MediaPipe Python 모듈
│   ├── mediapipe_processor.py   # (구버전, 사용 안 함)
│   ├── requirements.txt          # Python 의존성
│   └── README_EMBEDDED.md       # Python 임베딩 사용법
├── src/
│   ├── MediaPipeProcessor.h     # MediaPipe 프로세서 헤더
│   └── MediaPipeProcessor.cpp    # Python C API 구현
└── thirdparty/
    └── mediapipe/
        └── models/
            └── face_landmarker.task  # MediaPipe 모델 파일
```

### 다음 단계

1. **Blendshape를 활용한 아바타 제어**
   - 눈 깜빡임 (`eyeBlinkLeft`, `eyeBlinkRight`)
   - 입 벌림 (`jawOpen`)
   - 표정 변화 (`mouthSmileLeft`, `mouthSmileRight` 등)

2. **성능 최적화**
   - 처리 간격 조정 (현재 5프레임마다)
   - 이미지 리사이즈 옵션 추가
   - 멀티스레딩 최적화

3. **UI 개선**
   - Blendshape 값 실시간 표시
   - 얼굴 탐지 상태 표시
   - 처리 FPS 표시

### 참고 자료
- MediaPipe Face Landmarker: https://developers.google.com/mediapipe/solutions/vision/face_landmarker
- Python C API: https://docs.python.org/3/c-api/
- NumPy C API: https://numpy.org/doc/stable/reference/c-api/
