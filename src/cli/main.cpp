#include <iostream>
#include <string>
#include <cstdlib>

#include "CLI/CLI.hpp"
#include "core/compressor.h"
#include "cli/commands.h"

void print_version() {
    std::cout << "llmzip v1.0.0 - LLM-powered compression tool\n";
    std::cout << "Built with ONNX Runtime for cross-platform AI inference\n";
}

void print_usage(const char* program) {
    std::cout << "Usage: " << program << " <command> [options]\n\n";
    std::cout << "Commands:\n";
    std::cout << "  compress, c    Compress text to .lz format\n";
    std::cout << "  decode, d      Decompress .lz file to text\n";
    std::cout << "  stats, s       Show compression statistics\n";
    std::cout << "  version, v     Show version information\n\n";
    std::cout << "Examples:\n";
    std::cout << "  " << program << " compress input.txt output.lz -m lossy\n";
    std::cout << "  " << program << " decode archive.lz decoded.txt\n";
    std::cout << "  echo \"text\" | " << program << " compress - -o out.lz\n";
}

int main(int argc, char** argv) {
    // Быстрая проверка на версию
    if (argc >= 2) {
        std::string arg1 = argv[1];
        if (arg1 == "-v" || arg1 == "--version" || arg1 == "version") {
            print_version();
            return 0;
        }
        if (arg1 == "-h" || arg1 == "--help" || arg1 == "help") {
            print_usage(argv[0]);
            return 0;
        }
    }

    CLI::App app{"llmzip - LLM-powered compression tool"};
    app.set_help_flag("-h,--help", "Print help message");
    
    // Глобальные опции
    bool verbose = false;
    std::string model_path = "";
    
    app.add_flag("-v,--verbose", verbose, "Verbose output");
    app.add_option("--model", model_path, "Path to ONNX model file");

    // Команда compress
    auto compress_cmd = app.add_subcommand("compress", "Compress text to .lz format");
    
    std::string compress_input = "";
    std::string compress_output = "";
    std::string compress_mode = "lossy";
    
    compress_cmd->add_option("input", compress_input, "Input file (use '-' for stdin)")
                ->required(false);
    compress_cmd->add_option("-o,--output", compress_output, "Output file")
                ->required(true);
    compress_cmd->add_option("-m,--mode", compress_mode, "Compression mode: lossy, lossless, hybrid")
                ->check(CLI::IsMember({"lossy", "lossless", "hybrid"}));

    // Команда decode
    auto decode_cmd = app.add_subcommand("decode", "Decompress .lz file to text");
    
    std::string decode_input = "";
    std::string decode_output = "";
    
    decode_cmd->add_option("input", decode_input, "Input .lz file")
               ->required(true);
    decode_cmd->add_option("-o,--output", decode_output, "Output text file (use '-' for stdout)")
               ->required(false);

    // Команда stats
    auto stats_cmd = app.add_subcommand("stats", "Show compression statistics");
    
    std::string stats_file = "";
    stats_cmd->add_option("file", stats_file, ".lz file to analyze")
             ->required(true);

    CLI11_PARSE(app, argc, argv);

    llmzip::cli::CommandContext ctx;
    ctx.verbose = verbose;
    ctx.model_path = model_path;
    ctx.use_stdin = false;
    ctx.use_stdout = false;

    // Обработка команд
    if (*compress_cmd) {
        ctx.command = "compress";
        ctx.input_file = compress_input;
        ctx.output_file = compress_output;
        ctx.mode = compress_mode;
        ctx.use_stdin = (compress_input == "-");
        ctx.use_stdout = false;
        
        llmzip::cli::Commands commands;
        return commands.compress(ctx);
    }
    
    if (*decode_cmd) {
        ctx.command = "decode";
        ctx.input_file = decode_input;
        ctx.output_file = decode_output.empty() ? "-" : decode_output;
        ctx.use_stdout = (ctx.output_file == "-");
        
        llmzip::cli::Commands commands;
        return commands.decompress(ctx);
    }
    
    if (*stats_cmd) {
        ctx.command = "stats";
        ctx.input_file = stats_file;
        
        llmzip::cli::Commands commands;
        return commands.stats(ctx);
    }

    // Если команда не указана
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    std::cerr << "Unknown command: " << argv[1] << "\n";
    print_usage(argv[0]);
    return 1;
}
