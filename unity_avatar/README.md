# Unity Avatar Renderer

Unity를 사용하여 MediaPipe Blendshape 데이터를 받아 3D 아바타를 렌더링하고, 렌더링된 이미지를 Qt 클라이언트에 전달합니다.

## 아키텍처

```
Qt Client (C++)
  ├─ MediaPipe (Python) → Blendshape 데이터 (52개)
  ├─ Unity 프로세스 시작 (별도 프로세스)
  │   ├─ Unity에서 GLB 모델 로드
  │   ├─ Blendshape → BlendShape 매핑 (52개 모두 지원)
  │   └─ 렌더링된 이미지를 텍스처로 출력
  └─ Qt에서 Unity 렌더링 이미지를 QML에 표시
```

## 왜 Unity로 전환했나?

### Qt Quick3D의 한계
- **Morph Target 제한**: GPU Attribute 슬롯이 최대 8~16개로 제한됨
- **52개 Blendshape 불가**: MediaPipe는 52개 Blendshape를 제공하지만, Qt Quick3D는 8개만 지원
- **결과**: 눈을 깜빡였는데 입이 벌어지는 등 잘못된 매핑 발생

### Unity의 장점
- **52개 BlendShape 완전 지원**: Unity는 BlendShape를 제한 없이 지원
- **고성능 렌더링**: Unity의 최적화된 렌더링 파이프라인
- **표준 GLB 지원**: GLB 파일의 모든 BlendShape를 정확히 매핑

## 프로젝트 구조

```
unity_avatar/
├── README.md              # 이 파일
├── Assets/
│   ├── Scripts/
│   │   ├── MediaPipeReceiver.cs      # MediaPipe 데이터 수신 (TCP/UDP)
│   │   ├── BlendShapeController.cs   # BlendShape 적용
│   │   └── RenderTextureExporter.cs  # 렌더링 결과를 이미지로 출력
│   ├── Models/
│   │   └── Avatar02.glb              # 3D 아바타 모델
│   └── Scenes/
│       └── AvatarScene.unity         # 메인 씬
└── Build/
    └── UnityAvatar.app               # 빌드된 Unity 실행 파일
```

## 통신 프로토콜

### MediaPipe → Unity
- **방식**: TCP 소켓 또는 UDP
- **데이터 형식**: JSON
  ```json
  {
    "blendshapes": [0.0, 0.1, 0.2, ...],  // 52개 float 배열
    "faceX": 0.5,
    "faceY": 0.5,
    "faceWidth": 0.3,
    "faceHeight": 0.4
  }
  ```

### Unity → Qt
- **방식**: 공유 메모리 또는 네트워크 (TCP/UDP)
- **데이터 형식**: PNG/JPEG 이미지 스트림 또는 텍스처 공유

## 빌드 및 실행

### Unity 프로젝트 설정
1. Unity Hub에서 프로젝트 열기
2. `Assets/Scenes/AvatarScene.unity` 열기
3. `File > Build Settings`에서 플랫폼 선택 (macOS)
4. `Build` 클릭하여 실행 파일 생성

### Qt 클라이언트에서 Unity 실행
```cpp
// MainWindow.cpp
QProcess *unityProcess = new QProcess(this);
unityProcess->start("/path/to/UnityAvatar.app/Contents/MacOS/UnityAvatar", 
                    QStringList() << "--port" << "12345");
```

## 개발 상태

- [x] 아키텍처 설계
- [x] 통합 가이드 문서 작성
- [ ] Unity 프로젝트 생성
- [ ] MediaPipe 데이터 수신 스크립트
- [ ] BlendShape 매핑 로직
- [ ] 렌더링 결과 출력
- [ ] Qt 클라이언트 통합

## 문서

- **[INTEGRATION_GUIDE.md](./INTEGRATION_GUIDE.md)**: 상세한 통합 가이드 (Unity 스크립트 포함)
- **[QUICK_START.md](./QUICK_START.md)**: 빠른 시작 가이드
- **[SETUP.md](./SETUP.md)**: Unity 프로젝트 설정 가이드
