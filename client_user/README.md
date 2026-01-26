# RTSP 클라이언트 - Qt 기반 아바타 렌더링

Qt 기반 RTSP 클라이언트 프로그램으로, RTSP 스트림을 받아서 실시간으로 얼굴을 인식하고 선택한 캐릭터로 아바타를 렌더링합니다.

## 기능

- ✅ RTSP/UDP 비디오 스트림 수신 및 재생
- ✅ 웹캠 모드 지원 (기본 모드)
- ✅ **실시간 얼굴 탐지 (MediaPipe Face Landmarker)**
  - 478개 얼굴 랜드마크 추출
  - 52개 Blendshape 값 추출 (눈 깜빡임, 입 벌림, 표정 등)
  - Python C API 임베딩으로 고성능 처리
- ✅ 실시간 얼굴 표시 (OpenCV + QML)
  - 랜드마크 점 표시
  - 얼굴 영역 박스 표시
- ✅ 캐릭터 선택 UI (하단)
- ✅ 선택한 캐릭터로 아바타 렌더링
- ✅ 연결/연결 해제 버튼

## 요구사항

### 필수
- Qt5 또는 Qt6
- OpenCV 4.x
- Python 3.10 이상 (Python3 C API 지원)
- NumPy (Python 패키지)
- MediaPipe (Python 패키지)
- CMake 3.10 이상
- C++17 지원 컴파일러

### Python 의존성 설치
```bash
cd client_user/python
pip3 install -r requirements.txt
```

또는 직접 설치:
```bash
pip3 install mediapipe opencv-python numpy
```

### MediaPipe 모델 파일
`face_landmarker.task` 파일이 다음 경로에 있어야 합니다:
- `client_user/thirdparty/mediapipe/models/face_landmarker.task`

[MediaPipe Face Landmarker 모델 다운로드](https://developers.google.com/mediapipe/solutions/vision/face_landmarker)

### 선택적
- QGst (GStreamer Qt 바인딩) - 있으면 사용, 없으면 OpenCV로 대체

## 빌드

```bash
cd client_user
mkdir build
cd build
cmake ..
make
```

## 실행

```bash
cd build/bin
./client_user
```

## 사용 방법

1. **웹캠 모드 (기본)**
   - 애플리케이션 실행 시 자동으로 웹캠 모드로 시작
   - 웹캠 권한이 필요합니다 (macOS)
   - 실시간으로 얼굴 탐지 및 표시

2. **서버 연결**
   - 상단의 서버 주소 입력란에 UDP 포트 번호를 입력 (기본값: 5000)
   - 또는 RTSP URL 입력: `rtsp://서버주소:포트/스트림`
   - "연결" 버튼 클릭

3. **얼굴 탐지 확인**
   - 얼굴이 탐지되면:
     - 비디오 프레임에 초록색 랜드마크 점들이 표시됩니다
     - 얼굴 영역 주변에 노란색 박스가 표시됩니다
     - 콘솔에 Blendshape 값이 출력됩니다

4. **캐릭터 선택**
   - 하단의 캐릭터 목록에서 원하는 캐릭터 클릭
   - 선택한 캐릭터가 검출된 얼굴 위에 렌더링됩니다

5. **연결 해제**
   - "연결 해제" 버튼 클릭

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

각 캐릭터의 아이콘을 추가하려면:
```cpp
item->setIcon(QIcon("path/to/character.png"));
```

## 아바타 렌더링 커스터마이징

`MainWindow.cpp`의 `renderAvatar()` 함수에서 아바타 렌더링 로직을 수정할 수 있습니다.

현재는 간단한 원형 아바타를 렌더링하지만, 3D 모델이나 이미지 기반 아바타로 교체할 수 있습니다.

## 문제 해결

### 얼굴 인식 모델을 찾을 수 없음
- `resource/haarcascade_frontalface_alt.xml` 파일이 프로젝트 루트의 `resource/` 폴더에 있는지 확인하세요.

### 비디오 스트림을 열 수 없음
- 서버가 실행 중인지 확인하세요.
- 포트 번호가 올바른지 확인하세요.
- 방화벽 설정을 확인하세요.

### Qt를 찾을 수 없음
- Qt가 설치되어 있는지 확인하세요: `brew install qt` (macOS)
- CMake가 Qt를 찾을 수 있도록 경로를 설정하세요.

## 개발 노트

### 아키텍처
- **Python C API 임베딩**: MediaPipe를 Python으로 실행하되, 같은 프로세스 내에서 직접 호출
- **비동기 처리**: 5프레임마다 얼굴 탐지 (설정 가능)
- **이중 렌더링**: OpenCV로 프레임에 직접 그리기 + QML로 오버레이 표시

### 얼굴 탐지
- **MediaPipe Face Landmarker**: 478개 랜드마크 + 52개 Blendshape
- **처리 속도**: ~30-50ms (5프레임마다)
- **정확도**: 실시간 비디오에서 안정적인 탐지

### Blendshape 활용
52개의 Blendshape 값으로 아바타를 제어할 수 있습니다:
- 눈 깜빡임: `eyeBlinkLeft`, `eyeBlinkRight`
- 눈 움직임: `eyeLookUpLeft`, `eyeLookDownLeft` 등
- 입 벌림: `jawOpen`
- 표정: `mouthSmileLeft`, `mouthSmileRight` 등

### 기술적 결정사항
- QGst는 macOS에서 잘 작동하지 않을 수 있으므로, OpenCV의 GStreamer 백엔드를 사용합니다.
- Python C API를 사용하여 프로세스 간 통신 오버헤드를 제거했습니다.
- venv의 site-packages를 자동으로 감지하여 추가합니다.
- 아바타 렌더링은 현재 간단한 도형으로 구현되어 있으며, 향후 3D 모델로 확장 가능합니다.

## 문제 해결

### MediaPipe 모델을 찾을 수 없음
- `client_user/thirdparty/mediapipe/models/face_landmarker.task` 파일이 있는지 확인하세요.
- [MediaPipe Face Landmarker 모델 다운로드](https://developers.google.com/mediapipe/solutions/vision/face_landmarker)

### Python 모듈을 찾을 수 없음
- venv가 활성화되어 있는지 확인하세요.
- `pip3 install mediapipe opencv-python numpy` 실행
- Python 버전이 3.10 이상인지 확인하세요.

### 얼굴이 탐지되지 않음
- 웹캠이 제대로 연결되어 있는지 확인하세요.
- 조명이 충분한지 확인하세요.
- 콘솔에 오류 메시지가 있는지 확인하세요.
