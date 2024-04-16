#pragma once

#include <istream>
#include <vector>

std::vector<uint8_t> decompressLz4Frames(std::istream& input);