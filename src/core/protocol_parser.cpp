#include "core/protocol_parser.h"
#include <sstream>
#include <algorithm>
#include <cctype>
#include <regex>

namespace llmzip {
namespace {

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
std::string trim(const std::string& s) {
    auto start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return {};
    auto end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

std::string replace_all(const std::string& str, const std::string& from, const std::string& to) {
    std::string result = str;
    size_t pos = 0;
    while ((pos = result.find(from, pos)) != std::string::npos) {
        result.replace(pos, from.length(), to);
        pos += to.length();
    }
    return result;
}

// ---------------------------------------------------------------------------
// Data tables
// ---------------------------------------------------------------------------
struct Abbreviation { std::string abbr; std::string full; };
static const std::vector<Abbreviation> kAbbr = {
    {"PG","PostgreSQL"}, {"DRF","Django REST Framework"},
    {"PyDev","Python Developer"}, {"FSDev","Full-Stack Developer"},
    {"DevOps","DevOps Engineer"}, {"2+y","2+ years"},
    {"LLM","Large Language Model"}, {"RAG","Retrieval-Augmented Generation"},
    {"Emb","Embeddings"}, {"Tok","Tokens"}, {"Temp","Temperature"},
    {"CDN","Content Delivery Network"}, {"WAF","Web Application Firewall"},
    {"RBAC","Role-Based Access Control"}, {"IaC","Infrastructure as Code"},
};

// Section markers
static const std::vector<std::pair<std::string,std::string>> kSections = {
    {"Req:","Requirements"}, {"Stack:","Technology Stack"},
    {"Offer:","Proposed Solution"}, {"Opt:","Optional Elements"},
    {"Arch:","Architecture"}, {"Ctx:","Session Context"},
    {"Meta:","Metadata"}, {"Out:","Output Format"},
    {"Err:","Error Handling"}, {"Test:","Test Scenarios"},
};

// Domain prefixes
static const std::vector<std::pair<std::string,std::string>> kDomains = {
    {"ML:","Machine Learning"}, {"Sec:","Security"},
    {"Biz:","Business"}, {"Dev:","Development"},
    {"UX:","User Experience"}, {"Ops:","Operations"},
    {"AI:","AI Agent"}, {"Cache:","Caching"},
};

// Output modifiers (without the → leader, matched via pattern)
static const std::vector<std::pair<std::string,std::string>> kMods = {
    {"no-meta","remove AI self-references"},
    {"no-safe","remove hedging"},
    {"tab","table output"}, {"code","code only"},
    {"steps","numbered steps"}, {"arrow","arrow list"},
    {"ans","one-line answer"}, {"why","reasons only"},
    {"rev","conclusion first"}, {"perf","performance"},
    {"arch","architecture focus"}, {"cmp","comparison"},
    {"stat","statistics"}, {"trend","trend analysis"},
    {"risk","risk assessment"}, {"cost","cost calculation"},
    {"now","urgent"}, {"later","can defer"},
    {"review","review"}, {"compress","apply compression"},
    {"decompress","decode"}, {"roundtrip","verify reversibility"},
};

// Symbols
static const std::vector<std::pair<std::string,std::string>> kSymbols = {
    {"(tm)","reduces"}, {"(tb)","increases"},
    {"[crit]","critical"}, {"[req]","required"},
    {"[opt]","optional"}, {"[warn]","warning"},
};

// Common compressible phrases (longer first for safety)
static const std::vector<std::pair<std::string,std::string>> kPhrases = {
    {"large language model", "LLM"},
    {"machine learning model", "ML:model"},
    {"context window", "CtxWin"},
    {"function calling", "FuncCall"},
    {"batch processing", "Batch"},
    {"role based access control", "RBAC"},
    {"infrastructure as code", "IaC"},
    {"pull request", "PR"},
    {"user experience", "UX"},
    {"is required", " [req]"},
    {"is mandatory", " [crit]"},
    {"is critical", " [crit]"},
    {"is optional", " [opt]"},
    {"as soon as possible", " [urgent]"},
    {"compared to", " [cmp]"},
    {"step by step", " [steps]"},
};

} // anonymous namespace

// ===========================================================================
// ProtocolParser
// ===========================================================================

ProtocolParser::ProtocolParser() { load_default_rules(); }
ProtocolParser::~ProtocolParser() = default;

bool ProtocolParser::load_default_rules() {
    rules_.clear();
    token_map_.clear();
    reverse_token_map_.clear();
    int prio = 1000;

    // Section markers
    for (const auto& s : kSections) {
        ProtocolRule r; r.pattern = s.first; r.replacement = s.first;
        r.priority = prio--; r.is_semantic = true;
        rules_.push_back(r);
        token_map_[s.first] = s.second;
        reverse_token_map_[s.second] = s.first;
    }

    // Domain prefixes
    for (const auto& d : kDomains) {
        ProtocolRule r; r.pattern = d.first; r.replacement = d.first;
        r.priority = prio--; r.is_semantic = true;
        rules_.push_back(r);
        token_map_[d.first] = d.second;
        reverse_token_map_[d.second] = d.first;
    }

    // Abbreviations
    for (const auto& a : kAbbr) {
        ProtocolRule r; r.pattern = a.abbr; r.replacement = a.abbr;
        r.priority = prio--; r.is_semantic = false;
        rules_.push_back(r);
        token_map_[a.abbr] = a.full;
        reverse_token_map_[a.full] = a.abbr;
    }

    // Symbols
    for (const auto& sy : kSymbols) {
        ProtocolRule r; r.pattern = sy.first; r.replacement = sy.first;
        r.priority = prio--; r.is_semantic = true;
        rules_.push_back(r);
    }

    // Modifiers (→xxx)
    for (const auto& m : kMods) {
        std::string tok = std::string("→") + m.first;
        ProtocolRule r; r.pattern = tok; r.replacement = tok;
        r.priority = prio--; r.is_semantic = true;
        rules_.push_back(r);
        token_map_[tok] = m.second;
        reverse_token_map_[m.second] = tok;
    }

    sort_rules_by_priority();
    return true;
}

bool ProtocolParser::load_rules_from_file(const std::string&) {
    return load_default_rules();
}

std::string ProtocolParser::apply_rules(const std::string& input, bool reverse) {
    if (input.empty()) return {};
    std::string result = input;

    if (!reverse) {
        // COMPRESS: text → protocol
        result = normalize_whitespace(result);
        result = compress_common_phrases(result);

        // Replace full names with abbreviations
        std::vector<std::pair<std::string,std::string>> rev_sorted;
        for (const auto& [full, abbr] : reverse_token_map_)
            rev_sorted.emplace_back(full, abbr);
        std::sort(rev_sorted.begin(), rev_sorted.end(),
                  [](const auto& a, const auto& b) { return a.first.size() > b.first.size(); });
        for (const auto& [full, abbr] : rev_sorted) {
            try {
                std::regex word_re("\\b" + full + "\\b", std::regex::icase);
                result = std::regex_replace(result, word_re, abbr);
            } catch (...) {}
        }

        // Section patterns: "Requirements:" → "Req:"
        for (const auto& s : kSections) {
            try {
                std::regex pat(        "\\b" + s.second + ":\\s*", std::regex::icase);
                result = std::regex_replace(result, pat, s.first + " ");
            } catch (...) {}
        }

        // "increases X by Y%" → ↑X:Y%
        try {
            result = std::regex_replace(result,
                std::regex("\\bincreases\\s+(\\S+)\\s+by\\s+(\\d+%)\\b", std::regex::icase),
                "(tb)$1:$2");
            result = std::regex_replace(result,
                std::regex("\\bdecreases\\s+(\\S+)\\s+by\\s+(\\d+%)\\b", std::regex::icase),
                "(tm)$1:$2");
            result = std::regex_replace(result,
                std::regex("\\breduces\\s+(\\S+)\\s+by\\s+(\\d+%)\\b", std::regex::icase),
                "(tm)$1:$2");
        } catch (...) {}

    } else {
        // DECOMPRESS: protocol → readable text
        // Expand section markers
        for (const auto& s : kSections)
            result = replace_all(result, s.first, s.first + " (" + s.second + ") ");

        // Expand domains
        for (const auto& d : kDomains)
            result = replace_all(result, d.first, d.first + " (" + d.second + ") ");

        // Expand abbreviations
        for (const auto& a : kAbbr) {
            try {
                std::regex abbr_re("\\b" + a.abbr + "\\b");
                result = std::regex_replace(result, abbr_re, a.full + " (" + a.abbr + ")");
            } catch (...) {}
        }

        // Expand symbols
        for (const auto& sy : kSymbols)
            result = replace_all(result, sy.first, " [" + sy.second + "]");

        // Expand modifiers
        for (const auto& m : kMods) {
            std::string tok = std::string("→") + m.first;
            result = replace_all(result, tok, "[" + m.second + "]");
        }

        // Handle →prob-N, →byte-N, →deadline:
        try {
            result = std::regex_replace(result, std::regex("→prob-(\\d+)"),    "[probability: $1%]");
            result = std::regex_replace(result, std::regex("→byte-(\\d+)"),    "[token limit: $1]");
            result = std::regex_replace(result, std::regex("→deadline:(\\S+)"), "[deadline: $1]");
            result = std::regex_replace(result, std::regex("→api:(\\S+)"),     "[API: $1]");
            result = std::regex_replace(result, std::regex("→db:(\\S+)"),      "[database: $1]");
            result = std::regex_replace(result, std::regex("→tool:(\\S+)"),    "[tool: $1]");
        } catch (...) {}

        result = normalize_whitespace(result);
    }

    return trim(result);
}

std::vector<std::string> ProtocolParser::extract_markers(const std::string& input) {
    std::vector<std::string> markers;
    if (input.empty()) return markers;

    auto check = [&](const std::string& tok) {
        if (input.find(tok) != std::string::npos)
            markers.push_back(tok);
    };

    for (const auto& s : kSections) check(s.first);
    for (const auto& d : kDomains) check(d.first);
    for (const auto& m : kMods) check(std::string("→") + m.first);

    std::sort(markers.begin(), markers.end());
    markers.erase(std::unique(markers.begin(), markers.end()), markers.end());
    return markers;
}

bool ProtocolParser::validate_protocol(const std::string& input) {
    if (input.empty()) return false;
    for (const auto& s : kSections)
        if (input.find(s.first) != std::string::npos) return true;
    return false;
}

void ProtocolParser::sort_rules_by_priority() {
    std::sort(rules_.begin(), rules_.end(),
              [](const ProtocolRule& a, const ProtocolRule& b) {
                  return a.priority > b.priority;
              });
}

std::string ProtocolParser::normalize_whitespace(const std::string& input) {
    std::string r = input;
    try {
        r = std::regex_replace(r, std::regex("[ \\t]+"), " ");
        r = std::regex_replace(r, std::regex("\\n{3,}"), "\n\n");
        r = std::regex_replace(r, std::regex(";(?! )"), "; ");
    } catch (...) {}
    return trim(r);
}

std::string ProtocolParser::compress_common_phrases(const std::string& input) {
    std::string result = input;
    for (const auto& [phrase, replacement] : kPhrases) {
        try {
            std::regex re("\\b" + phrase + "\\b", std::regex::icase);
            result = std::regex_replace(result, re, replacement);
        } catch (...) {}
    }
    return result;
}

std::string ProtocolParser::decompress_common_phrases(const std::string& input) {
    std::string result = input;
    for (const auto& [phrase, replacement] : kPhrases) {
        result = replace_all(result, replacement, phrase);
    }
    return result;
}

} // namespace llmzip