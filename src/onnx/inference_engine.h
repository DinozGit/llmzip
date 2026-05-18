#pragma once

#include <string>
#include <vector>
#include <memory>

// Forward declaration ONNX Runtime
struct OrtSession;
struct OrtEnv;
struct OrtValue;

namespace llmzip {

// Результаты инференса модели
struct InferenceResult {
    std::string output_text;
    float confidence_score;
    bool success;
    std::string error_message;
    size_t inference_time_ms; // Время инференса в мс
};

// Движок инференса на базе ONNX
class InferenceEngine {
public:
    InferenceEngine();
    ~InferenceEngine();

    // Инициализация с загрузкой модели
    bool initialize(const std::string& model_path);
    
    // Выполнение инференса для сжатия текста
    InferenceResult compress_text(const std::string& input);
    
    // Выполнение инференса для восстановления текста
    InferenceResult decompress_text(const std::vector<uint8_t>& compressed_data);
    
    // Проверка инициализации
    bool is_initialized() const { return initialized_; }
    
    // Получение информации о модели
    std::string get_model_info() const;

private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
    bool initialized_ = false;
    
    // Внутренние методы для работы с ONNX
    std::vector<int64_t> tokenize_input(const std::string& input);
    std::string detokenize_output(const std::vector<int64_t>& tokens);
};

} // namespace llmzip
