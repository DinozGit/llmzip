#include "core/binary_format.h"
#include <cstring>
#include <algorithm>
#include <stdexcept>

namespace llmzip {
namespace {

// CRC32 lookup table
static uint32_t crc32_table[256];
static bool crc32_table_initialized = false;

void init_crc32_table() {
    for (uint32_t i = 0; i < 256; ++i) {
        uint32_t crc = i;
        for (int j = 0; j < 8; ++j) {
            if (crc & 1)
                crc = (crc >> 1) ^ 0xEDB88320;
            else
                crc >>= 1;
        }
        crc32_table[i] = crc;
    }
    crc32_table_initialized = true;
}

} // anonymous namespace

BinaryFormat::BinaryFormat() = default;
BinaryFormat::~BinaryFormat() = default;

LZHeader BinaryFormat::create_header(LZCompressionType type,
                                     size_t original_size,
                                     size_t compressed_size) {
    LZHeader header{};
    header.magic = LZ_MAGIC;
    header.version = LZ_VERSION;
    header.type = type;
    header.reserved = 0;
    header.original_size = static_cast<uint64_t>(original_size);
    header.compressed_size = static_cast<uint64_t>(compressed_size);
    header.checksum = 0; // will be calculated after serialization

    // Serialize to calculate CRC
    auto serialized = serialize_header(header);
    header.checksum = calculate_crc32(serialized.data(), serialized.size());

    return header;
}

std::vector<uint8_t> BinaryFormat::serialize_header(const LZHeader& header) {
    std::vector<uint8_t> buf(LZHeader::SIZE, 0);

    write_uint32(buf, header.magic, 0);
    buf[4] = header.version;
    buf[5] = static_cast<uint8_t>(header.type);
    buf[6] = header.reserved;
    buf[7] = 0; // padding
    write_uint64(buf, header.original_size, 8);
    write_uint64(buf, header.compressed_size, 16);
    write_uint32(buf, header.checksum, 24);
    // bytes 28-31 are padding (zeros)

    return buf;
}

bool BinaryFormat::deserialize_header(const std::vector<uint8_t>& data, LZHeader& header) {
    if (data.size() < LZHeader::SIZE)
        return false;

    header.magic = read_uint32(data, 0);
    header.version = data[4];
    header.type = static_cast<LZCompressionType>(data[5]);
    header.reserved = data[6];
    // byte 7 is padding, skip
    header.original_size = read_uint64(data, 8);
    header.compressed_size = read_uint64(data, 16);
    header.checksum = read_uint32(data, 24);

    return true;
}

bool BinaryFormat::validate_header(const LZHeader& header) {
    // Check magic number
    if (header.magic != LZ_MAGIC)
        return false;

    // Check version
    if (header.version != LZ_VERSION)
        return false;

    // Check compression type is valid
    if (header.type != LZCompressionType::LOSSY &&
        header.type != LZCompressionType::LOSSLESS &&
        header.type != LZCompressionType::HYBRID)
        return false;

    // Verify checksum
    // Create a temporary header with checksum=0 for CRC calculation
    LZHeader temp = header;
    temp.checksum = 0;
    auto serialized = serialize_header(temp);
    uint32_t expected_crc = calculate_crc32(serialized.data(), serialized.size());

    return header.checksum == expected_crc;
}

uint32_t BinaryFormat::calculate_crc32(const uint8_t* data, size_t length) {
    if (!crc32_table_initialized)
        init_crc32_table();

    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < length; ++i) {
        uint8_t index = (crc ^ data[i]) & 0xFF;
        crc = (crc >> 8) ^ crc32_table[index];
    }
    return crc ^ 0xFFFFFFFF;
}

std::vector<uint8_t> BinaryFormat::pack_data(LZCompressionType type,
                                              const std::string& original_text,
                                              const std::vector<uint8_t>& compressed_payload) {
    auto header = create_header(type, original_text.size(), compressed_payload.size());
    auto header_bytes = serialize_header(header);

    // Total size: header + payload + CRC32 of payload
    std::vector<uint8_t> result;
    result.reserve(header_bytes.size() + compressed_payload.size() + 4);

    result.insert(result.end(), header_bytes.begin(), header_bytes.end());
    result.insert(result.end(), compressed_payload.begin(), compressed_payload.end());

    // Append CRC32 of the payload
    uint32_t payload_crc = calculate_crc32(compressed_payload.data(), compressed_payload.size());
    result.push_back(static_cast<uint8_t>(payload_crc & 0xFF));
    result.push_back(static_cast<uint8_t>((payload_crc >> 8) & 0xFF));
    result.push_back(static_cast<uint8_t>((payload_crc >> 16) & 0xFF));
    result.push_back(static_cast<uint8_t>((payload_crc >> 24) & 0xFF));

    return result;
}

bool BinaryFormat::unpack_data(const std::vector<uint8_t>& packed_data,
                                LZHeader& header,
                                std::vector<uint8_t>& payload) {
    // Minimum size: header (32) + CRC32 (4)
    if (packed_data.size() < LZHeader::SIZE + 4)
        return false;

    // Deserialize header
    if (!deserialize_header(packed_data, header))
        return false;

    // Validate header
    if (!validate_header(header))
        return false;

    // Check that payload size matches
    if (LZHeader::SIZE + header.compressed_size + 4 > packed_data.size())
        return false;

    // Extract payload
    payload.resize(header.compressed_size);
    if (header.compressed_size > 0) {
        std::memcpy(payload.data(),
                    packed_data.data() + LZHeader::SIZE,
                    header.compressed_size);
    }

    // Verify payload CRC32
    uint32_t stored_crc = read_uint32(packed_data, LZHeader::SIZE + header.compressed_size);
    uint32_t actual_crc = calculate_crc32(payload.data(), payload.size());

    return stored_crc == actual_crc;
}

void BinaryFormat::write_uint32(std::vector<uint8_t>& buffer, uint32_t value, size_t offset) {
    if (offset + 4 > buffer.size())
        return;
    buffer[offset + 0] = static_cast<uint8_t>(value & 0xFF);
    buffer[offset + 1] = static_cast<uint8_t>((value >> 8) & 0xFF);
    buffer[offset + 2] = static_cast<uint8_t>((value >> 16) & 0xFF);
    buffer[offset + 3] = static_cast<uint8_t>((value >> 24) & 0xFF);
}

void BinaryFormat::write_uint64(std::vector<uint8_t>& buffer, uint64_t value, size_t offset) {
    if (offset + 8 > buffer.size())
        return;
    buffer[offset + 0] = static_cast<uint8_t>(value & 0xFF);
    buffer[offset + 1] = static_cast<uint8_t>((value >> 8) & 0xFF);
    buffer[offset + 2] = static_cast<uint8_t>((value >> 16) & 0xFF);
    buffer[offset + 3] = static_cast<uint8_t>((value >> 24) & 0xFF);
    buffer[offset + 4] = static_cast<uint8_t>((value >> 32) & 0xFF);
    buffer[offset + 5] = static_cast<uint8_t>((value >> 40) & 0xFF);
    buffer[offset + 6] = static_cast<uint8_t>((value >> 48) & 0xFF);
    buffer[offset + 7] = static_cast<uint8_t>((value >> 56) & 0xFF);
}

uint32_t BinaryFormat::read_uint32(const std::vector<uint8_t>& buffer, size_t offset) {
    if (offset + 4 > buffer.size())
        return 0;
    return static_cast<uint32_t>(buffer[offset]) |
           (static_cast<uint32_t>(buffer[offset + 1]) << 8) |
           (static_cast<uint32_t>(buffer[offset + 2]) << 16) |
           (static_cast<uint32_t>(buffer[offset + 3]) << 24);
}

uint64_t BinaryFormat::read_uint64(const std::vector<uint8_t>& buffer, size_t offset) {
    if (offset + 8 > buffer.size())
        return 0;
    return static_cast<uint64_t>(buffer[offset]) |
           (static_cast<uint64_t>(buffer[offset + 1]) << 8) |
           (static_cast<uint64_t>(buffer[offset + 2]) << 16) |
           (static_cast<uint64_t>(buffer[offset + 3]) << 24) |
           (static_cast<uint64_t>(buffer[offset + 4]) << 32) |
           (static_cast<uint64_t>(buffer[offset + 5]) << 40) |
           (static_cast<uint64_t>(buffer[offset + 6]) << 48) |
           (static_cast<uint64_t>(buffer[offset + 7]) << 56);
}

} // namespace llmzip
