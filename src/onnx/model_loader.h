#pragma once

#include <string>
#include <vector>

namespace llmzip {

// Загрузчик моделей ONNX
class ModelLoader {
public:
    ModelLoader();
    ~ModelLoader();

    // Проверка существования файла модели
    static bool model_exists(const std::string& path);
    
    // Поиск модели в стандартных путях
    static std::string find_default_model();
    
    // Валидация ONNX файла
    static bool validate_onnx_file(const std::string& path);
    
    // Получение информации о модели (размер, версия и т.д.)
    struct ModelInfo {
        std::string path;
        size_t size_bytes;
        std::string format_version;
        bool is_quantized;
    };
    
    static ModelInfo get_model_info(const std::string& path);

private:
    // Стандартные пути для поиска моделей
    static const std::vector<std::string>& get_search_paths();
};

} // namespace llmzip
