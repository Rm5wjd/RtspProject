#include "media/StreamBuffer.h"

void StreamBuffer::push(std::vector<uint8_t>&& nalu) {
    std::lock_guard<std::mutex> lock(mutex_);
    buffer_.push_back(std::move(nalu));
    cv_.notify_one(); // 데이터가 추가되었음을 알림
}

std::vector<uint8_t> StreamBuffer::pop() {
    std::unique_lock<std::mutex> lock(mutex_);
    // 버퍼가 비어있으면 데이터가 들어올 때까지 대기
    cv_.wait(lock, [this] { return !buffer_.empty(); });

    std::vector<uint8_t> nalu = std::move(buffer_.front());
    buffer_.pop_front();
    return nalu;
}

void StreamBuffer::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    buffer_.clear();

    std::lock_guard<std::mutex> sps_lock(sps_pps_mutex_);
    sps_.clear();
    pps_.clear();
}

// --- New implementations ---

void StreamBuffer::setSps(const std::vector<uint8_t>& sps) {
    std::lock_guard<std::mutex> lock(sps_pps_mutex_);
    sps_ = sps;
}

void StreamBuffer::setPps(const std::vector<uint8_t>& pps) {
    std::lock_guard<std::mutex> lock(sps_pps_mutex_);
    pps_ = pps;
}

std::vector<uint8_t> StreamBuffer::getSps() {
    std::lock_guard<std::mutex> lock(sps_pps_mutex_);
    return sps_;
}

std::vector<uint8_t> StreamBuffer::getPps() {
    std::lock_guard<std::mutex> lock(sps_pps_mutex_);
    return pps_;
}

bool StreamBuffer::hasSpsPps() {
    std::lock_guard<std::mutex> lock(sps_pps_mutex_);
    return !sps_.empty() && !pps_.empty();
}
