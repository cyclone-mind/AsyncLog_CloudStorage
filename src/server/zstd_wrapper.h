#pragma once

#include <zstd.h>
#include <string>
#include <vector>
#include <cassert>

namespace zstd_wrapper {


static std::string pack(const std::string& content, int compression_level = 3) {
    
    if (content.empty()) {
        return content;
    }
    
    size_t const compressed_size = ZSTD_compressBound(content.size());
    std::string compressed;
    compressed.resize(compressed_size);
    
    size_t const actual_size = ZSTD_compress(
        &compressed[0], compressed_size,
        content.data(), content.size(),
        compression_level
    );
    
    if (ZSTD_isError(actual_size)) {
        return "";
    }
    
    compressed.resize(actual_size);
    return compressed;
}

static std::string unpack(const std::string& content) {
    if (content.empty()) {
        return content;
    }
    

    unsigned long long const decompressed_size = ZSTD_getFrameContentSize(
        content.data(), content.size()
    );
    
    if (decompressed_size == ZSTD_CONTENTSIZE_ERROR) {
        return content;
    }
    
    if (decompressed_size == ZSTD_CONTENTSIZE_UNKNOWN) {
        std::string decompressed;
        decompressed.resize(content.size() * 4);
        
        size_t const actual_size = ZSTD_decompress(
            &decompressed[0], decompressed.size(),
            content.data(), content.size()
        );
        
        if (ZSTD_isError(actual_size)) {
            return content;
        }
        
        decompressed.resize(actual_size);
        return decompressed;
    }
    
    std::string decompressed;
    decompressed.resize(decompressed_size);
    
    size_t const actual_size = ZSTD_decompress(
        &decompressed[0], decompressed_size,
        content.data(), content.size()
    );
    
    if (ZSTD_isError(actual_size) || actual_size != decompressed_size) {
        return content;
    }
    
    return decompressed;
}



} // namespace zstd_wrapper