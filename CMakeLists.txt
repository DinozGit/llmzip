cmake_minimum_required(VERSION 3.16)
project(llmzip VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Опции сборки
option(BUILD_GUI "Build GUI for Windows" OFF)
option(USE_STATIC_RUNTIME "Link static runtime (Windows)" OFF)
option(EMBED_MODEL "Embed ONNX model into binary" OFF)

# Поиск зависимостей
find_package(onnxruntime REQUIRED)
find_package(CLI11 CONFIG QUIET)

# Если CLI11 не найден системно, используем submodule
if(NOT CLI11_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        CLI11
        GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
        GIT_TAG v2.3.2
    )
    FetchContent_MakeAvailable(CLI11)
endif()

# Исходники ядра
set(CORE_SOURCES
    src/core/binary_format.cpp
    src/core/protocol_parser.cpp
    src/core/compressor.cpp
    src/onnx/inference_engine.cpp
    src/onnx/model_loader.cpp
    src/cli/commands.cpp
)

if(EMBED_MODEL)
    # Embed ONNX model as a C source file via xxd (portable, always works)
    set(MODEL_ONNX "${CMAKE_SOURCE_DIR}/build/model.onnx")
    if(EXISTS "${MODEL_ONNX}")
        message(STATUS "Embedding ONNX model via xxd: ${MODEL_ONNX}")
        add_custom_command(
            OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/model_embedded.c
            COMMAND xxd -i "${MODEL_ONNX}" > model_embedded.c
            DEPENDS "${MODEL_ONNX}"
            COMMENT "Converting ONNX model to C source..."
        )
        add_custom_target(model_embedded DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/model_embedded.c)
        list(APPEND CORE_SOURCES ${CMAKE_CURRENT_BINARY_DIR}/model_embedded.c)
        set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/model_embedded.c PROPERTIES GENERATED TRUE)
    else()
        message(WARNING "EMBED_MODEL enabled but model.onnx not found at ${MODEL_ONNX}")
    endif()
    add_definitions(-DEMBED_MODEL)
endif()

# Основная библиотека ядра
add_library(llmzip_core STATIC
    ${CORE_SOURCES}
)

add_dependencies(llmzip_core model_embedded)

target_include_directories(llmzip_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${onnxruntime_INCLUDE_DIRS}
)

target_link_libraries(llmzip_core PUBLIC
    onnxruntime
    CLI11::CLI11
    ZLIB::ZLIB
)

# Платформенные настройки
if(WIN32)
    target_compile_definitions(llmzip_core PRIVATE PLATFORM_WINDOWS)
    if(USE_STATIC_RUNTIME)
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    endif()

    if(BUILD_GUI)
        # Поиск SDL2 и ImGui для GUI
        find_package(SDL2 CONFIG QUIET)
        add_executable(llmzip_gui WIN32
            src/gui/main.cpp
            src/gui/app_window.cpp
            ${CORE_SOURCES}
        )
        target_link_libraries(llmzip_gui PRIVATE llmzip_core SDL2::SDL2)
    endif()
elseif(APPLE)
    target_compile_definitions(llmzip_core PRIVATE PLATFORM_MACOS)
elseif(UNIX)
    target_compile_definitions(llmzip_core PRIVATE PLATFORM_LINUX)
endif()

# CLI исполняемый файл
add_executable(llmzip
    src/cli/main.cpp
)

add_dependencies(llmzip model_embedded)

target_link_libraries(llmzip PRIVATE llmzip_core)

# Установка
install(TARGETS llmzip DESTINATION bin)
if(BUILD_GUI AND WIN32)
    install(TARGETS llmzip_gui DESTINATION bin)
endif()



# find_package zlib
find_package(ZLIB REQUIRED)

message(STATUS "llmzip build configuration:")
message(STATUS "  - Build GUI: ${BUILD_GUI}")
message(STATUS "  - Platform: ${CMAKE_SYSTEM_NAME}")
message(STATUS "  - C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  - Core sources: binary_format protocol_parser compressor")
message(STATUS "  - ONNX: inference_engine model_loader")
