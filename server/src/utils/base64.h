#pragma once

#include <string>
#include <vector>
#include <cstdint>

// Encodes a vector of bytes into a Base64 string.
std::string base64_encode(const std::vector<uint8_t>& data);
