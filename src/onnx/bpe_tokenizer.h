#pragma once
#include <string>
#include <vector>
#include <unordered_map>

namespace llmzip {

class BPETokenizer {
public:
    BPETokenizer();
    
    std::vector<int64_t> encode(const std::string& text);
    std::string decode(const std::vector<int64_t>& tokens);
    
    int vocab_size() const { return 50265; }
    int pad_token_id() const { return 1; }
    int eos_token_id() const { return 2; }
    int bos_token_id() const { return 0; }
    int unk_token_id() const { return 3; }

private:
    std::vector<std::string> id_to_token_;
    std::unordered_map<std::string, int> token_to_id_;
    std::vector<std::pair<std::string, std::string>> merges_;
    
    void load_vocab();
    std::vector<std::string> bpe(const std::string& word);
    std::string pre_tokenize(const std::string& text);
};

}
