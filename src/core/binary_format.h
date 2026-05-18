#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace llmzip {

// Магическое число формата .lz: "LLMZ" + версия
constexpr uint32_t LZ_MAGIC = 0x4C4C4D5A; // "LLMZ" в little-endian
constexpr uint8_t LZ_VERSION = 1;

// Типы сжатия в заголовке
enum class LZCompressionType : uint8_t {
    LOSSY = 0,
    LOSSLESS = 1,
    HYBRID = 2
};

// Заголовок файла .lz
struct LZHeader {
    uint32_t magic;             // Магическое число
    uint8_t version;            // Версия формата
    LZCompressionType type;     // Тип сжатия
    uint8_t reserved;           // Зарезервировано
    uint64_t original_size;     // Оригинальный размер
    uint64_t compressed_size;   // Размер сжатых данных
    uint32_t checksum;          // CRC32 чек-сумма заголовка
    
    static constexpr size_t SIZE = 32; // Общий размер заголовка
};

// Бинарный форматтер для сериализации/десериализации
class BinaryFormat {
public:
    BinaryFormat();
    ~BinaryFormat();

    // Создание заголовка
    LZHeader create_header(LZCompressionType type, 
                          size_t original_size, 
                          size_t compressed_size);
    
    // Сериализация заголовка в байты
    std::vector<uint8_t> serialize_header(const LZHeader& header);
    
    // Десериализация заголовка из байтов
    bool deserialize_header(const std::vector<uint8_t>& data, LZHeader& header);
    
    // Проверка валидности заголовка
    bool validate_header(const LZHeader& header);
    
    // Вычисление CRC32
    uint32_t calculate_crc32(const uint8_t* data, size_t length);
    
    // Полная упаковка данных (заголовок + payload)
    std::vector<uint8_t> pack_data(LZCompressionType type,
                                   const std::string& original_text,
                                   const std::vector<uint8_t>& compressed_payload);
    
    // Полная распаковка данных
    bool unpack_data(const std::vector<uint8_t>& packed_data,
                    LZHeader& header,
                    std::vector<uint8_t>& payload);

private:
    // Внутренние утилиты
    void write_uint32(std::vector<uint8_t>& buffer, uint32_t value, size_t offset);
    void write_uint64(std::vector<uint8_t>& buffer, uint64_t value, size_t offset);
    uint32_t read_uint32(const std::vector<uint8_t>& buffer, size_t offset);
    uint64_t read_uint64(const std::vector<uint8_t>& buffer, size_t offset);
};

} // namespace llmzip
