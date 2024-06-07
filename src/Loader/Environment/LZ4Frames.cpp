
#include <lz4frame.h>

#include <array>
#include <istream>
#include <vector>

#include "../../Util/Log.h"

// Reference: https://gist.github.com/t-mat/c47868a00c60f93682d8
std::vector<uint8_t> decompressLz4Frames(std::istream& input) {
    LZ4F_dctx* dctx;
    size_t const dctx_status = LZ4F_createDecompressionContext(&dctx, LZ4F_VERSION);
    if (LZ4F_isError(dctx_status)) {
        PANIC("LZ4F_dctx creation error: " + std::string(LZ4F_getErrorName(dctx_status)));
    }

    std::array<uint8_t, 64 * 1024> src_buffer;
    std::array<uint8_t, 256 * 1024> dst_buffer;
    std::vector<uint8_t> output;

    while (!input.eof()) {
        input.read(reinterpret_cast<char*>(&src_buffer), src_buffer.size());
        size_t src_size = static_cast<size_t>(input.gcount());
        size_t src_offset = 0;

        while (src_offset < src_size) {
            uint8_t* src_ptr = &src_buffer[src_offset];
            size_t dst_size = dst_buffer.size();
            size_t src_remain = src_size - src_offset;
            size_t result = LZ4F_decompress(dctx, &dst_buffer, &dst_size, src_ptr, &src_remain, nullptr);
            if (LZ4F_isError(result)) {
                PANIC("LZ4F_decompress error: " + std::string(LZ4F_getErrorName(result)));
            }

            output.insert(output.end(), dst_buffer.begin(), dst_buffer.begin() + dst_size);
            src_offset += src_remain;

            if (result == 0) break;
        }
    }

    LZ4F_freeDecompressionContext(dctx);

    return output;
}