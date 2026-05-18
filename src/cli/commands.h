#pragma once

#include <string>
#include <vector>
#include <functional>

namespace llmzip {
namespace cli {

// Контекст команды
struct CommandContext {
    std::string command;
    std::string input_file;
    std::string output_file;
    std::string mode;  // lossy, lossless, hybrid
    std::string model_path;
    bool verbose;
    bool use_stdin;
    bool use_stdout;
};

// Обработчики команд
class Commands {
public:
    Commands();
    ~Commands();

    // Выполнение команды сжатия
    int compress(const CommandContext& ctx);
    
    // Выполнение команды распаковки
    int decompress(const CommandContext& ctx);
    
    // Показ статистики файла
    int stats(const CommandContext& ctx);
    
    // Pipe режим (stdin -> stdout)
    int pipe_mode(const CommandContext& ctx);

private:
    // Внутренние утилиты
    std::string read_file(const std::string& path);
    bool write_file(const std::string& path, const std::vector<uint8_t>& data);
    bool write_file(const std::string& path, const std::string& text);
    void print_progress(const std::string& message, bool verbose);
};

} // namespace cli
} // namespace llmzip
