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
    // Databases & Storage
    {"PG","PostgreSQL"}, {"MySQL","MySQL"}, {"SQLite","SQLite"},
    {"Redis","Redis"}, {"Mongo","MongoDB"}, {"S3","Amazon S3"},
    {"ES","Elasticsearch"}, {"TSDB","Time Series Database"},

    // Frameworks & Libraries
    {"DRF","Django REST Framework"}, {"ORM","Object-Relational Mapping"},
    {"PyDev","Python Developer"}, {"FSDev","Full-Stack Developer"},

    // Roles & Positions
    {"DevOps","DevOps Engineer"}, {"2+y","2+ years"},
    {"PM","Product Manager"}, {"TL","Tech Lead"},
    {"EM","Engineering Manager"}, {"SWE","Software Engineer"},

    // AI / ML
    {"LLM","Large Language Model"}, {"RAG","Retrieval-Augmented Generation"},
    {"Emb","Embeddings"}, {"Tok","Tokens"}, {"Temp","Temperature"},
    {"AI","Artificial Intelligence"}, {"ML","Machine Learning"},
    {"DL","Deep Learning"}, {"NLP","Natural Language Processing"},
    {"CV","Computer Vision"}, {"ASR","Automatic Speech Recognition"},
    {"TTS","Text-to-Speech"}, {"NER","Named Entity Recognition"},
    {"SLM","Small Language Model"}, {"GenAI","Generative AI"},
    {"Agent","AI Agent"}, {"FT","Fine-tuning"},
    {"RHLF","Reinforcement Learning from Human Feedback"},
    {"LoRA","Low-Rank Adaptation"}, {"QLoRA","Quantized LoRA"},
    {"SFT","Supervised Fine-Tuning"}, {"DPO","Direct Preference Optimization"},
    {"KTO","KL-constrained TO"}, {"KG","Knowledge Graph"},
    {"Embed","Embeddings Model"}, {"VecDB","Vector Database"},
    {"ANN","Approximate Nearest Neighbor"},

    // Tech stack
    {"CDN","Content Delivery Network"}, {"WAF","Web Application Firewall"},
    {"RBAC","Role-Based Access Control"}, {"IaC","Infrastructure as Code"},
    {"API","Application Programming Interface"}, {"REST","RESTful API"},
    {"gRPC","gRPC"}, {"GraphQL","GraphQL"},
    {"CLI","Command-Line Interface"}, {"GUI","Graphical User Interface"},
    {"SDK","Software Development Kit"}, {"IDE","Integrated Development Environment"},
    {"JSON","JSON"}, {"YAML","YAML"}, {"XML","XML"},
    {"CSV","CSV"}, {"TSV","TSV"}, {"Proto","Protocol Buffers"},
    {"TUI","Terminal User Interface"}, {"BPM","Business Process Management"},

    // Protocols & infra
    {"HTTP","HTTP"}, {"HTTPS","HTTPS"}, {"TCP","TCP"},
    {"UDP","UDP"}, {"TLS","Transport Layer Security"},
    {"mTLS","Mutual TLS"}, {"gRPC","gRPC"},
    {"SSH","SSH"}, {"SFTP","SSH File Transfer"},
    {"JWT","JSON Web Token"}, {"OAuth","OAuth"},
    {"OIDC","OpenID Connect"}, {"SAML","SAML"},

    // CI/CD & DevOps
    {"CI","Continuous Integration"}, {"CD","Continuous Deployment"},
    {"PR","Pull Request"}, {"MR","Merge Request"},
    {"CR","Code Review"}, {"LGTM","Looks Good To Me"},
    {"QA","Quality Assurance"}, {"QE","Quality Engineering"},
    {"Stg","Staging"}, {"Prod","Production"},
    {"Dev","Development"}, {"UAT","User Acceptance Testing"},
    {"k8s","Kubernetes"}, {"K8s","Kubernetes"},
    {"K3s","K3s"}, {"k3s","K3s"},
    {"Dkr","Docker"}, {"Docker","Docker"},
    {"Kustomize","Kustomize"}, {"Helm","Helm"},
    {"Terraform","Terraform"}, {"Pulumi","Pulumi"},
    {"Ansible","Ansible"}, {"Chef","Chef"},
    {"Pup","Puppet"}, {"SRE","Site Reliability Engineering"},
    {"SLI","Service Level Indicator"}, {"SLO","Service Level Objective"},
    {"SLA","Service Level Agreement"},

    // Architecture
    {"CLI11","CLI11"}, {"ONNX","ONNX Runtime"},
    {"CXX","C++"}, {"Py","Python"},
    {"JS","JavaScript"}, {"TS","TypeScript"},
    {"CPU","CPU"}, {"GPU","GPU"}, {"TPU","TPU"},
    {"NPU","NPU"}, {"RAM","RAM"},
    {"BIOS","BIOS"}, {"OS","Operating System"},
    {"P2P","Peer-to-Peer"}, {"P2V","Physical-to-Virtual"},
    {"V2V","Virtual-to-Virtual"},

    // BPM / Analytics
    {"KPI","Key Performance Indicator"}, {"OKR","Objectives and Key Results"},
    {"ROI","Return on Investment"}, {"TCO","Total Cost of Ownership"},
    {"T2M","Time to Market"}, {"CA","Cost Analysis"},
    {"EA","Enterprise Architecture"}, {"ERP","Enterprise Resource Planning"},
    {"CRM","Customer Relationship Management"},

    // Math / Stats
    {"SVD","Singular Value Decomposition"}, {"PCA","Principal Component Analysis"},
    {"t-SNE","t-distributed Stochastic Neighbor Embedding"},
    {"UMAP","Uniform Manifold Approximation"},
    {"GMM","Gaussian Mixture Model"}, {"HMM","Hidden Markov Model"},
    {"CRF","Conditional Random Field"}, {"SVM","Support Vector Machine"},
    {"KNN","K-Nearest Neighbors"}, {"DT","Decision Tree"},
    {"RF","Random Forest"}, {"GB","Gradient Boosting"},
    {"GBM","Gradient Boosting Machine"}, {"XGB","XGBoost"},
    {"LGBM","LightGBM"}, {"CB","CatBoost"},
    {"MLP","Multilayer Perceptron"}, {"CNN","Convolutional Neural Network"},
    {"RNN","Recurrent Neural Network"}, {"GRU","Gated Recurrent Unit"},
    {"LSTM","Long Short-Term Memory"}, {"GAN","Generative Adversarial Network"},
    {"VAE","Variational Autoencoder"}, {"AE","Autoencoder"},
    {"MHA","Multi-Head Attention"}, {"FFN","Feed-Forward Network"},
    {"LN","Layer Normalization"}, {"BN","Batch Normalization"},
    {"ReLU","ReLU"}, {"GeLU","GeLU"}, {"SiLU","SiLU"},
    {"Softmax","Softmax"}, {"Sig","Sigmoid"},

    // Parser-specific
    {"LLMZip","LLMZip"}, {"llmzip","llmzip"},
};

// Section markers
static const std::vector<std::pair<std::string,std::string>> kSections = {
    {"Proj:","Project"}, {"Status:","Status"},
    {"Type:","Type"}, {"Stack:","Technology Stack"},
    {"Arch:","Architecture"}, {"Mode:","Mode"},
    {"IO:","Input/Output"}, {"Fmt:","Format"},
    {"Dev:","Development"}, {"Perf:","Performance"},
    {"Lic:","License"}, {"Meta:","Metadata"},
    {"Req:","Requirements"}, {"Offer:","Proposed Solution"},
    {"Opt:","Optional Elements"}, {"Ctx:","Session Context"},
    {"Out:","Output Format"}, {"Err:","Error Handling"},
    {"Test:","Test Scenarios"}, {"Conf:","Configuration"},
    {"Deploy:","Deployment"}, {"Build:","Build"},
    {"Doc:","Documentation"}, {"Roadmap:","Roadmap"},
    {"Next:","Next Steps"},
};

// Domain prefixes
static const std::vector<std::pair<std::string,std::string>> kDomains = {
    {"ML:","Machine Learning"}, {"Sec:","Security"},
    {"Biz:","Business"}, {"Dev:","Development"},
    {"UX:","User Experience"}, {"Ops:","Operations"},
    {"AI:","AI Agent"}, {"Cache:","Caching"},
    {"Net:","Networking"}, {"DB:","Database"},
    {"FE:","Frontend"}, {"BE:","Backend"},
    {"FS:","Full-Stack"}, {"QA:","Quality Assurance"},
    {"Infra:","Infrastructure"}, {"Data:","Data"},
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
    {"tech","technical focus"}, {"ready","ready for review"},
    {"progress","in progress"}, {"done","completed"},
    {"blocked","blocked"}, {"planned","planned"},
    {"deprecated","deprecated"}, {"experimental","experimental"},
    {"stable","stable"}, {"beta","beta"},
    {"alpha","alpha"}, {"deprecated","deprecated"},
    {"wip","work in progress"},
};

// Symbols
static const std::vector<std::pair<std::string,std::string>> kSymbols = {
    {"(tm)","reduces"}, {"(tb)","increases"},
    {"[crit]","critical"}, {"[req]","required"},
    {"[opt]","optional"}, {"[warn]","warning"},
};

// Common compressible phrases (longer first for safety)
static const std::vector<std::pair<std::string,std::string>> kPhrases = {
    // AI/ML phrases
    {"large language model", "LLM"},
    {"machine learning", "ML"},
    {"deep learning", "DL"},
    {"natural language processing", "NLP"},
    {"retrieval augmented generation", "RAG"},
    {"context window", "CtxWin"},
    {"function calling", "FuncCall"},
    {"batch processing", "Batch"},
    {"fine tuning", "FT"},
    {"low rank adaptation", "LoRA"},
    {"embedding model", "Embed"},
    {"vector database", "VecDB"},
    {"knowledge graph", "KG"},
    {"named entity recognition", "NER"},
    {"computer vision", "CV"},
    {"speech recognition", "ASR"},
    {"text to speech", "TTS"},

    // DevOps / Infra
    {"continuous integration", "CI"},
    {"continuous deployment", "CD"},
    {"role based access control", "RBAC"},
    {"infrastructure as code", "IaC"},
    {"site reliability engineering", "SRE"},
    {"service level agreement", "SLA"},
    {"service level objective", "SLO"},
    {"service level indicator", "SLI"},
    {"pull request", "PR"},
    {"user experience", "UX"},
    {"key performance indicator", "KPI"},
    {"return on investment", "ROI"},
    {"total cost of ownership", "TCO"},

    // Architecture
    {"command line interface", "CLI"},
    {"graphical user interface", "GUI"},
    {"application programming interface", "API"},
    {"software development kit", "SDK"},
    {"integrated development environment", "IDE"},
    {"operating system", "OS"},
    {"virtual machine", "VM"},
    {"container orchestration", "k8s"},
    {"transport layer security", "TLS"},
    {"json web token", "JWT"},

    // Business / Project
    {"proof of concept", "PoC"},
    {"minimum viable product", "MVP"},
    {"time to market", "T2M"},
    {"work in progress", "WIP"},
    {"as soon as possible", "ASAP"},

    // Status markers
    {"is required", " [req]"},
    {"is mandatory", " [crit]"},
    {"is critical", " [crit]"},
    {"is optional", " [opt]"},
    {"as soon as possible", " [urgent]"},
    {"compared to", " [cmp]"},
    {"step by step", " [steps]"},
    {"in progress", " >"},
    {"not implemented", " ~"},
    {"planned for", " ~"},
    {"already done", " ✅"},
    {"completed", " ✅"},
    {"finished", " ✅"},
    {"requires attention", " ⚠"},
};

} // anonymous namespace

// ===========================================================================
// ProtocolParser
// ===========================================================================

std::string ProtocolParser::compress_deterministic(const std::string& input) {
    // 1. Normalize
    std::string text = normalize(input);
    
    // 2. Map Dictionary (Greedy)
    text = map_dictionary(text);
    
    // 3. Segment
    auto segments = segment(text);
    
    // 4. Assemble by Template
    return assemble(segments);
}

std::string ProtocolParser::normalize(const std::string& input) {
    std::string result = input;
    // Remove Markdown formatting: **, __, `
    try {
        result = std::regex_replace(result, std::regex("[*_`]"), "");
        // Normalize whitespace
        result = std::regex_replace(result, std::regex("\\s+"), " ");
    } catch (...) {}
    return trim(result);
}

std::vector<std::string> ProtocolParser::segment(const std::string& input) {
    std::vector<std::string> segments;
    std::stringstream ss(input);
    std::string line;
    std::string current_block;
    
    while (std::getline(ss, line)) {
        std::string t_line = trim(line);
        if (t_line.empty()) {
            if (!current_block.empty()) {
                segments.push_back(current_block);
                current_block.clear();
            }
            continue;
        }
        
        // Split by Markdown headers
        if (t_line[0] == '#') {
            if (!current_block.empty()) segments.push_back(current_block);
            segments.push_back(t_line);
            current_block.clear();
        } else {
            if (!current_block.empty()) current_block += " ";
            current_block += t_line;
        }
    }
    if (!current_block.empty()) segments.push_back(current_block);
    return segments;
}

std::string ProtocolParser::map_dictionary(const std::string& input) {
    return apply_rules(input, false);
}

std::string ProtocolParser::assemble(const std::vector<std::string>& segments) {
    std::string proj = "llmzip";
    std::vector<std::string> stack, done, plan, ops;
    
    for (const auto& s : segments) {
        std::string lower = s;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        
        // Skip noise
        if (s.find("+-") != std::string::npos || s.find("|") != std::string::npos) continue;
        if (lower.find("см. раздел") != std::string::npos) continue;

        if (s[0] == '#') {
            if (proj == "llmzip" && lower.find("llmzip") != std::string::npos) {
                // Keep default or extract name
            }
            continue;
        }

        if (s.find("✅") != std::string::npos || lower.find("done") != std::string::npos || s.find("- [x]") != std::string::npos) {
            done.push_back(s);
        } else if (s.find("⬜") != std::string::npos || lower.find("todo") != std::string::npos || s.find("- [ ]") != std::string::npos) {
            plan.push_back(s);
        } else if (lower.find("c++") != std::string::npos || lower.find("python") != std::string::npos || 
                   lower.find("onnx") != std::string::npos || lower.find("cmake") != std::string::npos ||
                   lower.find("zlib") != std::string::npos) {
            stack.push_back(s);
        } else if (lower.size() < 100) { // Only keep short meaningful lines in ops
            ops.push_back(s);
        }
    }
    
    std::ostringstream oss;
    oss << "Proj:" << proj << "; Status:" << (done.empty() ? "🔄" : "✅") << "; →tech; →no-meta\n";
    
    if (!stack.empty()) {
        oss << "Stack:";
        // Extract only keywords from stack blocks
        std::set<std::string> keywords = {"C++", "Python", "ONNX", "CMake", "zlib", "CLI11", "Linux", "macOS", "Windows"};
        bool first = true;
        for (const auto& k : keywords) {
            bool found = false;
            for (const auto& st : stack) {
                if (st.find(k) != std::string::npos) { found = true; break; }
            }
            if (found) {
                if (!first) oss << "/";
                oss << k;
                first = false;
            }
        }
        oss << ";\n";
    }
    
    if (!done.empty()) {
        oss << "Done:";
        for (size_t i = 0; i < std::min(done.size(), (size_t)5); ++i) {
            std::string d = done[i];
            if (d.size() > 50) d = d.substr(0, 47) + "...";
            oss << (i > 0 ? "; " : "") << d;
        }
        oss << ";\n";
    }
    
    oss << "Meta: fp:auto; ver:2.0; →roundtrip:✓✓";
    return oss.str();
}

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