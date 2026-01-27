# MediaPipe Python 사이드카 프로세서

이 디렉토리는 Python으로 MediaPipe를 실행하여 얼굴 탐지 및 Blendshape 추출을 수행하는 사이드카 프로세서입니다.

## 설치 방법

### 1. Python 의존성 설치

```bash
pip3 install mediapipe opencv-python numpy
```

또는 `requirements.txt` 사용:

```bash
pip3 install -r requirements.txt
```

### 2. MediaPipe 모델 파일 확인

`face_landmarker.task` 파일이 다음 경로 중 하나에 있어야 합니다:

- `../thirdparty/mediapipe/models/face_landmarker.task`
- `../../thirdparty/mediapipe/models/face_landmarker.task`
- 절대 경로: `/Users/jincheol/Desktop/VEDA/RtspProject/client_user/thirdparty/mediapipe/models/face_landmarker.task`

모델 파일이 없으면 [MediaPipe Face Landmarker 모델](https://developers.google.com/mediapipe/solutions/vision/face_landmarker)을 다운로드하세요.

## 사용 방법

### 직접 실행 (테스트)

```bash
python3 mediapipe_processor.py
```

stdin으로 JSON 명령을 입력하면 stdout으로 결과가 출력됩니다.

### C++에서 호출

`MediaPipeProcessor` 클래스가 자동으로 이 스크립트를 실행하고 통신합니다.

## 통신 프로토콜

### 입력 (stdin)

```json
{
  "action": "process",
  "image_data": "<base64_encoded_jpeg>",
  "width": 640,
  "height": 480
}
```

### 출력 (stdout)

```json
{
  "faces": [
    {
      "landmarks": [
        {"x": 0.5, "y": 0.5, "z": 0.0},
        ...
      ],
      "blendshapes": [
        {"category": "eyeBlinkLeft", "score": 0.8},
        {"category": "jawOpen", "score": 0.3},
        ...
      ]
    }
  ]
}
```

## Blendshape 카테고리

MediaPipe는 52개의 Blendshape를 제공합니다:

- `eyeBlinkLeft`, `eyeBlinkRight`
- `eyeLookDownLeft`, `eyeLookDownRight`
- `eyeLookInLeft`, `eyeLookInRight`
- `eyeLookOutLeft`, `eyeLookOutRight`
- `eyeLookUpLeft`, `eyeLookUpRight`
- `jawOpen`, `jawForward`, `jawLeft`, `jawRight`
- `mouthClose`, `mouthFunnel`, `mouthPucker`
- `mouthLeft`, `mouthRight`, `mouthRollLower`, `mouthRollUpper`
- `mouthShrugLower`, `mouthShrugUpper`
- `mouthSmileLeft`, `mouthSmileRight`
- `noseSneerLeft`, `noseSneerRight`
- 등등...

전체 목록은 [MediaPipe 문서](https://developers.google.com/mediapipe/solutions/vision/face_landmarker)를 참조하세요.
