# Unity 프로젝트 설정 가이드

## 1. Unity Hub에서 프로젝트 열기

1. Unity Hub 실행
2. "Open" 또는 "Add" 클릭
3. `/Users/jincheol/Desktop/VEDA/RtspProject/unity_avatar` 디렉토리 선택
4. Unity 2022.3.0f1 이상 버전으로 프로젝트 열기

## 2. GLB 모델 Import

1. `Assets/Models/` 폴더 생성 (없으면)
2. `resource/assets/Avatar02.glb` 파일을 `Assets/Models/`로 복사
3. Unity에서 GLB 파일을 선택하고 Import
4. Import 후 Prefab으로 변환:
   - Hierarchy에 GLB 모델 드래그
   - Prefab으로 저장 (`Assets/Prefabs/Avatar02.prefab`)

## 3. 씬 설정

1. `Assets/Scenes/AvatarScene.unity` 씬 생성 (또는 기존 씬 사용)
2. Hierarchy에 빈 GameObject 생성: "AvatarSceneController"
3. `AvatarSceneController.cs` 스크립트를 GameObject에 추가
4. Inspector에서 설정:
   - Avatar Prefab: `Assets/Prefabs/Avatar02.prefab` 할당
   - Main Camera: 씬의 Main Camera 할당

## 4. 스크립트 설정

### MediaPipeReceiver 설정
1. Hierarchy에 빈 GameObject 생성: "MediaPipeReceiver"
2. `MediaPipeReceiver.cs` 스크립트 추가
3. Inspector 설정:
   - Port: 12345
   - Use TCP: true

### BlendShapeController 설정
1. Avatar GameObject에 `BlendShapeController.cs` 추가
2. Inspector 설정:
   - Skinned Mesh Renderer: Avatar의 SkinnedMeshRenderer 할당
   - Blend Shape Mappings: 필요 시 매핑 테이블 설정

### RenderTextureExporter 설정
1. Hierarchy에 빈 GameObject 생성: "RenderTextureExporter"
2. `RenderTextureExporter.cs` 스크립트 추가
3. Inspector 설정:
   - Render Camera: Main Camera 할당
   - Texture Width: 640
   - Texture Height: 480
   - Target FPS: 30
   - Send To Network: true
   - Network Port: 12346

## 5. 빌드 설정

1. `File > Build Settings` 열기
2. Platform: macOS 선택
3. "Switch Platform" 클릭
4. "Build" 클릭
5. 빌드 경로: `unity_avatar/Build/UnityAvatar.app`

## 6. 테스트

1. Unity에서 Play 버튼 클릭
2. Qt 클라이언트 실행
3. 웹캠 모드 시작
4. Unity 콘솔에서 MediaPipe 데이터 수신 확인
5. Unity에서 아바타 표정 변화 확인

## 문제 해결

### GLB 모델이 Import되지 않음
- Unity의 GLTF Importer 패키지 설치 필요
- `Window > Package Manager > Unity Registry > glTFast` 설치

### BlendShape가 적용되지 않음
- GLB 모델에 BlendShape가 포함되어 있는지 확인
- BlendShape 이름이 MediaPipe 표준 이름과 일치하는지 확인
- `BlendShapeController.cs`의 매핑 테이블 확인

### 네트워크 통신이 안 됨
- 포트 12345, 12346이 다른 프로그램에서 사용 중인지 확인
- 방화벽 설정 확인
- Unity와 Qt 클라이언트가 같은 머신에서 실행되는지 확인
