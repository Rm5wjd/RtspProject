#include "V4L2Capture.h"
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include <cstring>

V4L2Capture::V4L2Capture(const std::string& dev) : deviceName(dev) {}

V4L2Capture::~V4L2Capture() {
    stopCapture();
    if (fd != -1) close(fd);
}

bool V4L2Capture::init(int width, int height) {
    // 1. 장치 열기
    fd = open(deviceName.c_str(), O_RDWR);
    if (fd < 0) {
        perror("Opening video device");
        return false;
    }

    // 2. 포맷 설정 (YUV420)
    struct v4l2_format fmt = {0};
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = width;
    fmt.fmt.pix.height = height;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_H264; // 라베파 기본 포맷
    fmt.fmt.pix.field = V4L2_FIELD_NONE;

    if (ioctl(fd, VIDIOC_S_FMT, &fmt) < 0) {
        perror("Setting Pixel Format");
        return false;
    }

    // [추가] 비트레이트 설정 (4Mbps = 4,000,000 bps)
    // Legacy 모드에서는 V4L2_CID_MPEG_VIDEO_BITRATE를 지원합니다.
    struct v4l2_control ctrl = {0};
    ctrl.id = V4L2_CID_MPEG_VIDEO_BITRATE;
    ctrl.value = 4000000; // 4Mbps (화질과 속도의 적절한 타협점)
    if (ioctl(fd, VIDIOC_S_CTRL, &ctrl) < 0) {
        perror("Setting Bitrate"); // 실패해도 치명적이진 않음 (기본값 사용)
    }

    std::cout << "[Camera] Format set: H.264 Compressed Stream (4Mbps)" << std::endl;

    // 3. 버퍼 요청 (커널에 메모리 할당 요청)
    struct v4l2_requestbuffers req = {0};
    req.count = 4; // 4중 버퍼링
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (ioctl(fd, VIDIOC_REQBUFS, &req) < 0) {
        perror("Requesting Buffer");
        return false;
    }

    // 4. 메모리 매핑 (Kernel Space -> User Space)
    buffers.resize(req.count);
    for (size_t i = 0; i < req.count; ++i) {
        struct v4l2_buffer buf = {0};
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        if (ioctl(fd, VIDIOC_QUERYBUF, &buf) < 0) return false;

        buffers[i].length = buf.length;
        buffers[i].start = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);

        if (buffers[i].start == MAP_FAILED) return false;
    }
    return true;
}

bool V4L2Capture::startCapture() {
    // 1. 모든 버퍼를 큐에 넣기 (Capture 준비)
    for (size_t i = 0; i < buffers.size(); ++i) {
        struct v4l2_buffer buf = {0};
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        if (ioctl(fd, VIDIOC_QBUF, &buf) < 0) return false;
    }

    // 2. 스트리밍 시작
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(fd, VIDIOC_STREAMON, &type) < 0) {
        perror("Start Capture");
        return false;
    }
    return true;
}

void V4L2Capture::stopCapture() {
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ioctl(fd, VIDIOC_STREAMOFF, &type);
    // 문 닫을 때 munmap 해주는 게 정석이지만 생략
}

bool V4L2Capture::grabFrame(void** outData, size_t* outSize) {
    struct v4l2_buffer buf = {0};
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    // 큐에서 다 찍힌 프레임 하나 꺼내오기 (Dequeue)
    if (ioctl(fd, VIDIOC_DQBUF, &buf) < 0) return false;

    currentBufferIndex = buf.index;
    *outData = buffers[buf.index].start;
    *outSize = buf.bytesused; // 실제 데이터 크기

    return true;
}

void V4L2Capture::releaseFrame() {
    if (currentBufferIndex != -1) {
        struct v4l2_buffer buf = {0};
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = currentBufferIndex;
        
        // 다 썼으니 다시 채워달라고 큐에 넣기 (Queue)
        ioctl(fd, VIDIOC_QBUF, &buf);
        currentBufferIndex = -1;
    }
}