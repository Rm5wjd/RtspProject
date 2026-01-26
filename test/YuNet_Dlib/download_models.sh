#!/bin/bash

# 모델 다운로드 스크립트

echo "=== 모델 다운로드 스크립트 ==="
echo ""

# resource 디렉토리 확인 및 생성
RESOURCE_DIR="/Users/jincheol/Desktop/VEDA/RtspProject/resource"
mkdir -p "$RESOURCE_DIR"
cd "$RESOURCE_DIR"

echo "1. YuNet 얼굴 탐지 모델 다운로드 중..."
YUNET_URL="https://github.com/opencv/opencv_zoo/raw/master/models/face_detection_yunet/face_detection_yunet_2023mar.onnx"
YUNET_FILE="face_detection_yunet_2023mar.onnx"

if [ -f "$YUNET_FILE" ]; then
    echo "   ✓ YuNet 모델이 이미 존재합니다: $YUNET_FILE"
else
    echo "   다운로드 중: $YUNET_URL"
    curl -L -o "$YUNET_FILE" "$YUNET_URL"
    if [ $? -eq 0 ]; then
        echo "   ✓ YuNet 모델 다운로드 완료: $YUNET_FILE"
        ls -lh "$YUNET_FILE"
    else
        echo "   ✗ YuNet 모델 다운로드 실패"
    fi
fi

echo ""
echo "2. dlib 얼굴 특징점 모델 확인 중..."
DLIB_DIR="/Users/jincheol/Desktop/VEDA/RtspProject/client_user/thirdparty/dlib/models"
DLIB_FILE="$DLIB_DIR/shape_predictor_68_face_landmarks.dat"

if [ -f "$DLIB_FILE" ]; then
    echo "   ✓ dlib 모델이 이미 존재합니다: $DLIB_FILE"
    ls -lh "$DLIB_FILE"
else
    echo "   ✗ dlib 모델이 없습니다"
    echo "   다운로드: http://dlib.net/files/shape_predictor_68_face_landmarks.dat.bz2"
    echo "   압축 해제 후 다음 경로에 배치: $DLIB_DIR"
    echo ""
    echo "   수동 다운로드 명령어:"
    echo "   cd $DLIB_DIR"
    echo "   curl -L -o shape_predictor_68_face_landmarks.dat.bz2 \\"
    echo "     http://dlib.net/files/shape_predictor_68_face_landmarks.dat.bz2"
    echo "   bunzip2 shape_predictor_68_face_landmarks.dat.bz2"
fi

echo ""
echo "=== 다운로드 완료 ==="
