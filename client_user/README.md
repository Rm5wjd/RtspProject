# RTSP 클라이언트 - Qt 기반 아바타 렌더링

Qt 기반 RTSP/UDP/웹캠 클라이언트 프로그램으로, 스트림을 받아서 **MediaPipe 얼굴 인식 + Blendshape**를 통해 선택한 3D 아바타(`Avatar02.glb`)를 실시간으로 렌더링합니다.

## 기능

- ✅ RTSP/UDP 비디오 스트림 수신 및 재생
- ✅ 웹캠 모드 지원 (기본 모드, 640x480 @ 30FPS)
- ✅ **MediaPipe 기반 실시간 얼굴 탐지 및 Blendshape 추출**
  - 478개 얼굴 랜드마크 (정규화 좌표 0~1)
  - 52개 Blendshape (눈 깜빡임, 입 벌림, 표정 등)
- ✅ **3D 아바타 렌더링 (Avatar02.glb)**
  - 얼굴 위치/크기에 맞게 아바타 배치
  - Blendshape 값을 Morph Target에 매핑하여 표정 제어
- ✅ 캐릭터 선택 UI (하단)
- ✅ 연결/연결 해제 버튼

## 요구사항

### 필수
- Qt6 (Widgets, Quick, QuickWidgets)
- OpenCV 4.x (VideoCapture, imgproc)
- Python 3.10 이상 + `mediapipe`, `opencv-python`, `numpy`
- Unity 2022.3 이상 (3D 아바타 렌더링용)
- CMake 3.16 이상
- C++17 지원 컴파일러

### Python 의존성 설치

```bash
cd client_user/python
pip3 install -r requirements.txt
```

### MediaPipe 모델 파일

- `client_user/thirdparty/mediapipe/models/face_landmarker.task`
- [MediaPipe Face Landmarker 모델](https://developers.google.com/mediapipe/solutions/vision/face_landmarker)에서 다운로드

### GLB 모델 파일

- `resource/assets/Avatar02.glb` (캐릭터 0번) - Unity 프로젝트에 복사 필요
- `resource/assets/Avatar01.glb` (캐릭터 1번 이상) - Unity 프로젝트에 복사 필요

### Unity 프로젝트

- `unity_avatar/` 디렉토리에 Unity 프로젝트가 있습니다.
- Unity Hub에서 프로젝트를 열고 빌드하세요.
- 자세한 내용은 `unity_avatar/README.md`를 참조하세요.

## 빌드

```bash
cd client_user
mkdir build
cd build
cmake ..
make -j4
```

## 실행

```bash
cd build/bin
./client_user
```

## 사용 방법

1. **웹캠 모드 (기본)**  
   - 프로그램 실행 후 1초 뒤 자동으로 웹캠 모드로 전환됩니다.  
   - 상단의 "웹캠 모드" 버튼을 눌러도 전환 가능.  
   - MediaPipe가 자동으로 시작되어 얼굴 탐지 + 아바타 연동이 동작합니다.

2. **서버 연결 (UDP / RTSP)**  
   - 상단 입력란에 **UDP 포트 번호**(예: `5000`)나 **RTSP URL** (예: `rtsp://192.168.0.82:8554/live`)을 입력  
   - "서버 연결" 버튼 클릭  
   - OpenCV + GStreamer/FFmpeg로 스트림을 열고, MediaPipe가 동일하게 동작합니다.

3. **캐릭터 선택**  
   - 하단의 캐릭터 목록에서 원하는 캐릭터를 클릭  
   - 캐릭터 0번 → `Avatar02.glb`, 나머지 → `Avatar01.glb`가 Unity에서 로드됩니다.
   - Unity에서 52개 Blendshape를 모두 적용하여 표정이 실시간으로 변합니다.

4. **연결 해제**  
   - "연결 해제" 버튼 클릭  
   - 비디오 스트림과 MediaPipe 프로세서가 모두 정리됩니다.

## 네트워크 설정

서버가 UDP로 비디오를 전송하는 경우:
- 기본 포트: 5000
- 입력란에 포트 번호만 입력 (예: `5000`)

RTSP 스트림을 사용하는 경우:
- 입력란에 RTSP URL 입력 (예: `rtsp://192.168.1.100:8554/stream`)

## 캐릭터 커스터마이징

`MainWindow.cpp`의 `setupCharacterSelector()` 함수에서 캐릭터 목록을 수정할 수 있습니다:

```cpp
QStringList characters = {"캐릭터 1", "캐릭터 2", "캐릭터 3", "캐릭터 4"};
```

각 캐릭터의 GLB 파일 경로는 `onCharacterSelected()` 함수에서 설정됩니다.

## 아바타 렌더링 커스터마이징

- Unity 프로젝트 (`unity_avatar/`)에서 3D 아바타 렌더링 로직을 수정할 수 있습니다.
- `BlendShapeController.cs`에서 MediaPipe Blendshape 이름 ↔ Unity BlendShape 인덱스를 매핑합니다.
- 필요 시 다른 GLB 파일로 교체하여 다른 캐릭터를 사용할 수 있습니다.
- 자세한 내용은 `unity_avatar/README.md`를 참조하세요.

## 문제 해결

### Unity 관련
- Unity 프로젝트가 빌드되었는지 확인: `unity_avatar/Build/UnityAvatar.app`
- Unity와 Qt 간 네트워크 통신이 정상인지 확인 (포트 12345, 12346)
- Unity 콘솔 로그 확인: MediaPipe 데이터 수신 및 BlendShape 적용 상태

### MediaPipe 관련
- Python 패키지가 누락된 경우: `pip3 install -r client_user/python/requirements.txt`
- 모델 파일 누락: `client_user/thirdparty/mediapipe/models/face_landmarker.task` 확인

### 비디오 스트림을 열 수 없음
- 서버가 실행 중인지 확인하세요.
- 포트 번호/RTSP URL이 올바른지 확인하세요.
- 방화벽 설정을 확인하세요.

### Unity 렌더링이 안 보임
- Unity 프로세스가 실행 중인지 확인
- Unity 콘솔에서 MediaPipe 데이터 수신 로그 확인
- Unity에서 GLB 모델이 로드되었는지 확인 (`Assets/Models/Avatar02.glb`)
- 네트워크 포트가 올바른지 확인 (기본: 12345, 12346)

### Qt를 찾을 수 없음
- Qt가 설치되어 있는지 확인: `brew install qt` (macOS)
- CMake가 Qt를 찾을 수 있도록 경로를 설정하세요.

## 개발 노트

- QGst는 macOS에서 잘 작동하지 않을 수 있으므로, OpenCV의 GStreamer/FFmpeg 백엔드를 사용합니다.
- 얼굴 인식/표정 추출은 **MediaPipe FaceLandmarker (Python)** 를 사용하며, C++에서는 pybind11로 임베딩합니다.
- **아바타 렌더링은 Unity 기반**이며, MediaPipe Blendshape 데이터를 Unity로 전송하여 52개 BlendShape를 모두 적용합니다.
- 웹캠 / UDP / RTSP 모드 전환 시 `MainWindow`에서 MediaPipe를 항상 `stop → start` 흐름으로 관리하여, 모드 변경 후에도 얼굴 탐지가 안정적으로 재시작되도록 구성되어 있습니다.
- **카메라는 MainWindow에서만 열고 관리**하며, MediaPipeProcessor는 프레임 데이터만 받아서 처리합니다.
- Qt Quick3D 관련 코드는 남아있지만, Unity 전환으로 인해 현재는 사용하지 않습니다.