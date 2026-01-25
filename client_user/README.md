# RTSP 클라이언트 - Qt 기반 아바타 렌더링

Qt 기반 RTSP 클라이언트 프로그램으로, RTSP 스트림을 받아서 실시간으로 얼굴을 인식하고 선택한 캐릭터로 아바타를 렌더링합니다.

## 기능

- ✅ RTSP/UDP 비디오 스트림 수신 및 재생
- ✅ 실시간 얼굴 인식 (OpenCV Haar Cascade)
- ✅ 캐릭터 선택 UI (하단)
- ✅ 선택한 캐릭터로 아바타 렌더링
- ✅ 연결/연결 해제 버튼

## 요구사항

### 필수
- Qt5 또는 Qt6
- OpenCV 4.x
- CMake 3.10 이상
- C++17 지원 컴파일러

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

1. **연결하기**
   - 상단의 서버 주소 입력란에 UDP 포트 번호를 입력 (기본값: 5000)
   - 또는 RTSP URL 입력: `rtsp://서버주소:포트/스트림`
   - "연결" 버튼 클릭

2. **캐릭터 선택**
   - 하단의 캐릭터 목록에서 원하는 캐릭터 클릭
   - 선택한 캐릭터가 검출된 얼굴 위에 렌더링됩니다

3. **연결 해제**
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

- QGst는 macOS에서 잘 작동하지 않을 수 있으므로, OpenCV의 GStreamer 백엔드를 사용합니다.
- 얼굴 인식은 OpenCV의 Haar Cascade를 사용합니다.
- 아바타 렌더링은 현재 간단한 도형으로 구현되어 있으며, 향후 3D 모델로 확장 가능합니다.
