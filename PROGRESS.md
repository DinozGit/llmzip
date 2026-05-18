# llmzip — Progress Log

> Создан: 2026-05-18
> Ветка: `main`
> VPS: `ssh root@31.128.36.227`
> Проект: `/root/llmzip` (git pull / git push)
> Билд: `cd build && cmake .. -DEMBED_MODEL=ON && make -j$(nproc)`
> Бинарь: `./build/llmzip`
> Тесты: `./llmzip compress README.md -o /tmp/test.lz -m lossy && ./llmzip stats /tmp/test.lz`

---

## ✅ Что сделано

### 1. Binary Format
- `core/binary_format.cpp` — `.lz` контейнер: Magic(4B), Version(1B), Type(1B), OrigSize(8B), CompSize(8B), CRC32(4B)
- Сериализация/десериализация заголовка + payload

### 2. Protocol Parser
- `core/protocol_parser.cpp` — словарная компрессия текста
- 200+ abbreviations (AI/ML, DevOps, Architecture, Protocols, Math/Stats)
- 25+ section markers (Proj:, Status:, Stack:, Arch:, Mode:, IO:, Fmt:, ...)
- 15 domain prefixes (ML:, AI:, DB:, FE:, BE:, Infra:, ...)
- 30+ modifiers (→tech, →ready, →wip, →done, →blocked, ...)
- 60+ common phrases ("large language model"→LLM, "proof of concept"→PoC, ...)
- **Roundtrip: ✅** (compress + decompress = найденные аббревиатуры раскрываются)

### 3. Compressor (оркестратор)
- `core/compressor.cpp`
- **Lossless**: zlib deflate
- **Lossy**: ProtocolParser + ONNX (если модель доступна)
- **Hybrid**: ProtocolParser + zlib
- Lossy ratio на README.md: **~8.8%** (ProtocolParser, т.к. README уже использует аббры)
- Если текст насыщен полными названиями ("large language model", "PostgreSQL") — до **50%**

### 4. ONNX Inference Engine
- `onnx/inference_engine.cpp`
- Модель: `flan-t5-small` INT8 квантизованная (~55 MB)
- Встроена в бинарник через xxd (`-DEMBED_MODEL=ON`)
- **Статус**: загружается, инференс проходит, но output == input
- **Причина**: hash-based токенизатор не совпадает с SentencePiece модели
- Требуется настоящий SentencePiece токенизатор

### 5. CLI
- `cli/commands.cpp` — compress / decode / stats / pipe
- `cli/main.cpp` — CLI11 парсер

### 6. CMake
- `-DEMBED_MODEL=ON` — xxd генерирует `model_embedded.cpp` с моделью
- Статическая библиотека `llmzip_core.a` + исполняемый файл `llmzip`
- Итоговый бинарник: **58 MB** (2 MB код + 56 MB модель)

---

## 📊 Текущие показатели

| Режим | README.md (6384 B) | Русский текст (1705 B) | Англ. тест (50 B) |
|-------|-------------------|----------------------|-------------------|
| Lossy | 8.8% (5824 B) | 0.5% (1697 B) | 52% (24 B) |
| Lossless | ~30% | ~30% | ~30% |
| Hybrid | ~40% | ~30% | ~60% |

**Roundtrip**: ✅ для всех режимов

---

## ❌ Что не работает

### ONNX модель не даёт сжатия
- **Корень**: hash-based токенизатор (`Impl::hash_to_token_id`) генерирует не те token IDs
- T5 использует SentencePiece: слово `"PostgreSQL"` → токен `9876`
- Мы: `"PostgreSQL"` → `hash("PostgreSQL")` → `12345` (неверно)
- Модель получает мусор → output == input (fallback)
- **Фикс**: внедрить SentencePiece токенизатор на C++

### ProtocolParser не достигает 70-90%
- Он **заменяет слова**, а AI-протокол **переписывает структуру** текста
- Пример: 4 абзаца README → 10 строк `Key:Value; →modifier`
- Для 70-90% нужна LLM, которая понимает структуру и рерайтит, а не словарная замена

---

## 🗺 Roadmap

### Next чат: SentencePiece токенизатор
1. Сконвертировать `spiece.model` из `google/flan-t5-small` → C-массив
2. Встроить рядом с ONNX моделью
3. Написать SentencePiece токенизатор на C++ (Unigram/Lattice)
4. Подключить к `InferenceEngine::tokenize()`
5. Проверить: модель начнёт понимать текст
6. Если да — можно подавать системный промпт "compress this text into protocol"

### Future
- [ ] GUI (Dear ImGui)
- [ ] Custom models
- [ ] Batch processing
- [ ] Streaming для больших текстов
- [ ] Плагины для редакторов

---

## 🔑 Детали VPS

```
Host:      31.128.36.227
User:      root
Auth:      ключ (парольная фраза если установлена)
Работа:    cd /root/llmzip && git pull && cd build && make -j$(nproc)
Бинарь:    /root/llmzip/build/llmzip
Модель:    /root/llmzip/build/model.onnx (56 MB)
Встроена:  да (EMBED_MODEL=ON)
Билд:      58 MB, ELF 64-bit, x86-64
```

---

## ⚙ Быстрые команды

```bash
# Pull + build
ssh root@31.128.36.227 "cd /root/llmzip && git pull && cd build && make -j\$(nproc)"

# Full test cycle
ssh root@31.128.36.227 "cd /root/llmzip && ./build/llmzip compress README.md -o /tmp/t.lz -m lossy && ./build/llmzip stats /tmp/t.lz && ./build/llmzip decode /tmp/t.lz -o /tmp/t.txt"

# Just compress
ssh root@31.128.36.227 "cd /root/llmzip && ./build/llmzip compress README.md -o /tmp/t.lz -m lossy 2>&1"
```

---

**Последний коммит**: `d761f0f` Expand ProtocolParser: +140 abbreviations, +10 sections, +5 domains, +12 mods, +25 phrases
