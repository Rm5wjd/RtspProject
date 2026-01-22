# Client User 프로그램 개발 문서

## 개요

`client_user`는 RTSP 서버로부터 전송되는 영상 스트림을 받아서 실시간으로 얼굴을 인식하고, OpenGL을 사용하여 얼굴 위에 3D 아바타를 렌더링하는 클라이언트 프로그램입니다.

## 프로젝트 구조

```
client_user/
├── CMakeLists.txt          # 빌드 설정
└── src/
    └── main.cpp            # 메인 프로그램
resource/                   # 렌더링 관련 리소스
├── haarcascade_frontalface_alt.xml  # 얼굴 인식 모델
└── (향후 아바타 모델 및 텍스처 추가)
```

## 개발 구현 단계

### Phase 1: 기본 OpenGL 렌더링 (현재 단계) ✅

**목표**: 로컬 비디오 파일 또는 웹캠에서 영상을 받아서 OpenGL로 렌더링

**구현 내용**:
- OpenGL 초기화 및 윈도우 생성
- OpenCV를 사용한 비디오 캡처
- 비디오 프레임을 OpenGL 텍스처로 변환하여 렌더링
- 기본 셰이더 프로그램 구현

**모드 전환**:
- `#define USE_LOCAL_VIDEO 1`: 로컬 비디오/웹캠 사용
- `#define USE_LOCAL_VIDEO 0`: 서버 스트림 사용 (Phase 2에서 구현)

**현재 상태**:
- ✅ OpenGL 윈도우 생성 및 초기화
- ✅ 비디오 프레임 렌더링
- ✅ 기본 셰이더 구현

### Phase 2: 얼굴 인식 기능

**목표**: OpenCV Haar Cascade를 사용하여 실시간 얼굴 인식

**구현 내용**:
- Haar Cascade 모델 로드 (`haarcascade_frontalface_alt.xml`)
- 각 프레임에서 얼굴 검출
- 검출된 얼굴 위치를 OpenGL 좌표계로 변환

**현재 상태**:
- ✅ 얼굴 인식 기능 구현
- ✅ 검출된 얼굴 위치 표시 (빨간색 사각형)

### Phase 3: 3D 아바타 렌더링

**목표**: 검출된 얼굴 위치에 3D 아바타 모델을 렌더링

**구현 내용**:
- 3D 모델 로더 구현 (OBJ 파일 등)
- 아바타 모델을 얼굴 위치에 맞춰 배치
- 얼굴 크기에 맞춰 아바타 스케일 조정
- 텍스처 매핑 및 조명 처리

**예상 작업**:
- [ ] 3D 모델 로더 클래스 구현
- [ ] 아바타 모델 파일 추가 (resource 폴더)
- [ ] 얼굴 위치에 따른 아바타 변환 행렬 계산
- [ ] 3D 렌더링 파이프라인 구현

### Phase 4: 서버 스트림 연동

**목표**: RTSP 서버로부터 영상 스트림을 받아서 처리

**구현 내용**:
- RTSP 클라이언트 구현 또는 라이브러리 사용 (예: libVLC, FFmpeg)
- 서버로부터 H.264 스트림 수신
- 스트림 디코딩 및 프레임 추출
- Phase 1의 렌더링 파이프라인과 통합

**예상 작업**:
- [ ] RTSP 클라이언트 라이브러리 선택 및 통합
- [ ] 스트림 수신 및 디코딩 구현
- [ ] `USE_LOCAL_VIDEO` 모드 전환 로직 완성
- [ ] 네트워크 오류 처리 및 재연결 로직

### Phase 5: 최적화 및 개선

**목표**: 성능 최적화 및 사용자 경험 개선

**구현 내용**:
- 멀티스레딩을 통한 렌더링 성능 향상
- 얼굴 인식 정확도 개선 (더 정교한 모델 사용)
- 아바타 애니메이션 추가
- UI 개선 (설정 메뉴, 모드 전환 등)

## 기술 스택

### 필수 라이브러리
- **OpenCV**: 비디오 처리 및 얼굴 인식
- **OpenGL 3.3+**: 3D 렌더링
- **GLFW**: 윈도우 관리 및 이벤트 처리
- **GLEW**: OpenGL 확장 로더

### 향후 추가 예정
- **libVLC** 또는 **FFmpeg**: RTSP 스트림 처리
- **Assimp**: 3D 모델 로더 (OBJ, FBX 등)

## 빌드 방법

### Mac M1 (Apple Silicon)

#### 1. Homebrew 설치 (미설치 시)

```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

#### 2. 의존성 설치

```bash
# CMake
brew install cmake

# OpenCV 설치 또는 업데이트
brew install opencv
# 또는 업데이트가 필요한 경우
brew upgrade opencv

# OpenGL, GLFW, GLEW
brew install glfw glew
```

**OpenCV 버전 확인**:
```bash
# 설치된 OpenCV 버전 확인
brew list --versions opencv

# OpenCV 설치 경로 확인
brew --prefix opencv

# OpenCV CMake 설정 파일 위치 확인
ls -la $(brew --prefix opencv)/lib/cmake/opencv4/
```

#### 3. CMake 빌드

```bash
cd client_user
mkdir build
cd build
cmake ..
make
```

#### 4. 실행

```bash
# resource 폴더에 haarcascade_frontalface_alt.xml 파일이 있어야 함
./bin/client_user
```

**참고**: Mac M1에서 OpenCV를 Homebrew로 설치하면 기본적으로 `/opt/homebrew` 경로에 설치됩니다. CMake가 자동으로 찾지만, 문제가 발생하면 다음 환경 변수를 설정하세요:

```bash
export OpenCV_DIR=/opt/homebrew/opt/opencv/lib/cmake/opencv4
```

### Ubuntu

#### 1. 의존성 설치

```bash
# CMake
sudo apt-get update
sudo apt-get install cmake

# OpenCV
sudo apt-get install libopencv-dev

# OpenGL, GLFW, GLEW
sudo apt-get install libgl1-mesa-dev libglfw3-dev libglew-dev
```

#### 2. CMake 빌드

```bash
cd client_user
mkdir build
cd build
cmake ..
make
```

#### 3. 실행

```bash
# resource 폴더에 haarcascade_frontalface_alt.xml 파일이 있어야 함
./bin/client_user
```

### 공통 사항

#### 리소스 파일 준비

프로그램 실행 전에 얼굴 인식 모델 파일이 필요합니다:

```bash
# OpenCV GitHub에서 다운로드
cd resource
wget https://raw.githubusercontent.com/opencv/opencv/master/data/haarcascades/haarcascade_frontalface_alt.xml
```

또는 수동으로 다운로드:
- URL: https://github.com/opencv/opencv/blob/master/data/haarcascades/haarcascade_frontalface_alt.xml
- 저장 위치: `resource/haarcascade_frontalface_alt.xml`

#### 빌드 문제 해결

**Mac M1에서 OpenCV를 찾을 수 없는 경우:**

```bash
# CMake 캐시 삭제 후 재빌드
cd build
rm -rf *
cmake -DOpenCV_DIR=/opt/homebrew/opt/opencv/lib/cmake/opencv4 ..
make
```

**Ubuntu에서 GLFW를 찾을 수 없는 경우:**

```bash
# pkg-config 확인
pkg-config --modversion glfw3

# CMake에서 직접 경로 지정
cmake -DGLFW3_DIR=/usr/lib/x86_64-linux-gnu/cmake/glfw3 ..
```

## 모드 전환

`src/main.cpp` 파일의 상단에서 모드를 변경할 수 있습니다:

```cpp
// 로컬 비디오/웹캠 사용
#define USE_LOCAL_VIDEO 1

// 서버 스트림 사용 (Phase 4에서 구현)
#define USE_LOCAL_VIDEO 0
```

## 현재 구현 상태

### 완료된 기능
- ✅ OpenGL 초기화 및 윈도우 생성
- ✅ 비디오 프레임 캡처 및 렌더링
- ✅ 얼굴 인식 기능
- ✅ 검출된 얼굴 위치 시각화

### 진행 중
- 🔄 기본 렌더링 파이프라인 안정화

### 예정된 기능
- ⏳ 3D 아바타 모델 렌더링
- ⏳ RTSP 서버 스트림 연동
- ⏳ 성능 최적화

## 문제 해결

### 얼굴 인식 모델 파일이 없다는 오류
- `resource/haarcascade_frontalface_alt.xml` 파일이 필요합니다
- OpenCV GitHub에서 다운로드: https://github.com/opencv/opencv/tree/master/data/haarcascades

### OpenGL 컨텍스트 생성 실패

**Mac M1:**
- OpenGL은 macOS에서 Metal을 통해 지원됩니다
- GLFW가 올바르게 설치되었는지 확인: `brew list glfw`
- Xcode Command Line Tools 설치 확인: `xcode-select --install`

**Ubuntu:**
- 그래픽 드라이버가 최신인지 확인
- `glxinfo | grep "OpenGL version"` 명령으로 OpenGL 버전 확인
- Mesa 드라이버 설치: `sudo apt-get install mesa-utils`

### OpenCV를 찾을 수 없는 경우 (Mac M1)

**증상**: CMake 에러 메시지에서 OpenCV를 찾을 수 없다고 표시되거나, `/usr/local/lib/cmake/opencv4`의 오래된 설정 파일을 참조하는 오류 발생

**해결 방법**:

1. **OpenCV 설치 확인**:
```bash
brew list opencv
```

2. **OpenCV 설치/업데이트**:
```bash
# 설치되지 않은 경우
brew install opencv

# 이미 설치되어 있지만 업데이트가 필요한 경우
brew upgrade opencv

# 강제 재설치 (문제가 지속되는 경우)
brew uninstall opencv
brew install opencv
```

3. **OpenCV 버전 및 경로 확인**:
```bash
# 버전 확인
brew list --versions opencv

# 설치 경로 확인
brew --prefix opencv

# CMake 설정 파일 확인
ls -la $(brew --prefix opencv)/lib/cmake/opencv4/
```

3. **오래된 OpenCV 설정 파일 제거** (Intel Mac에서 남아있는 경우):
```bash
# /usr/local의 오래된 설정 파일 제거 (주의: 다른 프로그램에 영향을 줄 수 있음)
sudo rm -rf /usr/local/lib/cmake/opencv4
sudo rm -rf /usr/local/include/opencv4
```

4. **빌드 디렉토리 정리 후 재빌드**:
```bash
cd client_user/build
rm -rf *
cmake ..
make
```

5. **OpenCV 경로를 수동으로 지정** (필요한 경우):
```bash
cd client_user/build
rm -rf *
cmake -DOpenCV_DIR=$(brew --prefix opencv)/lib/cmake/opencv4 ..
make
```

### 비디오 캡처 실패
- 웹캠이 연결되어 있는지 확인
- 비디오 파일 경로를 사용하는 경우 파일 경로 확인

## 향후 개선 사항

1. **더 정확한 얼굴 인식**: DNN 기반 얼굴 인식 모델 사용 (예: OpenCV DNN, MediaPipe)
2. **아바타 커스터마이징**: 사용자가 아바타 모델을 선택할 수 있는 기능
3. **실시간 성능 모니터링**: FPS 표시 및 성능 메트릭
4. **설정 파일**: JSON/YAML을 통한 설정 관리
5. **멀티 얼굴 지원**: 여러 얼굴에 각각 다른 아바타 렌더링
