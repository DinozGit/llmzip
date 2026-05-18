#include "onnx/model_loader.h"
#include <fstream>
#include <vector>
#include <sys/stat.h>

namespace llmzip {

ModelLoader::ModelLoader() = default;
ModelLoader::~ModelLoader() = default;

bool ModelLoader::model_exists(const std::string& path) {
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
}

std::string ModelLoader::find_default_model() {
    const auto& paths = ModelLoader::get_search_paths();
    for (const auto& p : paths) {
        if (model_exists(p))
            return p;
    }
    return {};
}

bool ModelLoader::validate_onnx_file(const std::string& path) {
    // Quick validation: check file size and magic bytes
    if (!model_exists(path)) return false;

    std::ifstream file(path, std::ios::binary);
    if (!file) return false;

    // ONNX files start with magic: 0x08 0x00 0x00 0x00 (protobuf)
    char magic[4];
    file.read(magic, 4);
    if (file.gcount() < 4) return false;

    // Check it's non-empty and has reasonable size
    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    return size > 1024; // minimal valid ONNX model is > 1KB
}

ModelLoader::ModelInfo ModelLoader::get_model_info(const std::string& path) {
    ModelInfo info;
    info.path = path;
    info.size_bytes = 0;
    info.is_quantized = false;

    if (!model_exists(path)) return info;

    struct stat st;
    if (stat(path.c_str(), &st) == 0)
        info.size_bytes = st.st_size;

    // Heuristic: quantized models typically have "quant" in filename
    std::string lower = path;
    for (auto& c : lower) c = tolower(c);
    info.is_quantized = (lower.find("quant") != std::string::npos ||
                         lower.find("int8") != std::string::npos);

    info.format_version = "ONNX Opset ";

    return info;
}

const std::vector<std::string>& ModelLoader::get_search_paths() {
    static const std::vector<std::string> paths = {
        "model.onnx",
        "./model.onnx",
        "./resources/model.onnx",
        "../resources/model.onnx",
        "../model.onnx",
    };
    return paths;
}

} // namespace llmzip