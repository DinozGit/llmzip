#include "core/compressor.h"
#include "core/protocol_parser.h"
#include "core/binary_format.h"
#include "onnx/inference_engine.h"

#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <cstring>
#include <zlib.h>

namespace llmzip {

struct Compressor::Impl {
    ProtocolParser parser;
    BinaryFormat binfmt;
    std::unique_ptr<InferenceEngine> engine;

    bool onnx_available = false;
    std::string model_path;
};

Compressor::Compressor() : pimpl_(std::make_unique<Impl>()) {}
Compressor::~Compressor() = default;

bool Compressor::initialize(const std::string& model_path) {
    pimpl_->model_path = model_path;
    pimpl_->engine = std::make_unique<InferenceEngine>();
    pimpl_->onnx_available = pimpl_->engine->initialize(model_path);
    pimpl_->parser.load_default_rules();
    initialized_ = true;
    return true;
}

CompressionResult Compressor::compress(const std::string& input, CompressionMode mode) {
    CompressionResult result;
    result.original_size = input.size();
    result.success = false;

    if (!initialized_) {
        result.error_message = "Compressor not initialized";
        return result;
    }

    std::vector<uint8_t> payload;
    std::string mode_str;

    try {
        switch (mode) {
            case CompressionMode::LOSSLESS: {
                auto res = compress_lossless(input);
                payload = std::move(res.data);
                mode_str = "lossless";
                break;
            }
            case CompressionMode::LOSSY: {
                auto res = compress_lossy(input);
                payload.assign(res.data.begin(), res.data.end());
                mode_str = "lossy";
                break;
            }
            case CompressionMode::HYBRID: {
                auto res = compress_hybrid(input);
                payload = std::move(res.data);
                mode_str = "hybrid";
                break;
            }
        }

        result.mode = mode_str;
        LZCompressionType lz_type = (mode == CompressionMode::LOSSY) ? LZCompressionType::LOSSY : 
                                    (mode == CompressionMode::HYBRID ? LZCompressionType::HYBRID : LZCompressionType::LOSSLESS);

        result.data = pimpl_->binfmt.pack_data(lz_type, input, payload);
        result.compressed_size = payload.size();
        result.compression_ratio = result.original_size > 0 ? 1.0 - (double)result.compressed_size / (double)result.original_size : 0.0;
        result.success = true;

    } catch (const std::exception& e) {
        result.error_message = "Compression error: " + std::string(e.what());
    }

    return result;
}

DecompressionResult Compressor::decompress(const std::vector<uint8_t>& input) {
    DecompressionResult result;
    result.success = false;
    try {
        LZHeader header;
        std::vector<uint8_t> payload;
        if (!pimpl_->binfmt.unpack_data(input, header, payload)) {
            result.error_message = "Invalid .lz data";
            return result;
        }

        std::string compressed_text(payload.begin(), payload.end());
        std::string decompressed;
        switch (header.type) {
            case LZCompressionType::LOSSLESS: decompressed = decompress_lossless_impl(compressed_text); break;
            case LZCompressionType::LOSSY:    decompressed = decompress_lossy_impl(compressed_text); break;
            case LZCompressionType::HYBRID:   decompressed = decompress_hybrid_impl(compressed_text); break;
        }

        result.text = decompressed;
        result.compressed_size = header.compressed_size;
        result.decompressed_size = decompressed.size();
        result.success = true;
    } catch (const std::exception& e) {
        result.error_message = "Decompression error: " + std::string(e.what());
    }
    return result;
}

DecompressionResult Compressor::decompress_file(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file) return { "", 0, 0, false, "Cannot open file" };
    std::vector<uint8_t> data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return decompress(data);
}

CompressionResult Compressor::compress_lossy(const std::string& input) {
    CompressionResult result;
    // 1. Deterministic Semantic Compression (Level 1)
    std::string protocol_text = pimpl_->parser.compress_deterministic(input);

    // 2. LLM Polish (Level 2 - Optional Stub)
    if (pimpl_->onnx_available && pimpl_->engine) {
        auto onnx_result = pimpl_->engine->compress_text(protocol_text);
        if (onnx_result.success && onnx_result.output_text.size() < protocol_text.size()) {
            protocol_text = onnx_result.output_text;
        }
    }

    result.data.assign(protocol_text.begin(), protocol_text.end());
    result.success = true;
    return result;
}

CompressionResult Compressor::compress_lossless(const std::string& input) {
    CompressionResult result;
    z_stream strm;
    memset(&strm, 0, sizeof(strm));
    deflateInit(&strm, Z_DEFAULT_COMPRESSION);
    size_t bound = deflateBound(&strm, input.size());
    std::vector<uint8_t> out(bound);
    strm.next_in = (Bytef*)input.data();
    strm.avail_in = input.size();
    strm.next_out = out.data();
    strm.avail_out = bound;
    deflate(&strm, Z_FINISH);
    out.resize(strm.total_out);
    deflateEnd(&strm);
    result.data = std::move(out);
    result.success = true;
    return result;
}

CompressionResult Compressor::compress_hybrid(const std::string& input) {
    std::string protocol = pimpl_->parser.compress_deterministic(input);
    return compress_lossless(protocol);
}

std::string Compressor::decompress_lossless_impl(const std::string& compressed) {
    z_stream strm;
    memset(&strm, 0, sizeof(strm));
    inflateInit(&strm);
    std::vector<char> out(compressed.size() * 10 + 4096);
    strm.next_in = (Bytef*)compressed.data();
    strm.avail_in = compressed.size();
    strm.next_out = (Bytef*)out.data();
    strm.avail_out = out.size();
    inflate(&strm, Z_FINISH);
    size_t sz = strm.total_out;
    inflateEnd(&strm);
    return std::string(out.data(), sz);
}

std::string Compressor::decompress_lossy_impl(const std::string& compressed) {
    return pimpl_->parser.apply_rules(compressed, true);
}

std::string Compressor::decompress_hybrid_impl(const std::string& compressed) {
    return decompress_lossy_impl(decompress_lossless_impl(compressed));
}

} // namespace llmzip
