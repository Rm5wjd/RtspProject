#!/bin/bash

# MediaPipe Protobuf 파일 재생성 스크립트
# 현재 시스템 protoc 버전으로 .pb.h 파일들을 재생성합니다

set -e

MEDIAPIPE_DIR="/Users/jincheol/mediapipe"
BAZEL_BIN="${MEDIAPIPE_DIR}/bazel-bin"
PROTO_DIR="${MEDIAPIPE_DIR}/mediapipe/framework/formats"
OUTPUT_DIR="${BAZEL_BIN}/mediapipe/framework/formats"

echo "=== MediaPipe Protobuf 파일 재생성 ==="
echo ""

# 1. protoc 버전 확인
echo "1. 현재 protoc 버전 확인:"
protoc --version
echo ""

# 2. 기존 .pb.h 파일 삭제 (호환성 문제 방지)
echo "2. 기존 .pb.h 파일 삭제:"
if [ -d "${OUTPUT_DIR}" ]; then
    rm -f "${OUTPUT_DIR}"/*.pb.h "${OUTPUT_DIR}"/*.pb.cc 2>/dev/null
    echo "  → 기존 파일 삭제 완료"
else
    echo "  → 출력 디렉토리가 없음 (새로 생성)"
fi

# 잘못된 경로에 생성된 파일도 삭제
WRONG_DIR="${BAZEL_BIN}/mediapipe/framework/formats/framework/formats"
if [ -d "${WRONG_DIR}" ]; then
    echo "  → 잘못된 경로의 파일 삭제: ${WRONG_DIR}"
    rm -rf "${WRONG_DIR}" 2>/dev/null
fi

# 3. 출력 디렉토리 생성
echo ""
echo "3. 출력 디렉토리 생성:"
mkdir -p "${OUTPUT_DIR}"
echo "  → ${OUTPUT_DIR}"
echo ""

# 4. 필요한 .proto 파일 목록
PROTO_FILES=(
    "classification.proto"
    "landmark.proto"
    "matrix_data.proto"
)

echo "4. 재생성할 .proto 파일들:"
for proto in "${PROTO_FILES[@]}"; do
    if [ -f "${PROTO_DIR}/${proto}" ]; then
        echo "  ✓ ${proto}"
    else
        echo "  ✗ ${proto} (not found)"
    fi
done
echo ""

# 5. 재생성 실행
echo "5. .pb.h 파일 재생성 중..."
for proto in "${PROTO_FILES[@]}"; do
    if [ -f "${PROTO_DIR}/${proto}" ]; then
        echo "  → ${proto}"
        
        # protoc 실행
        # proto_path는 mediapipe 디렉토리만 지정 (중복 경로 방지)
        protoc --cpp_out="${OUTPUT_DIR}" \
            --proto_path="${MEDIAPIPE_DIR}/mediapipe" \
            "${PROTO_DIR}/${proto}"
        
        if [ $? -eq 0 ]; then
            echo "    ✓ 성공"
        else
            echo "    ✗ 실패"
            exit 1
        fi
    fi
done
echo ""

# 6. 생성된 파일 확인
echo "6. 생성된 파일 확인:"
for proto in "${PROTO_FILES[@]}"; do
    pb_h="${OUTPUT_DIR}/${proto%.proto}.pb.h"
    if [ -f "${pb_h}" ]; then
        echo "  ✓ $(basename ${pb_h})"
    else
        echo "  ✗ $(basename ${pb_h}) (not found)"
    fi
done
echo ""

echo "7. 재생성 완료!"
echo ""
echo "다음 단계:"
echo "  cd /Users/jincheol/Desktop/VEDA/RtspProject/client_user/build"
echo "  rm -rf *"
echo "  cmake .."
echo "  make"
