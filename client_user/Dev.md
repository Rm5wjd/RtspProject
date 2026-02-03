# Client User 개발 일지

## 개발 일자
- 2026년 1월 23일: 초기 프로젝트 구조 및 OpenGL 렌더링 파이프라인 구현
- 2026년 1월 26일: MediaPipe Python 임베딩 및 실시간 얼굴 탐지 구현
- 2026년 1월 29일: Qt Quick3D 기반 3D 아바타 렌더링 및 Blendshape 매핑 완료
- 2026년 1월 30일: **Unity로 전환 결정** (Qt Quick3D의 Morph Target 제한 문제)

## 개발 목표
RTSP 서버로부터 전송되는 영상 스트림을 받아서 실시간으로 얼굴을 인식하고, **Unity를 사용하여** 얼굴 위에 3D 아바타를 렌더링하는 클라이언트 프로그램 개발

## 현재 아키텍처 (Unity 전환)

### 전체 구조
```
Qt Client (C++)
  ├─ 카메라/비디오 소스 관리 (MainWindow)
  ├─ MediaPipe (Python) → 52개 Blendshape 추출
  ├─ Unity 프로세스 (별도 실행)
  │   ├─ MediaPipe 데이터 수신 (TCP/UDP)
  │   ├─ GLB 모델 로드 및 52개 BlendShape 적용
  │   └─ 렌더링된 이미지를 네트워크로 전송
  └─ Qt에서 Unity 렌더링 이미지를 QML에 표시
```

### 왜 Unity로 전환했나?

#### Qt Quick3D의 한계
- **Morph Target 제한**: GPU Attribute 슬롯이 최대 8~16개로 제한됨
- **52개 Blendshape 불가**: MediaPipe는 52개 Blendshape를 제공하지만, Qt Quick3D는 8개만 지원
- **결과**: 눈을 깜빡였는데 입이 벌어지는 등 잘못된 매핑 발생
- **에러**: `exceeds 8 attributes` 오류 발생

#### Unity의 장점
- **52개 BlendShape 완전 지원**: Unity는 BlendShape를 제한 없이 지원
- **고성능 렌더링**: Unity의 최적화된 렌더링 파이프라인
- **표준 GLB 지원**: GLB 파일의 모든 BlendShape를 정확히 매핑

## 현재 구현 상태

### ✅ 완료된 기능
1. **MediaPipe 기반 얼굴 탐지 및 Blendshape 추출**
   - pybind11로 Python MediaPipe 임베딩
   - 478개 랜드마크 + 52개 Blendshape 실시간 추출
   - 5프레임마다 비동기 처리
   - **카메라는 MainWindow에서만 관리, MediaPipeProcessor는 프레임 데이터만 받아서 처리**

2. **다중 모드 지원**
   - 웹캠 모드 (기본, 자동 시작)
   - UDP 서버 연결 (포트 번호 입력)
   - RTSP 스트림 연결 (URL 입력)
   - 모든 모드에서 MediaPipe 자동 시작

3. **실시간 비디오 표시**
   - QML Image Provider로 프레임 전달
   - 30 FPS 실시간 렌더링

### 🔄 진행 중 (Unity 전환)
1. **Unity 프로젝트 구조 생성**
   - MediaPipe 데이터 수신 스크립트 (`MediaPipeReceiver.cs`)
   - BlendShape 적용 스크립트 (`BlendShapeController.cs`)
   - 렌더링 결과 출력 스크립트 (`RenderTextureExporter.cs`)

2. **Qt ↔ Unity 통신**
   - MediaPipe Blendshape 데이터를 Unity로 전송 (TCP/UDP)
   - Unity 렌더링 이미지를 Qt로 수신하여 표시

### ❌ 중단된 기능 (Qt Quick3D)
- **Qt Quick3D 기반 3D 아바타 렌더링**: Morph Target 제한으로 인해 8개만 지원 가능
- **View3D.qml**: 현재는 사용하지 않음 (Unity 전환으로 인해)
- **참고**: 코드는 남아있지만, Unity 전환으로 인해 비활성화됨

### 🔄 개선 가능 사항
1. **Blendshape 튜닝**
   - 각 Blendshape의 weight 범위 최적화
   - 주요 표정의 반응성 향상

2. **성능 최적화**
   - 처리 간격 조정 (현재 5프레임마다)
   - 필요 시 입력 프레임 리사이즈 옵션 추가

3. **UI 개선**
   - Blendshape 값 실시간 표시 (디버그용)
   - 얼굴 탐지 상태 및 FPS 표시
   - 캐릭터 선택 UI 개선

## 2026년 1월 23일 개발 내용

### 초기 프로젝트 구조 생성
- `client_user/` 디렉토리 생성
- Qt 기반 UI 구조 설계
- OpenCV 비디오 캡처 통합

## 기술 스택

### 사용된 라이브러리
- **Qt6**: Widgets, Quick, QuickWidgets, Quick3D
- **OpenCV 4.13.0**: 비디오 처리 및 캡처
- **pybind11**: Python C++ 임베딩
- **MediaPipe (Python)**: 얼굴 탐지 및 Blendshape 추출
- **NumPy (Python)**: 이미지 데이터 변환

### 개발 환경
- **플랫폼**: Mac M1 (Apple Silicon)
- **컴파일러**: AppleClang 17.0.0
- **C++ 표준**: C++17
- **Python**: 3.14 (venv)

## 주요 코드 구조

### 클래스: MediaPipeProcessor
```cpp
class MediaPipeProcessor : public QObject {
    // pybind11 기반 Python 임베딩
    py::scoped_interpreter interpreter;
    py::object process_frame_func;
    
    // 비동기 프레임 처리
    QTimer *processTimer;
    std::queue<cv::Mat> frameQueue;
    
    // 시그널
    void faceDetected(QVector<FaceData>);
    void errorOccurred(QString);
};
```

### 클래스: VideoQuick3DWidget
```cpp
class VideoQuick3DWidget : public QQuickWidget {
    // QML property
    Q_PROPERTY(int avatarIndex ...)
    Q_PROPERTY(QString glbModelPath ...)
    Q_PROPERTY(QList<qreal> blendshapes ...)
    
    // 얼굴 위치/크기
    Q_PROPERTY(double faceX ...)
    Q_PROPERTY(double faceY ...)
    Q_PROPERTY(double faceWidth ...)
    Q_PROPERTY(double faceHeight ...)
};
```

## 알려진 이슈

1. **OpenCV/FFmpeg 중복 심볼 경고**
   - Homebrew OpenCV/FFmpeg와 venv 내 `opencv-python`이 동시에 로드될 때 macOS에서 클래스 중복 경고 발생
   - 현재는 기능에 큰 문제는 없으나, 필요시 시스템 OpenCV만 사용하거나 venv opencv를 제거하는 방향으로 정리 예정

2. **Qt Quick3DTools 의존성**
   - CMake에서 Quick3D를 찾을 때 Quick3DTools 의존성 오류 발생 가능
   - 현재는 런타임 로드 방식으로 해결 (QML 엔진이 자동으로 찾음)

3. **GLB 모델 Morph Target 이름**
   - Avatar02.glb의 Morph Target 이름이 MediaPipe Blendshape 이름과 정확히 일치해야 함
   - Blender에서 Shape Key 이름을 확인하여 매핑 필요

---

## 2026년 1월 26일 개발 내용

### 주요 성과
✅ **MediaPipe Python 임베딩 구현 완료 (pybind11 embed)**
- pybind11 `scoped_interpreter`로 같은 프로세스 내에서 Python 런타임 임베딩
- `mediapipe_module.py`에서 MediaPipe FaceLandmarker를 직접 호출
- QProcess/JSON 없이 C++에서 바로 Python 함수를 호출 (성능 및 구조 단순화)
- venv 경로 자동 감지 및 Python 경로 설정
- 실시간 얼굴 탐지 + Blendshape 추출 + 화면 표시 구현

### 구현된 기능

#### 1. Python 사이드카 패턴 → pybind11 기반 임베딩으로 전환
**초기 계획:**
- Python 프로세스를 별도로 실행하고 JSON으로 통신하는 사이드카 패턴 (`mediapipe_processor.py` + QProcess)

**최종 구현:**
- pybind11 `scoped_interpreter`를 사용하여 같은 프로세스 내에서 Python을 직접 임베딩
- `mediapipe_module.py`의 `initialize()` / `process_frame()`을 C++에서 바로 호출
- QProcess/JSON 파이프 통신 제거 → 구조 단순화, 디버깅 용이
- 프로세스 간 통신/직렬화 오버헤드 제거

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
- `start()`: 임베디드 Python 초기화 및 MediaPipe 모델 로드 (한 번만)
- `processFrame(frame)`: 프레임을 큐에 추가하여 비동기 처리 (N프레임마다)
- `faceDetected` 시그널: 얼굴 탐지 결과(랜드마크 + Blendshape) 전달
- `errorOccurred` 시그널: Python 예외/모듈 로드 실패 등 에러 전달

#### 3. venv 경로 자동 감지
**문제:**
- 시스템 Python을 사용하면 venv에 설치된 mediapipe를 찾을 수 없음

**해결:**
- Python 버전을 동적으로 감지 (`Py_GetVersion()`)
- 여러 Python 버전 경로 시도 (3.14, 3.13, 3.12 등)
- venv의 site-packages 경로를 `sys.path`에 자동 추가
- 상대/절대 경로 모두 지원

#### 4. 실시간 얼굴/아바타 표시
**OpenCV로 프레임에 직접 그리기 (디버그용):**
- MediaPipe 랜드마크를 초록색 점으로 표시
- 얼굴 영역을 노란색 박스로 표시

**QML에서 2D/3D 오버레이:**
- 정규화된 좌표(0-1)로 얼굴 위치/크기 계산 → `faceX`, `faceY`, `faceWidth`, `faceHeight`
- `VideoScene.qml`에서 반투명 Rectangle로 얼굴 영역 표시 (`faceWidth > 0 && faceHeight > 0`일 때)
- `View3D.qml`에서 `Avatar02.glb`를 로드하여 얼굴 위치/크기에 맞게 배치
- MediaPipe Blendshape(52개)를 `blendshapes` 배열로 넘겨, `MorphTarget` weight로 매핑 (눈 깜빡임, 입 벌림, 미소 등)

**데이터 흐름:**
```
MediaPipe (Python) → dict(landmarks, blendshapes)
    ↓ pybind11
MediaPipeProcessor::FaceData (C++)
    ↓ 시그널
MainWindow::onFaceDetected()
    ↓
VideoQuick3DWidget::setFaceData(), setBlendshapes()
    ↓
QML (VideoScene.qml, View3D.qml)에서 오버레이 + 3D 아바타 렌더링
```

### 기술 스택 추가

#### Python 관련
- **pybind11 (embed)**: Python 임베딩 및 C++ ↔ Python 호출
- **NumPy (Python)**: `mediapipe_module.py`에서 cv::Mat ↔ numpy array 변환 담당
- **MediaPipe (Python)**: 얼굴 탐지 및 Blendshape 추출

#### Qt Quick3D 관련
- **Qt Quick3D**: 3D 아바타 렌더링 (GLB 모델 로드, Morph Target 지원)
- **QML Loader**: 런타임에 View3D.qml 동적 로드
- **Homebrew qtquick3d**: `/opt/homebrew/Cellar/qtquick3d/6.10.1/share/qt/qml` 경로 자동 감지

#### 빌드/런타임
- pybind11는 헤더 전용 라이브러리로 포함 (`<pybind11/embed.h>`, `<pybind11/numpy.h>`)
- 런타임에는 Python 3 + `mediapipe`, `opencv-python`, `numpy` 패키지가 필요
- Qt Quick3D는 런타임에 QML 엔진이 자동으로 로드 (CMake 링크 불필요)

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

### 파일 구조 (MediaPipe 관련)

```
client_user/
├── python/
│   ├── mediapipe_module.py       # MediaPipe Python 모듈 (FaceLandmarker + Blendshape)
│   ├── mediapipe_processor.py    # (구버전, QProcess JSON 방식 – 참고용)
│   ├── requirements.txt          # Python 의존성 (mediapipe, opencv-python, numpy)
│   └── MEDIAPIPE_ARCHITECTURE.md # 임베딩 아키텍처 문서
├── src/
│   ├── MediaPipeProcessor.h      # pybind11 기반 MediaPipe 브리지 (C++)
│   ├── MediaPipeProcessor.cpp     # Python 임베딩 + 프레임 큐 + 결과 파싱
│   ├── VideoQuick3DWidget.*      # QQuickWidget + QML, 얼굴 위치/Blendshape 전달
│   └── MainWindow.*              # 비디오 캡처, 모드 전환, MediaPipeProcessor 연동
├── qml/
│   ├── VideoScene.qml            # 비디오 배경 + View3D Loader
│   └── View3D.qml                # Qt Quick3D 아바타 렌더링 + MorphTarget 매핑
└── thirdparty/
    └── mediapipe/
        └── models/
            └── face_landmarker.task  # MediaPipe FaceLandmarker 모델 파일
```

---

## 2026년 1월 29일 개발 내용

### 주요 성과
✅ **Qt Quick3D 기반 3D 아바타 렌더링 구현 완료**
- Qt Quick3D를 사용하여 GLB 모델(`Avatar02.glb`) 로드 및 렌더링
- MediaPipe Blendshape 52개를 Avatar02.glb의 Morph Target에 정확히 매핑
- 웹캠/RTSP/UDP 모든 모드에서 MediaPipe 자동 시작
- Qt Quick3D import 경로 자동 감지 및 설정

### 구현된 기능

#### 1. Qt Quick3D 통합
**구현 내용:**
- `View3D.qml`에서 `Model` 컴포넌트로 GLB 파일 로드
- `MorphTarget`을 사용하여 Blendshape 값을 모델의 표정에 반영
- 카메라/조명 설정으로 3D 씬 구성
- 비디오 Image 위에 3D 모델 오버레이 (`z-order` 조정)

**해결한 문제:**
- Qt Quick3D 모듈을 찾지 못하는 문제 → Homebrew 설치 경로 자동 감지
- Quick3DTools 의존성 오류 → 런타임 로드 방식으로 변경
- QML import 경로 설정 → `VideoQuick3DWidget`에서 동적으로 추가

#### 2. MediaPipe Blendshape → Avatar02.glb 매핑
**구현 내용:**
- MediaPipe의 52개 Blendshape를 정확한 인덱스로 매핑
- `View3D.qml`의 `morphTargets`에 모든 Blendshape 포함:
  - 눈 관련: `eyeBlinkLeft`, `eyeBlinkRight`, `eyeSquintLeft`, `eyeSquintRight`, `eyeLookDownLeft`, 등 (14개)
  - 눈썹 관련: `browInnerUp`, `browOuterUpLeft`, `browDownLeft` 등 (5개)
  - 입 관련: `jawOpen`, `mouthSmileLeft`, `mouthPucker`, `mouthShrugLower` 등 (23개)
  - 코/볼/혀: `noseSneerLeft`, `cheekPuff`, `tongueOut` 등 (10개)

**매핑 방식:**
- MediaPipe blendshape 배열 인덱스를 정확히 매칭 (예: `eyeBlinkLeft` = `blendshapes[9]`)
- 각 `MorphTarget`의 `weight`를 `blendshapes[index] * 100.0`으로 설정

#### 3. 모드 전환 시 MediaPipe 자동 관리
**구현 내용:**
- `onWebcamModeClicked()`: 웹캠 시작 시 MediaPipe 자동 시작
- `onConnectClicked()`: 서버 연결 시 MediaPipe 자동 시작 (UDP/RTSP 모두)
- `onDisconnectClicked()`: 연결 해제 시 MediaPipe 중지
- `startMediaPipeIfNeeded()`: 공통 함수로 중복 시작 방지

**결과:**
- 모드 전환 시 MediaPipe가 항상 정상적으로 재시작됨
- 웹캠/서버 모드 모두에서 얼굴 탐지 및 아바타 렌더링 동작

#### 4. GLB 파일 경로 자동 감지
**구현 내용:**
- `onCharacterSelected()`에서 여러 경로 후보 시도:
  - 절대 경로 우선 (`/Users/jincheol/Desktop/VEDA/RtspProject/resource/assets/`)
  - 실행 위치 기반 상대 경로
  - 캐릭터 0번 → `Avatar02.glb`, 나머지 → `Avatar01.glb`

**해결한 문제:**
- 실행 위치에 따라 경로가 깨지는 문제 → 다중 경로 시도로 해결
- `QString::arg()` 오류 → 경로 문자열 직접 구성으로 수정

### 해결한 문제들

#### 1. Qt Quick3D 모듈을 찾을 수 없음
**문제:**
- `module "QtQuick3D" version 1.15 is not installed` 에러
- QML 엔진이 Quick3D 모듈을 찾지 못함

**해결:**
- Homebrew의 qtquick3d 설치 경로를 동적으로 감지 (`/opt/homebrew/Cellar/qtquick3d/6.10.1/share/qt/qml`)
- `VideoQuick3DWidget`에서 QML 엔진에 import 경로 추가
- 런타임 로드 방식으로 Quick3DTools 의존성 문제 회피

#### 2. View3D.qml이 로드되지 않음
**문제:**
- QML 문법 오류 (`morphTargets` 배열에서 콤마 누락)
- Loader가 View3D를 로드하지 못함

**해결:**
- 모든 `MorphTarget` 사이에 콤마 추가
- 디버그 로그 추가하여 로드 상태 확인

#### 3. 3D 모델이 화면에 안 보임
**문제:**
- GLB 파일은 로드되지만 모델이 안 보임
- 카메라 위치/스케일 문제

**해결:**
- 카메라 거리 조정 (`position: Qt.vector3d(0, 0, 10)`)
- 모델 스케일 확대 (`faceWidth * 5.0`, 최소 1.0 보장)
- 모델 Z 위치 조정 (`-2.0`으로 카메라 앞에 배치)
- View3D Loader에 `z: 1` 설정하여 Image 위에 렌더링

### 성능

**처리 속도:**
- MediaPipe 얼굴 탐지: ~30-50ms (5프레임마다 처리)
- Blendshape 추출: MediaPipe 내부 처리 (추가 오버헤드 없음)
- 3D 렌더링: Qt Quick3D 하드웨어 가속 (Metal on macOS)
- 프레임 표시: 실시간 (30 FPS)

**메모리:**
- numpy array 변환 시 필요 최소한의 복사만 수행
- 프레임 큐 크기 제한 (최대 5개 프레임)
- GLB 모델은 Qt Quick3D가 효율적으로 관리

### 다음 단계

1. **Blendshape 튜닝 및 최적화**
   - 각 Blendshape의 weight 범위 조정 (0-100 → 더 자연스러운 범위)
   - 주요 표정(눈 깜빡임, 미소 등)의 반응성 향상

2. **다중 캐릭터 지원**
   - 캐릭터별 GLB 파일 매핑 확장
   - 각 캐릭터의 Morph Target 이름 차이 대응

3. **UI 개선**
   - Blendshape 값 실시간 표시 (디버그용)
   - 얼굴 탐지 상태 및 FPS 표시
   - 캐릭터 선택 UI 개선

4. **RTSP/UDP 모드 안정화**
   - 네트워크 패킷 손실에 대한 강건성 향상
   - 재연결 로직 개선

### 참고 자료
- MediaPipe Face Landmarker: https://developers.google.com/mediapipe/solutions/vision/face_landmarker
- pybind11 임베딩: https://pybind11.readthedocs.io/en/stable/advanced/embedding.html
- Qt Quick3D: https://doc.qt.io/qt-6/qtquick3d-index.html
