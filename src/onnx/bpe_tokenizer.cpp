#include "bpe_tokenizer.h"
#include "bart_bpe_data.h"
#include <sstream>
#include <algorithm>
#include <unordered_map>

namespace llmzip {

BPETokenizer::BPETokenizer() {
    load_vocab();
}

void BPETokenizer::load_vocab() {
    using namespace bart_bpe;
    
    id_to_token_.resize(VOCAB_SIZE);
    for (int i = 0; i < VOCAB_SIZE; ++i) {
        std::string tok = VOCAB[i].token;
        int id = VOCAB[i].id;
        if (id >= 0 && id < VOCAB_SIZE) {
            id_to_token_[id] = tok;
        }
        token_to_id_[tok] = id;
    }
    
    merges_.reserve(NUM_MERGES);
    for (int i = 0; i < NUM_MERGES; ++i) {
        merges_.emplace_back(MERGES[i].left, MERGES[i].right);
    }
}

std::string BPETokenizer::pre_tokenize(const std::string& text) {
    // GPT-2 style pre-tokenization: simplified split
    std::string result;
    for (char c : text) {
        if (isspace((unsigned char)c)) {
            if (!result.empty() && result.back() != ' ') result += ' ';
        } else {
            result += c;
        }
    }
    return result;
}

std::vector<std::string> BPETokenizer::bpe(const std::string& token) {
    if (token.empty()) return {};
    
    std::vector<std::string> word;
    for (size_t i = 0; i < token.size(); ++i) {
        word.push_back(std::string(1, token[i]));
    }
    
    while (word.size() > 1) {
        int best_rank = 999999;
        int best_idx = -1;
        
        for (size_t i = 0; i < word.size() - 1; ++i) {
            std::string pair = word[i] + " " + word[i+1];
            // Linear search merges for simplicity (can be optimized)
            for (size_t r = 0; r < merges_.size(); ++r) {
                if (merges_[r].first + " " + merges_[r].second == pair) {
                    if ((int)r < best_rank) {
                        best_rank = (int)r;
                        best_idx = (int)i;
                    }
                    break;
                }
            }
        }
        
        if (best_idx == -1) break;
        
        std::string merged = word[best_idx] + word[best_idx+1];
        std::vector<std::string> new_word;
        for (int i = 0; i < (int)word.size(); ++i) {
            if (i == best_idx) {
                new_word.push_back(merged);
                i++;
            } else {
                new_word.push_back(word[i]);
            }
        }
        word = new_word;
    }
    return word;
}

std::vector<int64_t> BPETokenizer::encode(const std::string& text) {
    std::vector<int64_t> tokens;
    tokens.push_back(bos_token_id());
    
    std::stringstream ss(text);
    std::string word;
    while (ss >> word) {
        // BART uses Ġ for spaces at start of words
        std::string bart_word = "\xC4\xA0" + word; 
        auto bpe_res = bpe(bart_word);
        for (const auto& t : bpe_res) {
            auto it = token_to_id_.find(t);
            if (it != token_to_id_.end()) tokens.push_back(it->second);
            else tokens.push_back(unk_token_id());
        }
    }
    
    tokens.push_back(eos_token_id());
    return tokens;
}

std::string BPETokenizer::decode(const std::vector<int64_t>& tokens) {
    std::string result;
    for (int64_t id : tokens) {
        if (id < 0 || id >= (int64_t)id_to_token_.size()) continue;
        if (id == bos_token_id() || id == eos_token_id() || id == pad_token_id()) continue;
        
        std::string tok = id_to_token_[id];
        // Replace Ġ with space
        if (tok.size() >= 2 && (unsigned char)tok[0] == 0xC4 && (unsigned char)tok[1] == 0xA0) {
            result += " ";
            result += tok.substr(2);
        } else {
            result += tok;
        }
    }
    return result;
}

}
