#include <vector>
#include <iostream>
#include <fstream>
#include "core/binary_format.h"
#include "core/compressor.h"

int main() {
    std::ifstream file("/tmp/test_lossy.lz", std::ios::binary);
    std::vector<uint8_t> data(std::istreambuf_iterator<char>(file), {});
    std::cout << "Read " << data.size() << " bytes\n";

    llmzip::BinaryFormat bf;
    llmzip::LZHeader h;
    std::vector<uint8_t> p;
    bool ok1 = bf.unpack_data(data, h, p);
    std::cout << "BinaryFormat: " << ok1 << " payload=" << p.size() << "\n";

    llmzip::Compressor comp;
    comp.initialize();
    auto res = comp.decompress(data);
    std::cout << "Compressor: " << res.success << " msg=\"" << res.error_message << "\" text=\"" << res.text << "\"\n";
    return 0;
}