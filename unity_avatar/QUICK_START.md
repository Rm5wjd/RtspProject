# Unity 아바타 통합 빠른 시작 가이드

## 개요

Qt 애플리케이션에서 MediaPipe로 추출한 blendshape 데이터를 Unity로 전달하여 3D 아바타를 렌더링하고, 그 결과를 다시 Qt에 표시하는 방법입니다.

## 핵심 개념

### 데이터 흐름

```
Qt (C++) 
  → MediaPipe (Python) 
  → Blendshape 데이터 (52개) 
  → Unity (TCP 소켓) 
  → 아바타 렌더링 
  → 이미지 스트림 
  → Qt (QML 표시)
```

### 데이터 형식

**Qt에서 Unity로 전송하는 JSON:**
```json
{
  "blendshapes": [
    {"category": "eyeBlinkLeft", "score": 0.5},
    {"category": "mouthSmileLeft", "score": 0.8},
    ...
  ]
}
```

**Unity에서 Qt로 전송하는 이미지:**
- PNG 또는 JPEG 형식
- 헤더(4바이트: 이미지 크기) + 이미지 데이터

## 필수 단계

### 1. Unity 프로젝트 준비

1. **MediaPipeUnityPlugin 설치** (선택사항 - 직접 MediaPipe 실행 시)
   - [MediaPipeUnityPlugin 릴리스](https://github.com/homuler/MediaPipeUnityPlugin/releases)에서 다운로드
   - Unity 프로젝트에 임포트

2. **스크립트 추가**
   - `BlendshapeReceiver.cs` - Qt로부터 데이터 수신
   - `AvatarBlendshapeController.cs` - 아바타에 blendshape 적용
   - `RenderTextureSender.cs` - 렌더링 결과를 Qt로 전송

3. **씬 설정**
   - GLB 아바타 모델 추가
   - 카메라 설정
   - 스크립트 컴포넌트 연결

### 2. Qt 클라이언트 수정

**MainWindow.h에 추가:**
```cpp
QProcess *unityProcess;
QTcpSocket *unityBlendshapeSocket;
QTcpServer *unityImageServer;
QTcpSocket *unityImageSocket;
```

**MainWindow.cpp에 구현:**
- `startUnityProcess()` - Unity 프로세스 시작
- `sendBlendshapeDataToUnity()` - blendshape 데이터 전송
- `onUnityImageReceived()` - Unity 이미지 수신

### 3. 포트 설정

- **Unity → Qt (Blendshape)**: 포트 12345
- **Qt → Unity (Image)**: 포트 12346

## 빠른 테스트

### Unity 씬에서 테스트

1. Unity Editor에서 씬 열기
2. `BlendshapeReceiver` 컴포넌트의 `listenPort` 확인 (12345)
3. Play 모드 실행
4. Qt 애플리케이션에서 `startUnityProcess()` 호출
5. 웹캠 모드 활성화하여 얼굴 탐지 시작

### Qt에서 테스트

```cpp
// MainWindow 생성자 또는 버튼 클릭 핸들러에서
startUnityProcess();

// MediaPipeProcessor의 faceDetected 시그널 연결
connect(mediaPipeProcessor, &MediaPipeProcessor::faceDetected,
        this, &MainWindow::sendBlendshapeDataToUnity);
```

## 주요 매핑: MediaPipe → Unity BlendShape

MediaPipe의 52개 blendshape 이름은 Unity의 BlendShape 이름과 일치해야 합니다:

```
eyeBlinkLeft, eyeBlinkRight
eyeLookDownLeft, eyeLookDownRight
eyeLookInLeft, eyeLookInRight
eyeLookOutLeft, eyeLookOutRight
eyeLookUpLeft, eyeLookUpRight
eyeSquintLeft, eyeSquintRight
eyeWideLeft, eyeWideRight
browDownLeft, browDownRight
browInnerUp, browOuterUpLeft, browOuterUpRight
noseSneerLeft, noseSneerRight
cheekPuff, cheekSquintLeft, cheekSquintRight
jawForward, jawLeft, jawOpen, jawRight
mouthClose, mouthDimpleLeft, mouthDimpleRight
mouthFrownLeft, mouthFrownRight
mouthFunnel, mouthLeft, mouthLowerDownLeft, mouthLowerDownRight
mouthPressLeft, mouthPressRight, mouthPucker, mouthRight
mouthRollLower, mouthRollUpper, mouthShrugLower, mouthShrugUpper
mouthSmileLeft, mouthSmileRight
mouthStretchLeft, mouthStretchRight
mouthUpperUpLeft, mouthUpperUpRight
tongueOut
```

**중요**: GLB 모델의 BlendShape 이름이 위 목록과 정확히 일치해야 합니다.

## 문제 해결

### Unity가 데이터를 받지 못함
- 포트 번호 확인 (12345)
- Qt의 `unityBlendshapeSocket` 연결 상태 확인
- JSON 형식 확인 (줄바꿈 포함)

### 아바타가 움직이지 않음
- `AvatarBlendshapeController`의 `BuildBlendshapeMap()` 로그 확인
- GLB 모델의 BlendShape 이름 확인
- MediaPipe blendshape 이름과 Unity BlendShape 이름 매칭 확인

### 이미지가 표시되지 않음
- `RenderTextureSender`의 연결 포트 확인 (12346)
- Qt의 `unityImageServer` 리스닝 상태 확인
- 이미지 데이터 형식 확인 (PNG/JPEG)

## 상세 문서

더 자세한 내용은 [INTEGRATION_GUIDE.md](./INTEGRATION_GUIDE.md)를 참조하세요.
