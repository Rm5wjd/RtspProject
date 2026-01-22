#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>
#include <fcntl.h>      // open
#include <unistd.h>     // close
#include <sys/ioctl.h>  // ioctl
#include <sys/mman.h>   // mmap
#include <linux/videodev2.h>

// ---------------------------------------------------------
// V4L2Capture 클래스 (하나로 통합)
// ---------------------------------------------------------
class V4L2Capture {
private:
    struct Buffer {
        void* start;
        size_t length;
    };

    std::string deviceName;
    int fd = -1;
    std::vector<Buffer> buffers;
    int currentBufferIndex = -1;

public:
    V4L2Capture(const std::string& dev) : deviceName(dev) {}

    ~V4L2Capture() {
        stopCapture();
        if (fd != -1) close(fd);
    }

    bool init(int width, int height) {
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

        // [옵션] 비트레이트 설정 (4Mbps)
        struct v4l2_control ctrl = {0};
        ctrl.id = V4L2_CID_MPEG_VIDEO_BITRATE;
        ctrl.value = 4000000; 
        if (ioctl(fd, VIDIOC_S_CTRL, &ctrl) < 0) {
            perror("Setting Bitrate (Optional)");
        }
        std::cout << "[Camera] Format set: H.264 (4Mbps)" << std::endl;

        // 3. 버퍼 요청 (4중 버퍼링)
        struct v4l2_requestbuffers req = {0};
        req.count = 4;
        req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        req.memory = V4L2_MEMORY_MMAP;

        if (ioctl(fd, VIDIOC_REQBUFS, &req) < 0) {
            perror("Requesting Buffer");
            return false;
        }

        // 4. 메모리 매핑
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

    bool startCapture() {
        // 버퍼 큐잉
        for (size_t i = 0; i < buffers.size(); ++i) {
            struct v4l2_buffer buf = {0};
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_MMAP;
            buf.index = i;
            if (ioctl(fd, VIDIOC_QBUF, &buf) < 0) return false;
        }
        // 스트리밍 시작
        enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (ioctl(fd, VIDIOC_STREAMON, &type) < 0) {
            perror("Start Capture");
            return false;
        }
        return true;
    }

    void stopCapture() {
        if (fd != -1) {
            enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            ioctl(fd, VIDIOC_STREAMOFF, &type);
        }
    }

    bool grabFrame(void** outData, size_t* outSize) {
        struct v4l2_buffer buf = {0};
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;

        if (ioctl(fd, VIDIOC_DQBUF, &buf) < 0) return false;

        currentBufferIndex = buf.index;
        *outData = buffers[buf.index].start;
        *outSize = buf.bytesused; 
        return true;
    }

    void releaseFrame() {
        if (currentBufferIndex != -1) {
            struct v4l2_buffer buf = {0};
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_MMAP;
            buf.index = currentBufferIndex;
            ioctl(fd, VIDIOC_QBUF, &buf);
            currentBufferIndex = -1;
        }
    }
};

// ---------------------------------------------------------
// Main 실행 로직 (파일 저장)
// ---------------------------------------------------------
int main() {
    // 설정값
    const std::string devName = "/dev/video0";
    const std::string fileName = "output.h264";
    const int WIDTH = 1920;
    const int HEIGHT = 1080;
    const int FRAME_COUNT = 300; // 약 10초 녹화

    V4L2Capture capture(devName);
    
    // 초기화
    if (!capture.init(WIDTH, HEIGHT)) {
        std::cerr << "Init failed!" << std::endl;
        return -1;
    }

    // 캡처 시작
    if (!capture.startCapture()) {
        std::cerr << "Start failed!" << std::endl;
        return -1;
    }

    // 파일 열기 (Binary 모드)
    std::ofstream outFile(fileName, std::ios::binary);
    if (!outFile.is_open()) {
        std::cerr << "File open failed!" << std::endl;
        return -1;
    }

    std::cout << ">>> Recording " << FRAME_COUNT << " frames to " << fileName << "..." << std::endl;

    void* data = nullptr;
    size_t size = 0;
    int count = 0;

    // 녹화 루프
    while (count < FRAME_COUNT) {
        if (capture.grabFrame(&data, &size)) {
            // 파일 쓰기
            outFile.write((char*)data, size);
            
            // 버퍼 반환
            capture.releaseFrame();
            
            // 진행률 출력
            if (++count % 30 == 0) std::cout << "." << std::flush;
        }
    }

    std::cout << "\n>>> Done! Saved " << fileName << std::endl;
    outFile.close();
    
    return 0;
}
