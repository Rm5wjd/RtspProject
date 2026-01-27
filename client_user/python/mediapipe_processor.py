#!/usr/bin/env python3
"""
MediaPipe Face Detection and Blendshape Extraction
Python 사이드카 프로세서 - C++에서 호출되어 프레임을 처리하고 결과를 JSON으로 반환
"""

import sys
import json
import base64
import numpy as np
import cv2
import mediapipe as mp
from mediapipe.tasks import python
from mediapipe.tasks.python import vision

class MediaPipeProcessor:
    def __init__(self):
        """MediaPipe FaceLandmarker 초기화"""
        # 모델 경로 찾기
        model_path = self._find_model_path()
        if not model_path:
            print(json.dumps({"error": "Model file not found"}), file=sys.stderr)
            sys.exit(1)
        
        # FaceLandmarker 옵션 설정
        base_options = python.BaseOptions(model_asset_path=model_path)
        options = vision.FaceLandmarkerOptions(
            base_options=base_options,
            output_face_blendshapes=True,  # Blendshape 활성화
            output_facial_transformation_matrixes=True,
            num_faces=1,  # 단일 얼굴만 처리
            min_face_detection_confidence=0.5,
            min_face_presence_confidence=0.5,
            min_tracking_confidence=0.5
        )
        
        self.detector = vision.FaceLandmarker.create_from_options(options)
        print(json.dumps({"status": "initialized", "model": model_path}), file=sys.stderr)
    
    def _find_model_path(self):
        """MediaPipe 모델 파일 경로 찾기"""
        import os
        possible_paths = [
            "../thirdparty/mediapipe/models/face_landmarker.task",
            "../../thirdparty/mediapipe/models/face_landmarker.task",
            "thirdparty/mediapipe/models/face_landmarker.task",
            os.path.join(os.path.dirname(__file__), "../../thirdparty/mediapipe/models/face_landmarker.task"),
            "/Users/jincheol/Desktop/VEDA/RtspProject/client_user/thirdparty/mediapipe/models/face_landmarker.task"
        ]
        
        for path in possible_paths:
            if os.path.exists(path):
                return os.path.abspath(path)
        return None
    
    def process_frame(self, image_data_base64, width, height):
        """
        프레임 처리: 얼굴 탐지 및 Blendshape 추출
        
        Args:
            image_data_base64: Base64 인코딩된 이미지 데이터
            width: 이미지 너비
            height: 이미지 높이
        
        Returns:
            JSON 문자열: 얼굴 랜드마크 및 Blendshape 데이터
        """
        try:
            # Base64 디코딩
            image_bytes = base64.b64decode(image_data_base64)
            nparr = np.frombuffer(image_bytes, np.uint8)
            img = cv2.imdecode(nparr, cv2.IMREAD_COLOR)
            
            if img is None:
                return json.dumps({"error": "Failed to decode image"})
            
            # RGB로 변환 (MediaPipe는 RGB 사용)
            img_rgb = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
            
            # MediaPipe ImageFormat으로 변환
            mp_image = mp.Image(image_format=mp.ImageFormat.SRGB, data=img_rgb)
            
            # 얼굴 탐지 및 랜드마크 추출
            detection_result = self.detector.detect(mp_image)
            
            # 결과 파싱
            result = {
                "faces": [],
                "timestamp": None
            }
            
            if detection_result.face_landmarks:
                for i, landmarks in enumerate(detection_result.face_landmarks):
                    face_data = {
                        "landmarks": [],
                        "blendshapes": []
                    }
                    
                    # 랜드마크 좌표 (정규화된 좌표: 0.0 ~ 1.0)
                    for landmark in landmarks:
                        face_data["landmarks"].append({
                            "x": landmark.x,
                            "y": landmark.y,
                            "z": landmark.z
                        })
                    
                    # Blendshape 데이터 (52개)
                    if (detection_result.face_blendshapes and 
                        i < len(detection_result.face_blendshapes)):
                        blendshapes = detection_result.face_blendshapes[i]
                        for blendshape in blendshapes:
                            face_data["blendshapes"].append({
                                "category": blendshape.category_name,
                                "score": blendshape.score
                            })
                    
                    result["faces"].append(face_data)
            
            return json.dumps(result)
            
        except Exception as e:
            return json.dumps({"error": str(e)})


def main():
    """메인 함수: stdin에서 프레임 데이터를 읽고 stdout으로 결과 출력"""
    processor = MediaPipeProcessor()
    
    # stdin에서 명령 읽기
    for line in sys.stdin:
        line = line.strip()
        if not line:
            continue
        
        try:
            # JSON 명령 파싱
            command = json.loads(line)
            
            if command.get("action") == "process":
                # 프레임 처리
                image_data = command.get("image_data")
                width = command.get("width")
                height = command.get("height")
                
                if not image_data or not width or not height:
                    print(json.dumps({"error": "Invalid command parameters"}), file=sys.stderr)
                    continue
                
                result = processor.process_frame(image_data, width, height)
                print(result, flush=True)  # stdout으로 결과 출력
                
            elif command.get("action") == "exit":
                break
                
        except json.JSONDecodeError:
            print(json.dumps({"error": "Invalid JSON"}), file=sys.stderr)
            continue
        except Exception as e:
            print(json.dumps({"error": str(e)}), file=sys.stderr)
            continue


if __name__ == "__main__":
    main()
