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

**Последний коммит**: `5e7cdd2` Add PROGRESS.md with full session context for next chat

---

## 🎯 Вердикт

Архитектура зрелая, пайплайн корректно разделён, roundtrip-валидация работает. Единственный критический блокер — токенизатор + отсутствие seq2seq-декодирования в ONNX. Без этого LLM-слой останется заглушкой.

### Root Cause & Technical Fix

| Проблема | Причина | Решение |
|---|---|---|
| output == input | Hash-токенизатор ломает границы субслов. T5 ожидает SentencePiece (Unigram LM). Модель видит OOV-токены → игнорирует контекст. | Интегрировать sentencepiece C++ API или llama.cpp tokenizer. |
| Нет семантического сжатия | ONNX Runtime не поддерживает авто-регрессивную генерацию из коробки. Нужен цикл decoding + логиты. | Реализовать Greedy/Beam-декодирование вручную или подключить onnxruntime-genai. |
| ProtocolParser → 8.8% на README | Текст уже оптимизирован. Словарная замена не переписывает структуру. | Двухфазный пайплайн: ProtocolParser → AI-Structurer → Binary. |

---

## 🛠️ Пошаговый план внедрения (Next Chat)

### 1. SentencePiece Tokenizer (C++)

```python
# Экспорт модели из HuggingFace
pip install sentencepiece transformers
python -c "from transformers import AutoTokenizer; t=AutoTokenizer.from_pretrained('google/flan-t5-small'); t.sp_model.save('spiece.model')"
```

- Подключить sentencepiece как static lib (`-DSPM_ENABLE_SHARED=OFF`)
- В `InferenceEngine::tokenize()`: `sp.Encode(text, &ids)` + добавление decoder_start_token_id (0 для T5)
- Для detokenize: `sp.Decode(ids)` → string

### 2. ONNX Seq2Seq Decoding Loop

ONNX Runtime не генерирует текст автоматически. Нужен явный цикл:

```cpp
// Псевдокод
std::vector<int> generate(std::string prompt, int max_tokens=64) {
    auto ids = tokenize(prompt);
    for (int i=0; i<max_tokens; ++i) {
        auto logits = run_encoder_decoder(ids);
        int next_token = argmax(logits);
        if (next_token == EOS) break;
        ids.push_back(next_token);
    }
    return ids;
}
```

- Экспортировать модель в две части: `encoder.onnx` + `decoder.onnx` (с past_key_values для кэша)
- Или использовать `onnxruntime-genai` (поддержка T5 out-of-the-box)

### 3. Prompt Template для Сжатия

```
[INST] Compress this text into semantic markers.
Keep all facts, remove filler, use protocol syntax.
Max output: 150 tokens.
Text: {user_input} [/INST]
```

---

## 📈 Реалистичная стратегия сжатия

| Фаза | Механизм | Ожидаемое сжатие | Зависимости |
|---|---|---|---|
| v1 | ProtocolParser (словарь) | 10-30% | ✅ Готово |
| v2 | ProtocolParser + LLM-структурирование | 40-60% | ⏳ SentencePiece + Decoding |
| v3 | LLM-переписывание → Binary | 60-80% | ⏳ Prompt-tuning + Token budget |

⚠️ **Важно:** 70-90% достижимо только через семантический рерайт, а не замену слов. LLM должен:
- Вычленить сущности/метрики/требования
- Сопоставить с маркерами core_rules.md
- Упаковать в `Key:Value; →modifiers`
- Отбросить воду, повторы, пояснения

---

## ⏭️ Приоритеты на следующий спринт

- ✅ **SentencePiece интеграция (2-3 ч)** — без этого ONNX бесполезен
- ✅ **Decoding loop (3-4 ч)** — greedy first, beam later
- ✅ **Validation script (1 ч)** — `echo "test" | llmzip compress --llm` → roundtrip check
- 🔄 **Prompt engineering (2 ч)** — шаблоны под разные домены (Dev/AI/Biz)
- 📊 **Benchmark** — замер latency / RAM / compression_ratio на 3-х текстах

---

**Последний коммит**: `5e7cdd2` Add PROGRESS.md with full session context for next chat
