Proj:llmzip; Type:LLM-Compressor(CLI); →tech; →no-meta
Stack:C++17/CMake/ONNX/CLI11/Py/Torch; Arch:CLI11→Core→ONNX→Proto→Model(T5/BART)
Mode:Lossy(↓70%)/Lossless(↓30%)/Hybrid(↓50%)
IO:Stdin/Stdout(BPM/AI); CrossPL:Lin/Mac/Win; Perf:C++/ONNX
CLI:compress/decode/stats; Pipe:echo|llmzip; StdIO:cat|...
Fmt:.lz→Hdr(32B:Magic/Ver/Type/Sz)+Payload+CRC32
Dev:src(cli/core/onnx/gui); res; scripts; Conv:flan-t5-small→ONNX(INT8)
Perf:T5@INT8:60MB/200RAM/50-100t/s; DistilBART@INT8:80MB/300RAM/40-80t/s; HW:i7-12700K
Lic:MIT; Roadmap:⬜️GUI(DearImGui); ~CustomModels; ~Batch; ~Stream; ~Plugins
Meta:fp:auto; ver:2.0; →roundtrip:✓✓