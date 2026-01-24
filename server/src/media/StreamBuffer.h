#pragma once
#include <vector>
#include <list>
#include <mutex>
#include <condition_variable>
#include <cstdint>

class StreamBuffer {
public:
    void push(std::vector<uint8_t>&& nalu);
    std::vector<uint8_t> pop();
    void clear();

    // New methods for SPS/PPS
    void setSps(const std::vector<uint8_t>& sps);
    void setPps(const std::vector<uint8_t>& pps);
    std::vector<uint8_t> getSps();
    std::vector<uint8_t> getPps();
    bool hasSpsPps();

private:
    std::list<std::vector<uint8_t>> buffer_;
    std::mutex mutex_;
    std::condition_variable cv_;

    // New members for SPS/PPS
    std::vector<uint8_t> sps_;
    std::vector<uint8_t> pps_;
    std::mutex sps_pps_mutex_; // Separate mutex for SPS/PPS
};
