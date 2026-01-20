#pragma once
#include <string>
#include <vector>
#include <cstdio>
#include <cstdint>

class H264Parser {
public:
    H264Parser(const std::string& filename);
    ~H264Parser();

    bool open();
    // 다음 NAL Unit을 읽어서 buffer에 담아줌. (Start Code 제외)
    // 리턴값: 읽은 데이터 크기 (0이면 파일 끝)
    int getNextNalu(std::vector<uint8_t>& buffer);

private:
    std::string filename;
    FILE* fp = nullptr;
    std::vector<uint8_t> internalBuffer; // 파일 읽기용 버퍼
};