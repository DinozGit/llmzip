#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

namespace llmzip {

// Режимы сжатия
enum class CompressionMode {
    LOSSY,      // Семантическое сжатие с потерями (максимальная компрессия)
    LOSSLESS,   // Токен-based сжатие без потерь
    HYBRID      // Комбинированный режим
};

// Структура результата сжатия
struct CompressionResult {
    std::vector<uint8_t> data;      // Сжатые данные
    size_t original_size;           // Оригинальный размер в байтах
    size_t compressed_size;         // Размер после сжатия
    double compression_ratio;       // Коэффициент сжатия
    std::string mode;               // Использованный режим
    bool success;                   // Флаг успеха
    std::string error_message;      // Сообщение об ошибке
};

// Структура результата распаковки
struct DecompressionResult {
    std::string text;               // Восстановленный текст
    size_t compressed_size;         // Размер сжатых данных
    size_t decompressed_size;       // Размер восстановленного текста
    bool success;                   // Флаг успеха
    std::string error_message;      // Сообщение об ошибке
};

// Основной класс компрессора
class Compressor {
public:
    Compressor();
    ~Compressor();

    // Инициализация с загрузкой модели
    bool initialize(const std::string& model_path = "");
    
    // Сжатие текста
    CompressionResult compress(const std::string& input, CompressionMode mode = CompressionMode::LOSSY);
    
    // Распаковка данных
    DecompressionResult decompress(const std::vector<uint8_t>& input);
    
    // Распаковка из файла
    DecompressionResult decompress_file(const std::string& filepath);
    
    // Проверка инициализации
    bool is_initialized() const { return initialized_; }
    
    // Получение версии протокола
    static constexpr int PROTOCOL_VERSION = 1;

private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
    bool initialized_ = false;
    
    // Внутренние методы
    CompressionResult compress_lossy(const std::string& input);
    CompressionResult compress_lossless(const std::string& input);
    CompressionResult compress_hybrid(const std::string& input);
    std::string decompress_lossless_impl(const std::string& compressed);
    std::string decompress_lossy_impl(const std::string& compressed);
    std::string decompress_hybrid_impl(const std::string& compressed);
    
    // Парсинг заголовка .lz файла
    bool parse_header(const std::vector<uint8_t>& data, std::string& mode, std::vector<uint8_t>& payload);
};

} // namespace llmzip
