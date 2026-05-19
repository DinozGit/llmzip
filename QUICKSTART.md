# Quickstart Guide

Get `llmzip` up and running in 5 minutes.

## 1. Prerequisites

- **Linux/macOS**: `cmake`, `g++` or `clang`, `zlib1g-dev`
- **Windows**: Visual Studio 2019+ or MinGW, CMake

## 2. Build

```bash
git clone https://github.com/DinozGit/llmzip.git
cd llmzip
mkdir build && cd build
cmake ..
make -j$(nproc)
```

## 3. Basic Usage

### Compress
Compress any technical text file into the semantic protocol:
```bash
./llmzip compress ../README.md -o test.lz -m lossy
```

### Decompress
Expand the protocol markers back into readable text:
```bash
./llmzip decode test.lz -o decoded.txt
```

### Stats
Check compression ratio and integrity:
```bash
./llmzip stats test.lz
```

## 4. Advanced: LLM Integration

To enable Level 2 (LLM Polish), you need to provide an ONNX model:
1. Download a supported model (e.g., T5-small or BART).
2. Place it in the `resources/` folder.
3. Run with the model path:
```bash
./llmzip compress input.txt -o out.lz -m lossy --model resources/model.onnx
```

## 5. Integration

`llmzip` works perfectly with pipes:
```bash
echo "Project: MyApp; Stack: Python/FastAPI" | ./llmzip compress - -o - | ./llmzip decode - -o -
```
