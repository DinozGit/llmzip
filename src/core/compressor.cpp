#include "core/compressor.h"
#include "core/protocol_parser.h"
#include "core/binary_format.h"
#include "onnx/inference_engine.h"

#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <cstring>
#include <regex>
#include <zlib.h>

namespace llmzip {

// ============================================================================
// PIMPL
// ============================================================================
struct Compressor::Impl {
    ProtocolParser parser;
    BinaryFormat binfmt;
    std::unique_ptr<InferenceEngine> engine;

    bool onnx_available = false;
    std::string model_path;
};

// ============================================================================
// Public API
// ============================================================================
Compressor::Compressor()
    : pimpl_(std::make_unique<Impl>()) {
}

Compressor::~Compressor() = default;

bool Compressor::initialize(const std::string& model_path) {
    pimpl_->model_path = model_path;

    // Initialize ONNX — try file, or embedded model if path empty
    pimpl_->engine = std::make_unique<InferenceEngine>();
    if (model_path.empty()) {
        pimpl_->onnx_available = pimpl_->engine->initialize();  // empty path = try embedded
        if (!pimpl_->onnx_available) {
            std::cerr << "[Compressor] Embedded model not found, protocol-only mode\n";
        }
    } else {
        pimpl_->onnx_available = pimpl_->engine->initialize(model_path);
        if (!pimpl_->onnx_available) {
            std::cerr << "[Compressor] ONNX init failed for " << model_path
                      << ", falling back to protocol-only\n";
        }
    }

    // Protocol parser always initializes
    pimpl_->parser.load_default_rules();

    initialized_ = true;
    return true;
}

// ---------------------------------------------------------------------------
// compress – main compression pipeline
// ---------------------------------------------------------------------------
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
                result.compressed_size = payload.size();
                mode_str = "lossless";
                break;
            }
            case CompressionMode::LOSSY: {
                auto res = compress_lossy(input);
                payload.assign(res.data.begin(), res.data.end());
                result.compressed_size = payload.size();
                mode_str = "lossy";
                break;
            }
            case CompressionMode::HYBRID: {
                auto res = compress_hybrid(input);
                payload = std::move(res.data);
                result.compressed_size = payload.size();
                mode_str = "hybrid";
                break;
            }
        }

        result.mode = mode_str;

        // Pack into .lz
        LZCompressionType lz_type = LZCompressionType::LOSSLESS;
        if (mode == CompressionMode::LOSSY)
            lz_type = LZCompressionType::LOSSY;
        else if (mode == CompressionMode::HYBRID)
            lz_type = LZCompressionType::HYBRID;

        result.data = pimpl_->binfmt.pack_data(lz_type, input, payload);

        // Compression ratio
        if (result.original_size > 0) {
            result.compression_ratio = 1.0 - (double)result.compressed_size / (double)result.original_size;
        } else {
            result.compression_ratio = 0.0;
        }

        result.success = true;

    } catch (const std::exception& e) {
        result.error_message = "Compression error: " + std::string(e.what());
    }

    return result;
}

// ---------------------------------------------------------------------------
// decompress
// ---------------------------------------------------------------------------
DecompressionResult Compressor::decompress(const std::vector<uint8_t>& input) {
    DecompressionResult result;
    result.success = false;
    result.compressed_size = input.size();

    try {
        // Unpack .lz container
        LZHeader header;
        std::vector<uint8_t> payload;
        if (!pimpl_->binfmt.unpack_data(input, header, payload)) {
            result.error_message = "Invalid .lz data";
            return result;
        }

        result.compressed_size = header.compressed_size;

        // Convert payload to string
        std::string compressed_text(payload.begin(), payload.end());

        // Decompress based on type
        std::string decompressed;
        switch (header.type) {
            case LZCompressionType::LOSSLESS:
                decompressed = decompress_lossless_impl(compressed_text);
                break;

            case LZCompressionType::LOSSY:
                decompressed = decompress_lossy_impl(compressed_text);
                break;

            case LZCompressionType::HYBRID:
                decompressed = decompress_hybrid_impl(compressed_text);
                break;
        }

        result.text = decompressed;
        result.decompressed_size = decompressed.size();
        result.success = true;

    } catch (const std::exception& e) {
        result.error_message = "Decompression error: " + std::string(e.what());
    }

    return result;
}

DecompressionResult Compressor::decompress_file(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file) {
        DecompressionResult r;
        r.success = false;
        r.error_message = "Cannot open file: " + filepath;
        return r;
    }

    std::vector<uint8_t> data((std::istreambuf_iterator<char>(file)),
                               std::istreambuf_iterator<char>());
    return decompress(data);
}

// ---------------------------------------------------------------------------
// Private: lossless (zlib deflate)
// ---------------------------------------------------------------------------
CompressionResult Compressor::compress_lossy(const std::string& input) {
    CompressionResult result;
    result.original_size = input.size();

    // 1. Deterministic Protocol Compression (New Pipeline)
    std::string protocol_text = pimpl_->parser.compress_deterministic(input);

    // 2. If ONNX available, enhance with semantic compression (optional polish)
    if (pimpl_->onnx_available && pimpl_->engine) {
        auto onnx_result = pimpl_->engine->compress_text(protocol_text);
        // Safety: only use LLM output if it's valid and not much larger than skeleton
        if (onnx_result.success && !onnx_result.output_text.empty() && 
            onnx_result.output_text.size() < protocol_text.size() * 1.5) {
            std::cerr << "[lossy] LLM polish: " << protocol_text.size() << " B"
                      << " -> " << onnx_result.output_text.size() << " B\n";
            protocol_text = onnx_result.output_text;
        }
    }

    result.data.assign(protocol_text.begin(), protocol_text.end());
    result.compressed_size = result.data.size();
    result.mode = "lossy";
    if (result.original_size > 0)
        result.compression_ratio = 1.0 - (double)result.compressed_size / (double)result.original_size;
    result.success = true;

    return result;
}

CompressionResult Compressor::compress_lossless(const std::string& input) {
    CompressionResult result;
    result.original_size = input.size();

    // zlib deflate
    z_stream strm;
    memset(&strm, 0, sizeof(strm));
    deflateInit(&strm, Z_DEFAULT_COMPRESSION);

    size_t bound = deflateBound(&strm, input.size());
    std::vector<uint8_t> out(bound);

    strm.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(input.data()));
    strm.avail_in = input.size();
    strm.next_out = out.data();
    strm.avail_out = bound;

    deflate(&strm, Z_FINISH);
    size_t compressed_size = strm.total_out;
    deflateEnd(&strm);

    out.resize(compressed_size);
    result.data = std::move(out);
    result.compressed_size = compressed_size;
    result.mode = "lossless";
    if (result.original_size > 0)
        result.compression_ratio = 1.0 - (double)compressed_size / (double)result.original_size;
    result.success = true;

    return result;
}

CompressionResult Compressor::compress_hybrid(const std::string& input) {
    // 1. Protocol compress
    std::string protocol = pimpl_->parser.apply_rules(input, false);

    // 2. zlib on top
    z_stream strm;
    memset(&strm, 0, sizeof(strm));
    deflateInit(&strm, Z_DEFAULT_COMPRESSION);

    size_t bound = deflateBound(&strm, protocol.size());
    std::vector<uint8_t> out(bound);

    strm.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(protocol.data()));
    strm.avail_in = protocol.size();
    strm.next_out = out.data();
    strm.avail_out = bound;

    deflate(&strm, Z_FINISH);
    size_t compressed_size = strm.total_out;
    deflateEnd(&strm);

    out.resize(compressed_size);
    CompressionResult result;
    result.data = std::move(out);
    result.original_size = input.size();
    result.compressed_size = compressed_size;
    result.mode = "hybrid";
    if (result.original_size > 0)
        result.compression_ratio = 1.0 - (double)compressed_size / (double)result.original_size;
    result.success = true;

    return result;
}

// ---------------------------------------------------------------------------
// Private: decompress implementations
// ---------------------------------------------------------------------------
std::string Compressor::decompress_lossless_impl(const std::string& compressed) {
    // zlib inflate
    z_stream strm;
    memset(&strm, 0, sizeof(strm));
    inflateInit(&strm);

    std::vector<char> out(compressed.size() * 4 + 1024);
    strm.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(compressed.data()));
    strm.avail_in = compressed.size();
    strm.next_out = reinterpret_cast<Bytef*>(out.data());
    strm.avail_out = out.size();

    inflate(&strm, Z_FINISH);
    size_t decompressed_size = strm.total_out;
    inflateEnd(&strm);

    return std::string(out.data(), decompressed_size);
}

std::string Compressor::decompress_lossy_impl(const std::string& compressed) {
    // Reverse protocol
    return pimpl_->parser.apply_rules(compressed, true);
}

std::string Compressor::decompress_hybrid_impl(const std::string& compressed) {
    // Inflate first, then reverse protocol
    std::string inflated = decompress_lossless_impl(compressed);
    return pimpl_->parser.apply_rules(inflated, true);
}

// ---------------------------------------------------------------------------
// parse_header
// ---------------------------------------------------------------------------
bool Compressor::parse_header(const std::vector<uint8_t>& data, std::string& mode, std::vector<uint8_t>& payload) {
    LZHeader header;
    if (!pimpl_->binfmt.deserialize_header(data, header))
        return false;
    if (!pimpl_->binfmt.validate_header(header))
        return false;

    switch (header.type) {
        case LZCompressionType::LOSSY:    mode = "lossy"; break;
        case LZCompressionType::LOSSLESS: mode = "lossless"; break;
        case LZCompressionType::HYBRID:   mode = "hybrid"; break;
        default: mode = "unknown";
    }

    payload.assign(data.begin() + LZHeader::SIZE,
                   data.begin() + LZHeader::SIZE + header.compressed_size);
    return true;
}

} // namespace llmzip