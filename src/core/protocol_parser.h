#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <regex>

namespace llmzip {

// Правила протокола из ядро сжатия.md
struct ProtocolRule {
    std::string pattern;            // Regex паттерн
    std::string replacement;        // Замена (токен протокола)
    int priority;                   // Приоритет правила
    bool is_semantic;               // Семантическое ли правило
};

// Парсер протокола сжатия
class ProtocolParser {
public:
    ProtocolParser();
    ~ProtocolParser();

    // Загрузка правил из файла ядро сжатия.md
    bool load_rules_from_file(const std::string& filepath);
    
    // Загрузка правил из строки (встроенные правила)
    bool load_default_rules();
    
    // Применение правил к тексту
    std::string apply_rules(const std::string& input, bool reverse = false);
    
    // Детерминированное сжатие по шаблону (без LLM)
    std::string compress_deterministic(const std::string& input);
    
    // Извлечение семантических маркеров
    std::vector<std::string> extract_markers(const std::string& input);
    
    // Проверка валидности протокола
    bool validate_protocol(const std::string& input);
    
    // Получение статистики по правилам
    size_t get_rules_count() const { return rules_.size(); }

private:
    std::vector<ProtocolRule> rules_;
    std::unordered_map<std::string, std::string> token_map_;
    std::unordered_map<std::string, std::string> reverse_token_map_;
    
    // Внутренние методы
    void sort_rules_by_priority();
    std::string normalize(const std::string& input);
    std::vector<std::string> segment(const std::string& input);
    std::string map_dictionary(const std::string& input);
    std::string assemble(const std::vector<std::string>& segments);
    
    std::string normalize_whitespace(const std::string& input);
    std::string compress_common_phrases(const std::string& input);
    std::string decompress_common_phrases(const std::string& input);
};

} // namespace llmzip
