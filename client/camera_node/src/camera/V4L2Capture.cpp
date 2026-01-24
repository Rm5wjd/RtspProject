#include "V4L2Capture.h"
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include <cstring>

// For older kernels that might not have these definitions
#ifndef V4L2_CID_MPEG_VIDEO_HEADER_MODE
#define V4L2_CID_MPEG_VIDEO_HEADER_MODE (V4L2_CID_MPEG_BASE + 351)
#endif
#ifndef V4L2_MPEG_VIDEO_HEADER_MODE_JOINED_WITH_1ST_FRAME
#define V4L2_MPEG_VIDEO_HEADER_MODE_JOINED_WITH_1ST_FRAME 1
#endif
#ifndef V4L2_CID_MPEG_VIDEO_H264_SPS_PPS_TO_IDR
#define V4L2_CID_MPEG_VIDEO_H264_SPS_PPS_TO_IDR (V4L2_CID_MPEG_BASE + 369)
#endif


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

    // 2. 포맷 설정 (H.264)
    struct v4l2_format fmt = {0};
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = width;
    fmt.fmt.pix.height = height;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_H264;
    fmt.fmt.pix.field = V4L2_FIELD_NONE;

    if (ioctl(fd, VIDIOC_S_FMT, &fmt) < 0) {
        perror("Setting Pixel Format");
        return false;
    }

    // Explicitly request the driver to prepend SPS/PPS to IDR frames.
    struct v4l2_control idr_header_ctrl = {0};
    idr_header_ctrl.id = V4L2_CID_MPEG_VIDEO_H264_SPS_PPS_TO_IDR;
    idr_header_ctrl.value = 1;
    if (ioctl(fd, VIDIOC_S_CTRL, &idr_header_ctrl) < 0) {
        perror("[V4L2] Warning: Failed to set SPS_PPS_TO_IDR mode");
    }

    // 3. 비트레이트 설정
    struct v4l2_control bitrate_ctrl = {0};
    bitrate_ctrl.id = V4L2_CID_MPEG_VIDEO_BITRATE;
    bitrate_ctrl.value = 4000000; // 4Mbps
    if (ioctl(fd, VIDIOC_S_CTRL, &bitrate_ctrl) < 0) {
        perror("Setting Bitrate");
    }
    std::cout << "[Camera] Format set: H.264 Compressed Stream (4Mbps)" << std::endl;

    // 4. 버퍼 요청
    struct v4l2_requestbuffers req = {0};
    req.count = 4;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    if (ioctl(fd, VIDIOC_REQBUFS, &req) < 0) {
        perror("Requesting Buffer");
        return false;
    }

    // 5. 메모리 매핑
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
    for (size_t i = 0; i < buffers.size(); ++i) {
        struct v4l2_buffer buf = {0};
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        if (ioctl(fd, VIDIOC_QBUF, &buf) < 0) return false;
    }

    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(fd, VIDIOC_STREAMON, &type) < 0) {
        perror("Start Capture");
        return false;
    }
    return true;
}

void V4L2Capture::stopCapture() {
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (fd != -1) {
        ioctl(fd, VIDIOC_STREAMOFF, &type);
    }
}

bool V4L2Capture::grabFrame(void** outData, size_t* outSize) {
    struct v4l2_buffer buf = {0};
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    if (ioctl(fd, VIDIOC_DQBUF, &buf) < 0) return false;

    currentBufferIndex = buf.index;
    *outData = buffers[buf.index].start;
    *outSize = buf.bytesused;

    return true;
}

void V4L2Capture::releaseFrame() {
    if (currentBufferIndex != -1) {
        struct v4l2_buffer buf = {0};
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = currentBufferIndex;
        
ioctl(fd, VIDIOC_QBUF, &buf);
        currentBufferIndex = -1;
    }
}