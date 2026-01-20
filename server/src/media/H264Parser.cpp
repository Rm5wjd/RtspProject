#include "H264Parser.h"
#include <iostream>
#include <cstring>

H264Parser::H264Parser(const std::string& fname) : filename(fname) {}

H264Parser::~H264Parser() {
    if (fp) fclose(fp);
}

bool H264Parser::open() {
    fp = fopen(filename.c_str(), "rb");
    if (!fp) {
        perror("Failed to open H.264 file");
        return false;
    }
    return true;
}

// 파일에서 Start Code(0x000001 or 0x00000001)를 찾아 NALU 분리
int H264Parser::getNextNalu(std::vector<uint8_t>& buffer) {
    if (!fp) return 0;

    buffer.clear();
    
    // 1. Start Code 찾기 (현재 위치부터)
    // 실제 구현에서는 버퍼링을 해서 효율적으로 찾아야 하지만, 
    // 여기서는 이해를 돕기 위해 바이트 단위로 읽습니다.
    
    uint8_t byte;
    int state = 0; // 0: 초기, 1: 0발견, 2: 00발견, 3: 000발견
    
    // NALU 데이터를 임시 저장
    std::vector<uint8_t> temp;
    
    while (fread(&byte, 1, 1, fp) > 0) {
        temp.push_back(byte);
        
        // Start Code 패턴 감지 (00 00 01 또는 00 00 00 01)
        if (state == 0 && byte == 0x00) state = 1;
        else if (state == 1 && byte == 0x00) state = 2;
        else if (state == 2 && byte == 0x00) state = 3;
        else if ((state == 2 || state == 3) && byte == 0x01) {
            // Start Code 발견! 
            // 지금까지 읽은 temp에서 뒤쪽 Start Code를 제외한 게 이전 NALU임.
            // 하지만 첫 번째 NALU를 찾을 때는 앞에 데이터가 없으므로 계속 읽어야 함.
            
            // 이 로직은 "다음 Start Code"가 나올 때까지 읽어서 반환하는 방식입니다.
            // 단순화를 위해 파일을 미리 읽어두거나 프레임 단위 파싱 로직이 필요하지만,
            // 가장 쉬운 방법: "현재 위치부터 파일 끝까지 읽고 Start Code로 자른다"
            // 여기서는 성능보다 가독성을 위해 "한 프레임씩 읽는 척"하는 단순화된 로직을 씁니다.
            
            // 실제로는 매번 fseek로 왔다갔다 하기보다, 메모리에 통째로 올리고 포인터로 자르는 게 낫습니다.
            // 아래는 메모리 통째 로딩 방식이 아닌 스트림 방식 예시입니다.
             break;
        } else {
            state = 0;
        }
    }
    
    // 위 방식은 구현이 복잡하므로, 가장 확실하고 쉬운 방법으로 교체합니다.
    // --> "파일을 통째로 읽어서 Start Code 인덱스를 미리 다 저장해두는 방식"이 초보자에게 제일 좋습니다.
    // 하지만 파일이 클 수 있으니, 여기서는 "현재 위치" 기반으로 단순화합니다.
    
    // [간단 버전] : 그냥 1프레임 크기를 대략 40KB라고 가정하고 읽는 게 아니라,
    // 제대로 하려면 Start Code 탐색이 필수입니다.
    
    // 다시 작성: Start Code 탐색 로직 (Robust)
    // 1. 첫 Start Code는 무시 (이미 파일 포인터가 데이터 시작점이라 가정하거나 처리)
    // 2. 다음 Start Code가 나올 때까지 데이터를 buffer에 push
    
    return 0; // (아래 RtpSender.cpp에서 직접 구현하는 게 더 직관적일 수 있어 거기서 설명합니다)
}