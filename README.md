# llmzip - LLM-powered Compression Tool

Кроссплатформенный инструмент сжатия текста на базе нейросетей (ONNX Runtime).

## Возможности

- **Семантическое сжатие (lossy)**: Сокращение текста до 70% с сохранением смысла
- **Токенное сжатие (lossless)**: Без потерь, как классический архиватор
- **Гибридный режим**: Комбинация обоих подходов
- **Кроссплатформенность**: Linux, macOS, Windows (один бинарник)
- **Интеграция**: Работа через stdin/stdout для BPM и AI-агентов
- **Быстродействие**: C++ + ONNX Runtime без тяжелых зависимостей

## Установка

### Сборка из исходников

```bash
# Требования: CMake 3.16+, C++17 компилятор, ONNX Runtime
mkdir build && cd build
cmake ..
make -j$(nproc)

# Для Windows с GUI:
cmake .. -DBUILD_GUI=ON -A x64
cmake --build . --config Release
```

### Загрузка готовых бинарников

См. раздел [Releases](https://github.com/your-org/llmzip/releases)

## Использование

### CLI

```bash
# Сжатие файла
llmzip compress input.txt output.lz -m lossy

# Распаковка
llmzip decode archive.lz decoded.txt

# Статистика сжатия
llmzip stats archive.lz

# Pipe режим (для интеграции)
echo "Ваш текст здесь" | llmzip compress - -o compressed.lz

# Чтение из stdin, вывод в stdout
cat input.txt | llmzip compress - -o - > output.lz
```

### Режимы сжатия

| Режим | Описание | Экономия | Потери |
|-------|----------|----------|--------|
| `lossy` | Семантическое сжатие через LLM | до 70% | Да (смысл сохраняется) |
| `lossless` | Токенное сжатие без потерь | ~30-40% | Нет |
| `hybrid` | Комбинированный подход | ~50-60% | Минимальные |

### Интеграция в BPM/AI-агенты

```python
# Пример вызова из Python
import subprocess

text = "Многословный запрос пользователя..."
result = subprocess.run(
    ["llmzip", "compress", "-", "-o", "-"],
    input=text.encode(),
    capture_output=True
)
compressed_data = result.stdout
```

```bash
# Пример в bash-скрипте
compressed=$(echo "$USER_QUERY" | llmzip compress - -o -)
# Отправка в API агента
curl -X POST https://ai-agent/api \
  -H "Content-Type: application/octet-stream" \
  --data-binary "$compressed"
```

## Формат файлов .lz

```
+------------------+---------------------+------------------+
| Заголовок (32B)  | Payload (variable)  | CRC32 (4B)       |
+------------------+---------------------+------------------+
| Magic: "LLMZ"    | Сжатые данные       | Чек-сумма        |
| Version: 1       |                     |                  |
| Type: lossy/etc  |                     |                  |
| Orig Size        |                     |                  |
| Comp Size        |                     |                  |
+------------------+---------------------+------------------+
```

## Архитектура

```
┌─────────────────────────────────────────────────────────┐
│                    llmzip binary                        │
├─────────────┬──────────────┬──────────────┬─────────────┤
│   CLI       │   Core       │   ONNX       │   Protocol  │
│   Parser    │   Engine     │   Runtime    │   Parser    │
│   (CLI11)   │              │              │             │
└─────────────┴──────────────┴──────────────┴─────────────┘
                              │
                              ▼
                    ┌─────────────────┐
                    │  Model (.onnx)  │
                    │  T5-small/BART  │
                    └─────────────────┘
```

## Разработка

### Структура проекта

```
llmzip/
├── src/
│   ├── cli/          # CLI интерфейс (main.cpp, commands.cpp)
│   ├── core/         # Ядро компрессии
│   ├── onnx/         # ONNX интеграция
│   └── gui/          # GUI для Windows (опционально)
├── resources/        # Модели и файлы правил
├── scripts/          # Утилиты (конвертация моделей)
└── CMakeLists.txt    # Конфигурация сборки
```

### Конвертация модели

```bash
# Установка зависимостей
pip install torch transformers optimum[onnxruntime] onnx

# Конвертация T5-small в ONNX с квантованием
python scripts/convert_model.py --model google/flan-t5-small --output ./resources

# Результат: resources/model.onnx (квантованная версия)
```

## Производительность

| Модель | Размер | RAM | Скорость (токенов/сек) |
|--------|--------|-----|------------------------|
| T5-small (INT8) | ~60 MB | ~200 MB | ~50-100 |
| DistilBART (INT8) | ~80 MB | ~300 MB | ~40-80 |

*Тесты на CPU Intel i7-12700K*

## Лицензия

MIT License

## Roadmap

- [ ] GUI для Windows (Dear ImGui)
- [ ] Поддержка кастомных моделей
- [ ] Пакетная обработка файлов
- [ ] Streaming режим для больших текстов
- [ ] Плагины для популярных редакторов
