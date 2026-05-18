# Быстрый старт для разработчиков llmzip

## Предварительные требования

### Linux (Ubuntu/Debian)
```bash
sudo apt-get update
sudo apt-get install -y cmake g++ git libonnxruntime-dev
```

### macOS
```bash
brew install cmake onnxruntime
```

### Windows
1. Установите [Visual Studio Build Tools](https://visualstudio.microsoft.com/visual-cpp-build-tools/)
2. Установите [CMake](https://cmake.org/download/)
3. Установите ONNX Runtime через vcpkg:
   ```powershell
   choco install vcpkg
   vcpkg install onnxruntime:x64-windows
   ```

## Сборка проекта

```bash
cd /workspace/llmzip
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)  # Linux/macOS
# или cmake --build . --config Release  # Windows
```

## Подготовка модели

```bash
# Установка Python зависимостей
pip install -r scripts/requirements.txt

# Конвертация модели T5-small в ONNX
python scripts/convert_model.py --model google/flan-t5-small --output ../resources

# Скопируйте итоговый файл model.onnx в папку resources/
```

## Запуск тестов

```bash
# Проверка версии
./llmzip --version

# Тест сжатия
echo "Hello World! This is a test compression." | ./llmzip compress - -o test.lz -m lossy

# Тест распаковки
./llmzip decode test.lz -o decoded.txt

# Просмотр статистики
./llmzip stats test.lz
```

## Структура проекта

```
llmzip/
├── src/
│   ├── cli/           # CLI интерфейс
│   │   ├── main.cpp   # Точка входа, парсинг аргументов
│   │   └── commands.h # Обработчики команд
│   ├── core/          # Ядро компрессии (header-only пока)
│   │   ├── compressor.h
│   │   ├── protocol_parser.h
│   │   └── binary_format.h
│   ├── onnx/          # ONNX интеграция (header-only пока)
│   │   ├── inference_engine.h
│   │   └── model_loader.h
│   └── stubs.cpp      # Временные заглушки для сборки
├── resources/         # Модели и файлы правил
├── scripts/
│   ├── convert_model.py  # Скрипт конвертации моделей
│   └── requirements.txt  # Python зависимости
├── .github/workflows/ # CI/CD конфигурация
├── CMakeLists.txt     # Конфигурация сборки
└── README.md          # Документация
```

## Следующие шаги разработки

1. **Реализация ядра** (`src/core/*.cpp`):
   - `compressor.cpp` - основная логика сжатия
   - `protocol_parser.cpp` - парсер правил из `ядро сжатия.md`
   - `binary_format.cpp` - работа с форматом .lz

2. **Интеграция ONNX** (`src/onnx/*.cpp`):
   - `inference_engine.cpp` - инференс модели
   - `model_loader.cpp` - загрузка и валидация моделей

3. **CLI команды** (`src/cli/commands.cpp`):
   - Полная реализация compress/decompress/stats

4. **GUI для Windows** (опционально):
   - `src/gui/main.cpp`
   - `src/gui/app_window.cpp`

## Интеграция с существующим проектом

Для использования протокола из `ядро сжатия.md`:
1. Скопируйте файл в `resources/ядро сжатия.md`
2. Реализуйте парсер в `protocol_parser.cpp`
3. Добавьте правила в `load_default_rules()`

## Отладка

```bash
# Сборка с отладочной информацией
cmake .. -DCMAKE_BUILD_TYPE=Debug

# Запуск с verbose выводом
./llmzip compress input.txt output.lz -m lossy -v

# Использование GDB/LLDB
gdb --args ./llmzip compress input.txt output.lz -m lossy
```

## Лицензия

MIT License
