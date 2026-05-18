#include "cli/commands.h"
#include "core/compressor.h"
#include "core/binary_format.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cstdio>
#include <chrono>
#include <ctime>

namespace llmzip {
namespace cli {

Commands::Commands() = default;
Commands::~Commands() = default;

int Commands::compress(const CommandContext& ctx) {
    // 1. Read input
    std::string input_text;
    if (ctx.use_stdin) {
        std::ostringstream oss;
        oss << std::cin.rdbuf();
        input_text = oss.str();
    } else {
        input_text = read_file(ctx.input_file);
    }

    if (input_text.empty()) {
        std::cerr << "Error: empty input\n";
        return 1;
    }

    print_progress("Compressing (" + ctx.mode + ")...", ctx.verbose);

    // 2. Create compressor
    Compressor compressor;
    if (!ctx.model_path.empty()) {
        print_progress("Loading model: " + ctx.model_path, ctx.verbose);
        if (!compressor.initialize(ctx.model_path)) {
            std::cerr << "Warning: model load failed, using protocol-only\n";
        }
    } else {
        compressor.initialize();
    }

    // 3. Compress
    CompressionMode mode = CompressionMode::LOSSY;
    if (ctx.mode == "lossless") mode = CompressionMode::LOSSLESS;
    else if (ctx.mode == "hybrid") mode = CompressionMode::HYBRID;

    auto result = compressor.compress(input_text, mode);

    if (!result.success) {
        std::cerr << "Compression failed: " << result.error_message << "\n";
        return 1;
    }

    // 4. Write output
    if (!write_binary(ctx.output_file, result.data)) {
        std::cerr << "Error: cannot write output\n";
        return 1;
    }

    // 5. Print stats
    std::cout << "\n✅ Compression complete!\n";
    std::cout << "   Original: " << result.original_size << " bytes\n";
    std::cout << "   Compressed: " << result.compressed_size << " bytes\n";
    std::cout << "   Ratio: " << std::fixed << std::setprecision(1)
              << (result.compression_ratio * 100.0) << "%\n";
    std::cout << "   Mode: " << result.mode << "\n";
    std::cout << "   Output: " << ctx.output_file << "\n";

    return 0;
}

int Commands::decompress(const CommandContext& ctx) {
    // 1. Read .lz file
    std::vector<uint8_t> data;
    if (ctx.use_stdin) {
        std::ostringstream oss;
        oss << std::cin.rdbuf();
        std::string s = oss.str();
        data.assign(s.begin(), s.end());
    } else {
        std::ifstream file(ctx.input_file, std::ios::binary);
        if (!file) {
            std::cerr << "Error: cannot open " << ctx.input_file << "\n";
            return 1;
        }
        data = std::vector<uint8_t>(
            std::istreambuf_iterator<char>(file), {});
    }

    if (data.empty()) {
        std::cerr << "Error: empty input\n";
        return 1;
    }

    print_progress("Decompressing...", ctx.verbose);

    // 2. Create compressor & decompress
    Compressor compressor;
    compressor.initialize();
    auto result = compressor.decompress(data);
    
    if (!result.success) {
        std::cerr << "Decompression failed: " << result.error_message << "\n";
        return 1;
    }

    // 3. Write output
    if (ctx.use_stdout) {
        std::cout << result.text;
        if (!result.text.empty() && result.text.back() != '\n')
            std::cout << "\n";
    } else {
        if (!write_text(ctx.output_file, result.text)) {
            std::cerr << "Error: cannot write output\n";
            return 1;
        }
    }

    std::cout << "\n✅ Decompression complete!\n";
    std::cout << "   Size: " << result.decompressed_size << " bytes\n";

    return 0;
}

int Commands::stats(const CommandContext& ctx) {
    // 1. Read file
    std::ifstream file(ctx.input_file, std::ios::binary);
    if (!file) {
        std::cerr << "Error: cannot open " << ctx.input_file << "\n";
        return 1;
    }
    std::vector<uint8_t> data(std::istreambuf_iterator<char>(file), {});

    if (data.size() < LZHeader::SIZE) {
        std::cerr << "Error: file too small or not a .lz file\n";
        return 1;
    }

    // 2. Parse header
    BinaryFormat binfmt;
    LZHeader header;
    if (!binfmt.deserialize_header(data, header)) {
        std::cerr << "Error: cannot parse .lz header\n";
        return 1;
    }

    bool valid = binfmt.validate_header(header);

    // 3. Print
    std::cout << "\n📊 .lz File Statistics\n";
    std::cout << "═══════════════════════════\n";
    std::cout << "  File:       " << ctx.input_file << "\n";
    std::cout << "  Size:       " << data.size() << " bytes\n";
    std::cout << "  Magic:      " << std::hex << header.magic << std::dec << "\n";
    std::cout << "  Version:    " << (int)header.version << "\n";
    std::cout << "  Type:       ";

    switch (header.type) {
        case LZCompressionType::LOSSY:    std::cout << "lossy\n";    break;
        case LZCompressionType::LOSSLESS: std::cout << "lossless\n"; break;
        case LZCompressionType::HYBRID:   std::cout << "hybrid\n";   break;
        default: std::cout << "unknown\n";
    }

    std::cout << "  Original:   " << header.original_size << " bytes\n";
    std::cout << "  Compressed: " << header.compressed_size << " bytes\n";

    if (header.original_size > 0) {
        double ratio = 1.0 - (double)header.compressed_size / (double)header.original_size;
        std::cout << "  Ratio:      " << std::fixed << std::setprecision(1)
                  << (ratio * 100.0) << "%\n";
    }

    std::cout << "  CRC:        " << (valid ? "✅ valid" : "❌ invalid") << "\n";

    return 0;
}

int Commands::pipe_mode(const CommandContext& ctx) {
    // Pipe: stdin → compress → stdout
    CommandContext pipe_ctx = ctx;
    pipe_ctx.use_stdin = true;
    pipe_ctx.use_stdout = true;
    pipe_ctx.output_file = "-";
    return compress(pipe_ctx);
}

// ---------------------------------------------------------------------------
// Private utilities
// ---------------------------------------------------------------------------
std::string Commands::read_file(const std::string& path) {
    std::ifstream file(path);
    if (!file) return {};
    std::ostringstream oss;
    oss << file.rdbuf();
    return oss.str();
}

bool Commands::write_binary(const std::string& path, const std::vector<uint8_t>& data) {
    std::ofstream file(path, std::ios::binary);
    if (!file) return false;
    file.write(reinterpret_cast<const char*>(data.data()), data.size());
    return file.good();
}

bool Commands::write_text(const std::string& path, const std::string& text) {
    std::ofstream file(path);
    if (!file) return false;
    file << text;
    return file.good();
}

void Commands::print_progress(const std::string& message, bool verbose) {
    if (verbose) {
        std::cout << "[llmzip] " << message << "...\n";
    }
}

} // namespace cli
} // namespace llmzip