"""
MediaPipe Python 모듈 - C++에서 Python C API로 직접 임베딩하여 호출

이 모듈은 C++ 애플리케이션에서 Python C API를 통해 직접 호출됩니다.
MediaPipe FaceLandmarker를 사용하여 얼굴 탐지 및 Blendshape 추출을 수행합니다.

사용 방법:
    1. initialize(model_path)로 모델 초기화
    2. process_frame(image_array, width, height)로 프레임 처리
    3. 결과는 dict 형태로 반환 (또는 None)
"""
import numpy as np
import cv2
import mediapipe as mp
from mediapipe.tasks import python
from mediapipe.tasks.python import vision

# ============================================================================
# 전역 변수
# ============================================================================
# MediaPipe FaceLandmarker 인스턴스 (모듈 로드 시 한 번만 초기화)
_detector = None

def reset():
    """
    FaceLandmarker 인스턴스를 해제합니다.

    웹캠/RTSP를 끊었다가 다시 연결할 때, 내부 상태가 꼬이는 경우를 방지하기 위해
    C++에서 start() 시점에 reset() → initialize() 순서로 호출하는 것을 권장합니다.
    """
    global _detector
    _detector = None
    return True

def initialize(model_path):
    """
    MediaPipe FaceLandmarker 초기화
    
    Args:
        model_path (str): face_landmarker.task 모델 파일의 경로
    
    Returns:
        bool: 초기화 성공 여부 (항상 True, 실패 시 예외 발생)
    
    Raises:
        Exception: 모델 파일을 찾을 수 없거나 로드 실패 시
    """
    global _detector
    
    # BaseOptions 설정 (모델 파일 경로 지정)
    base_options = python.BaseOptions(model_asset_path=model_path)
    
    # FaceLandmarkerOptions 설정
    options = vision.FaceLandmarkerOptions(
        base_options=base_options,
        output_face_blendshapes=True,  # Blendshape 출력 활성화 (52개 값)
        output_facial_transformation_matrixes=True,  # 변환 행렬 출력
        num_faces=1,  # 단일 얼굴만 처리 (성능 향상)
        min_face_detection_confidence=0.5,  # 얼굴 탐지 최소 신뢰도
        min_face_presence_confidence=0.5,   # 얼굴 존재 최소 신뢰도
        min_tracking_confidence=0.5         # 추적 최소 신뢰도
    )
    
    # FaceLandmarker 인스턴스 생성
    _detector = vision.FaceLandmarker.create_from_options(options)
    return True

def process_frame(image_array, width, height):
    """
    프레임 처리: 얼굴 탐지 및 Blendshape 추출
    
    Args:
        image_array (numpy.ndarray): BGR 형식의 이미지 배열 (shape: (height, width, 3))
        width (int): 이미지 너비 (현재 사용 안 함, 호환성을 위해 유지)
        height (int): 이미지 높이 (현재 사용 안 함, 호환성을 위해 유지)
    
    Returns:
        dict | None: 얼굴 탐지 결과
            - 성공 시:
                {
                    "landmarks": [[x, y, z], ...],  # 478개 랜드마크 (정규화된 좌표 0-1)
                    "blendshapes": [
                        {"category": str, "score": float}, ...  # 52개 Blendshape 값
                    ]
                }
            - 실패 시: None (얼굴이 탐지되지 않았거나 오류 발생)
    
    Note:
        - 랜드마크 좌표는 정규화된 값 (0.0 ~ 1.0)
        - Blendshape score는 0.0 ~ 1.0 범위
        - 첫 번째 얼굴만 처리 (num_faces=1 설정)
    """
    global _detector
    
    # 초기화 확인
    if _detector is None:
        return None
    
    try:
        # ====================================================================
        # 1. 이미지 형식 변환 (BGR → RGB)
        # ====================================================================
        # OpenCV는 BGR 형식을 사용하지만, MediaPipe는 RGB를 사용
        img_rgb = cv2.cvtColor(image_array, cv2.COLOR_BGR2RGB)
        
        # ====================================================================
        # 2. MediaPipe ImageFormat으로 변환
        # ====================================================================
        mp_image = mp.Image(image_format=mp.ImageFormat.SRGB, data=img_rgb)
        
        # ====================================================================
        # 3. 얼굴 탐지 및 랜드마크 추출
        # ====================================================================
        detection_result = _detector.detect(mp_image)
        
        # 얼굴이 탐지되지 않은 경우
        if not detection_result.face_landmarks or len(detection_result.face_landmarks) == 0:
            return None
        
        # ====================================================================
        # 4. 첫 번째 얼굴 데이터 추출
        # ====================================================================
        landmarks = detection_result.face_landmarks[0]
        
        result = {
            "landmarks": [],
            "blendshapes": []
        }
        
        # ====================================================================
        # 5. 랜드마크 좌표 추출 (478개)
        # ====================================================================
        # 각 랜드마크는 정규화된 좌표 (x, y, z)를 가짐
        # x, y: 0.0 ~ 1.0 (이미지 내 상대 위치)
        # z: 깊이 정보 (얼굴의 앞뒤)
        for landmark in landmarks:
            result["landmarks"].append([
                landmark.x,  # 정규화된 X 좌표
                landmark.y,  # 정규화된 Y 좌표
                landmark.z   # 깊이 정보
            ])
        
        # ====================================================================
        # 6. Blendshape 데이터 추출 (52개)
        # ====================================================================
        # Blendshape는 얼굴 표정 및 움직임을 나타내는 값들
        # 예: eyeBlinkLeft, jawOpen, mouthSmileLeft 등
        if (detection_result.face_blendshapes and 
            len(detection_result.face_blendshapes) > 0):
            blendshapes = detection_result.face_blendshapes[0]
            for blendshape in blendshapes:
                result["blendshapes"].append({
                    "category": blendshape.category_name,  # Blendshape 이름
                    "score": float(blendshape.score)       # 값 (0.0 ~ 1.0)
                })
        
        return result
        
    except Exception as e:
        # 오류 발생 시 로그 출력 및 None 반환
        print(f"Error in process_frame: {e}")
        import traceback
        traceback.print_exc()
        return None
