#include <iostream>
#include <string>
#include <cstdlib>

#include "CLI/CLI.hpp"
#include "core/compressor.h"
#include "cli/commands.h"

void print_version() {
    std::cout << "llmzip v1.0.0 - LLM-powered semantic compression tool\n";
    std::cout << "Built with ONNX Runtime & ProtocolParser engine\n";
}

int main(int argc, char** argv) {
    CLI::App app{"llmzip - High-performance semantic text compression"};
    
    // Настройка глобального поведения
    app.set_help_flag("-h,--help", "Print this help message");
    app.footer("\nExamples:\n"
               "  llmzip c input.txt                # Compress to input.txt.lz\n"
               "  llmzip c input.txt out.lz -m lossy # Compress with specific mode\n"
               "  llmzip d archive.lz output.txt     # Decompress to file\n"
               "  llmzip s archive.lz                # Show statistics\n");

    // Глобальные опции
    bool verbose = false;
    std::string model_path = "";
    bool show_version = false;
    
    app.add_flag("-v,--verbose", verbose, "Enable verbose output");
    app.add_option("--model", model_path, "Path to custom ONNX model file");
    app.add_flag("--version", show_version, "Show version information");

    // Команда compress
    auto compress_cmd = app.add_subcommand("compress", "Compress text to .lz format");
    compress_cmd->alias("c");
    
    std::string compress_input = "";
    std::string compress_output = "";
    std::string compress_mode = "lossy";
    
    compress_cmd->add_option("input", compress_input, "Input file (use '-' for stdin)")
                ->required(true);
    compress_cmd->add_option("output", compress_output, "Output .lz file (optional)");
    compress_cmd->add_option("-o,--output-flag", compress_output, "Output file (flag alias)");
    compress_cmd->add_option("-m,--mode", compress_mode, "Mode: lossy (default), lossless, hybrid")
                ->check(CLI::IsMember({"lossy", "lossless", "hybrid"}));

    // Команда decode
    auto decode_cmd = app.add_subcommand("decode", "Decompress .lz file to text");
    decode_cmd->alias("d");
    
    std::string decode_input = "";
    std::string decode_output = "";
    
    decode_cmd->add_option("input", decode_input, "Input .lz file")
               ->required(true);
    decode_cmd->add_option("output", decode_output, "Output text file (optional, '-' for stdout)");
    decode_cmd->add_option("-o,--output-flag", decode_output, "Output file (flag alias)");

    // Команда stats
    auto stats_cmd = app.add_subcommand("stats", "Show compression statistics");
    stats_cmd->alias("s");
    
    std::string stats_file = "";
    stats_cmd->add_option("file", stats_file, ".lz file to analyze")
             ->required(true);

    // Обработка версии до основного парсинга, если это единственный аргумент
    if (argc == 2 && (std::string(argv[1]) == "v" || std::string(argv[1]) == "version")) {
        print_version();
        return 0;
    }

    CLI11_PARSE(app, argc, argv);

    if (show_version) {
        print_version();
        return 0;
    }

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
        
        if (ctx.output_file.empty() && compress_input != "-") {
            ctx.output_file = compress_input + ".lz";
        } else if (ctx.output_file.empty()) {
            std::cerr << "Error: Output file is required when reading from stdin\n";
            return 1;
        }

        ctx.mode = compress_mode;
        ctx.use_stdin = (compress_input == "-");
        
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

    // Если ничего не введено, CLI11 сам выведет help, так как нет обязательных параметров на уровне app
    return 0;
}
