# GLB 파일 로딩 설정 가이드

## 1. TinyGLTF 다운로드

GLB 파일을 로드하기 위해 `tinygltf` 헤더 파일이 필요합니다.

### 방법 1: 직접 다운로드 (권장)

```bash
cd client_user
mkdir -p thirdparty/tinygltf
cd thirdparty/tinygltf
curl -L https://raw.githubusercontent.com/syoyo/tinygltf/v2.8.13/tiny_gltf.h -o tiny_gltf.h
```

### 방법 2: Git Submodule 사용

```bash
cd client_user/thirdparty
git submodule add https://github.com/syoyo/tinygltf.git tinygltf
```

## 2. GLB 파일 준비

캐릭터별 GLB 파일을 `resource/` 폴더에 배치하세요:

```
resource/
├── character0.glb  # 캐릭터 1
├── character1.glb  # 캐릭터 2
├── character2.glb  # 캐릭터 3
└── character3.glb  # 캐릭터 4
```

또는 단일 GLB 파일을 사용하려면 `MainWindow.cpp`의 `onCharacterSelected()` 함수에서 경로를 수정하세요.

## 3. 빌드

TinyGLTF 헤더 파일이 `thirdparty/tinygltf/tiny_gltf.h`에 있으면 자동으로 감지됩니다:

```bash
cd client_user/build
cmake ..
make
```

## 4. 사용 방법

1. 프로그램 실행
2. 캐릭터 선택 시 해당 캐릭터의 GLB 파일이 자동으로 로드됩니다
3. GLB 파일이 없으면 기본 큐브가 표시됩니다

## 5. GLB 파일 생성

GLB 파일은 다음 도구로 생성할 수 있습니다:

- **Blender**: 3D 모델을 GLB로 내보내기
- **glTF-Pipeline**: GLTF를 GLB로 변환
- **Online converters**: https://products.aspose.app/3d/conversion

## 6. 문제 해결

### TinyGLTF를 찾을 수 없음
- `thirdparty/tinygltf/tiny_gltf.h` 파일이 있는지 확인
- CMake 재실행: `rm -rf build && mkdir build && cd build && cmake ..`

### GLB 파일을 로드할 수 없음
- GLB 파일 경로 확인
- GLB 파일이 유효한지 확인 (다른 뷰어로 테스트)
- 콘솔 로그 확인

### 렌더링이 안 됨
- OpenGL 컨텍스트 확인
- 셰이더 컴파일 오류 확인
- GLB 모델이 너무 복잡한지 확인 (간단한 모델로 테스트)
