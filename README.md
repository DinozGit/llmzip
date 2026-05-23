# llmzip — Deterministic Semantic Compression Tool

`llmzip` is a high-performance C++ utility designed to compress technical text into a structured, AI-readable protocol. It achieves up to 90% compression by extracting semantic entities and mapping them to a standardized DSL (Domain Specific Language).

## Key Features

- **Deterministic Semantic Compression (Level 1)**: Uses a rule-based engine to transform raw text into a `Key:Value;` protocol. 100% reproducible, zero latency.
- **LLM-Powered Polish (Level 2)**: Optional integration with ONNX models (BART/T5) for advanced summarization and noise removal.
- **Binary Container (.lz)**: Custom format with Magic headers, CRC32 integrity checks, and metadata tracking.
- **Hybrid Mode**: Combines semantic extraction with standard `zlib` compression for maximum efficiency.
- **Technical Focus**: Pre-configured for DevOps, AI/ML, and Software Architecture domains.

## How it Works

`llmzip` transforms this:
> "We are using a microservices architecture with a PostgreSQL database and Python FastAPI. The project is currently in progress and we have finished the authentication module."

Into this:
> `Proj:llmzip; Status:🔄; Stack:FastAPI/PG/Python; Done:Auth✅; →tech`

## Installation

### Build from Source
```bash
# Requirements: CMake 3.16+, C++17, zlib
mkdir build && cd build
cmake ..
make -j$(nproc)
```

## Usage

### CLI
```bash
# Compress a file (output will be README.md.lz by default)
llmzip c README.md

# Compress with specific output and mode
llmzip compress README.md archive.lz -m lossy

# Decompress
llmzip d archive.lz decoded.txt

# View compression statistics
llmzip s archive.lz
```

## Compression Modes

| Mode | Description | Ratio | Use Case |
|------|-------------|-------|----------|
| `lossy` | Deterministic Protocol + LLM | 70-95% | AI-to-AI communication, Knowledge bases |
| `lossless` | Standard zlib deflate | 30-40% | General purpose storage |
| `hybrid` | Protocol + zlib | 50-60% | Balanced efficiency |

## Architecture

- **Core**: Binary format and orchestration.
- **ProtocolParser**: Regex-based entity extraction and template assembly.
- **InferenceEngine**: ONNX Runtime wrapper for optional LLM layers.
- **CLI**: Built with CLI11 for robust command-line interaction.

## License
MIT
