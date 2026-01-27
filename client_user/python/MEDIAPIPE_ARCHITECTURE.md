# MediaPipe Python 임베딩 아키텍처

## 개요

이 프로젝트는 **Python C API**를 사용하여 MediaPipe를 C++ 애플리케이션에 임베딩합니다.
별도 프로세스나 JSON 통신 없이 같은 프로세스 내에서 직접 호출하여 높은 성능을 제공합니다.

## 아키텍처 다이어그램

```
┌─────────────────────────────────────────────────────────────┐
│                    C++ 애플리케이션                          │
│  ┌──────────────────────────────────────────────────────┐  │
│  │              MainWindow (Qt)                         │  │
│  │  - 비디오 프레임 캡처 (OpenCV)                        │  │
│  │  - UI 업데이트                                        │  │
│  └─────────────────┬────────────────────────────────────┘  │
│                    │                                        │
│                    │ processFrame(frame)                    │
│                    ▼                                        │
│  ┌──────────────────────────────────────────────────────┐  │
│  │        MediaPipeProcessor (C++)                      │  │
│  │  - 프레임 큐 관리                                     │  │
│  │  - Python C API 호출                                 │  │
│  │  - 결과 파싱 및 시그널 전송                           │  │
│  └─────────────────┬────────────────────────────────────┘  │
│                    │                                        │
│                    │ Python C API                           │
│                    │ (PyObject_CallObject)                  │
│                    ▼                                        │
│  ┌──────────────────────────────────────────────────────┐  │
│  │     Python Interpreter (같은 프로세스)                │  │
│  │  ┌────────────────────────────────────────────────┐  │  │
│  │  │      mediapipe_module.py                       │  │  │
│  │  │  - initialize(model_path)                      │  │  │
│  │  │  - process_frame(image_array, w, h)            │  │  │
│  │  └─────────────────┬──────────────────────────────┘  │  │
│  │                    │                                    │  │
│  │                    │ MediaPipe API                     │  │
│  │                    ▼                                    │  │
│  │  ┌────────────────────────────────────────────────┐  │  │
│  │  │      MediaPipe FaceLandmarker                  │  │  │
│  │  │  - 얼굴 탐지                                    │  │  │
│  │  │  - 478개 랜드마크 추출                          │  │  │
│  │  │  - 52개 Blendshape 추출                         │  │  │
│  │  └────────────────────────────────────────────────┘  │  │
│  └────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

## 데이터 흐름

### 1. 초기화 단계

```
MainWindow 생성
    ↓
MediaPipeProcessor 생성
    ↓
start() 호출
    ↓
initializePython() - Python 인터프리터 초기화
    ↓
Python 모듈 임포트 (mediapipe_module)
    ↓
initialize(model_path) 호출 - MediaPipe 모델 로드
    ↓
준비 완료
```

### 2. 프레임 처리 단계

```
비디오 프레임 캡처 (30 FPS)
    ↓
processFrame(frame) 호출
    ↓
프레임 큐에 추가 (5프레임마다)
    ↓
processQueuedFrame() - 큐에서 프레임 가져오기
    ↓
cv::Mat → numpy array 변환
    ↓
Python process_frame() 호출
    ↓
MediaPipe 얼굴 탐지 (30-50ms)
    ↓
결과 파싱 (dict → FaceData)
    ↓
faceDetected 시그널 전송
    ↓
MainWindow::onFaceDetected() 호출
    ↓
화면 업데이트 (OpenCV 그리기 + QML 표시)
```

## 주요 컴포넌트

### 1. Python 모듈 (`mediapipe_module.py`)

**역할:**
- MediaPipe FaceLandmarker 초기화 및 관리
- 프레임 처리 및 결과 반환

**함수:**
- `initialize(model_path)`: 모델 초기화
- `process_frame(image_array, width, height)`: 프레임 처리

**입력:**
- numpy array (BGR 형식, shape: (height, width, 3))

**출력:**
- dict: `{"landmarks": [...], "blendshapes": [...]}`
- 또는 None (얼굴 미탐지)

### 2. C++ 클래스 (`MediaPipeProcessor`)

**역할:**
- Python C API를 통한 Python 모듈 호출
- 프레임 큐 관리 및 비동기 처리
- 결과 파싱 및 Qt 시그널 전송

**주요 메서드:**
- `start()`: Python 초기화 및 MediaPipe 모델 로드
- `processFrame(frame)`: 프레임을 큐에 추가
- `processQueuedFrame()`: 큐에서 프레임 처리
- `matToNumpyArray()`: cv::Mat → numpy array 변환
- `parsePythonResult()`: Python dict → FaceData 변환

**시그널:**
- `faceDetected(faces)`: 얼굴 탐지 시 발생
- `errorOccurred(error)`: 오류 발생 시 발생

### 3. 메인 윈도우 (`MainWindow`)

**역할:**
- 비디오 프레임 캡처 및 표시
- MediaPipeProcessor와 통신
- 얼굴 데이터를 화면에 렌더링

**주요 메서드:**
- `updateVideoFrame()`: 비디오 프레임 업데이트
- `onFaceDetected()`: 얼굴 탐지 결과 처리

## 성능 특성

### 처리 속도
- **MediaPipe 얼굴 탐지**: ~30-50ms (5프레임마다 처리)
- **프레임 표시**: 실시간 (30 FPS)
- **Python 호출 오버헤드**: <1ms (같은 프로세스 내)

### 메모리
- **프레임 큐**: 최대 5개 프레임 (오래된 프레임 자동 제거)
- **numpy array**: 데이터 복사 (메모리 안전성 보장)
- **Python 객체**: 자동 참조 카운팅 관리

## 장점

1. **높은 성능**
   - 프로세스 간 통신 오버헤드 없음
   - JSON 파싱 불필요
   - 직접 메모리 접근

2. **간단한 통합**
   - Qt 시그널/슬롯으로 비동기 처리
   - 자동 메모리 관리 (Python 참조 카운팅)

3. **유연성**
   - Python 코드 수정 시 재컴파일 불필요
   - MediaPipe 업데이트 시 Python 패키지만 업데이트

## 주의사항

1. **GIL (Global Interpreter Lock)**
   - Python 함수 호출 시 GIL 획득 필요
   - `PyGILState_Ensure()` / `PyGILState_Release()` 사용

2. **메모리 관리**
   - Python 객체는 참조 카운팅으로 관리
   - `Py_DECREF()`로 명시적 해제 필요

3. **venv 경로**
   - venv의 site-packages를 sys.path에 추가해야 함
   - Python 버전에 따라 경로가 달라질 수 있음

## 사용 예제

### C++에서 사용

```cpp
// MediaPipeProcessor 생성
MediaPipeProcessor *processor = new MediaPipeProcessor(this);
processor->setProcessInterval(5);  // 5프레임마다 처리

// 시그널 연결
connect(processor, &MediaPipeProcessor::faceDetected,
        this, &MainWindow::onFaceDetected);

// 시작
processor->start();

// 프레임 처리
cv::Mat frame = ...;
processor->processFrame(frame);
```

### Python 모듈 직접 테스트

```python
import numpy as np
import cv2
from mediapipe_module import initialize, process_frame

# 초기화
initialize("path/to/face_landmarker.task")

# 이미지 로드
img = cv2.imread("test.jpg")
h, w = img.shape[:2]

# 처리
result = process_frame(img, w, h)

if result:
    print(f"Landmarks: {len(result['landmarks'])}")
    print(f"Blendshapes: {len(result['blendshapes'])}")
```

## 문제 해결

### Python 모듈을 찾을 수 없음
- venv의 site-packages 경로 확인
- Python 버전 확인 (3.10 이상)

### MediaPipe 모델을 찾을 수 없음
- `face_landmarker.task` 파일 경로 확인
- 절대 경로로 지정 가능

### GIL 관련 오류
- `PyGILState_Ensure()` / `PyGILState_Release()` 쌍 확인
- 모든 Python 호출 전후에 GIL 관리
