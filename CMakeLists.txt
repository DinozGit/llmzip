cmake_minimum_required(VERSION 3.16)

# Fix for newer CMake versions (4.x+) that removed support for very old versions
if(POLICY CMP0129)
  cmake_policy(SET CMP0129 NEW)
endif()
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

project(llmzip VERSION 1.0.0 LANGUAGES CXX C) # Added C for zlib

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Options
option(EMBED_MODEL "Embed lightweight resources (tokenizer)" ON)

# Dependencies
find_package(ZLIB QUIET)
if(NOT ZLIB_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        zlib
        GIT_REPOSITORY https://github.com/madler/zlib.git
        GIT_TAG v1.3.1
    )
    FetchContent_MakeAvailable(zlib)
    # Map zlib to ZLIB::ZLIB for consistency
    if(NOT TARGET ZLIB::ZLIB)
        add_library(ZLIB::ZLIB ALIAS zlibstatic)
        target_include_directories(zlibstatic INTERFACE ${zlib_SOURCE_DIR} ${zlib_BINARY_DIR})
    endif()
endif()
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(SPIECE sentencepiece)
    if(SPIECE_FOUND)
        add_library(SPIECE::SPIECE INTERFACE IMPORTED)
        set_target_properties(SPIECE::SPIECE PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${SPIECE_INCLUDE_DIRS}"
            INTERFACE_LINK_LIBRARIES "${SPIECE_LIBRARIES}")
    endif()
endif()

# CLI11
find_package(CLI11 CONFIG QUIET)
if(NOT CLI11_FOUND)
    include(FetchContent)
    FetchContent_Declare(CLI11 GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git GIT_TAG v2.3.2)
    FetchContent_MakeAvailable(CLI11)
endif()

# Core Sources
set(CORE_SOURCES
    src/core/binary_format.cpp
    src/core/protocol_parser.cpp
    src/core/compressor.cpp
    src/onnx/inference_engine.cpp
    src/onnx/model_loader.cpp
    src/onnx/bpe_tokenizer.cpp
    src/cli/commands.cpp
)

if(EMBED_MODEL)
    set(MODEL_SPIECE "${CMAKE_SOURCE_DIR}/resources/spiece.model")
    if(EXISTS "${MODEL_SPIECE}")
        add_custom_command(
            OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/spiece_embedded.cpp
            COMMAND xxd -i -n spiece_model "${MODEL_SPIECE}" > ${CMAKE_CURRENT_BINARY_DIR}/spiece_embedded.cpp
            DEPENDS "${MODEL_SPIECE}"
        )
        add_custom_target(spiece_embedded DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/spiece_embedded.cpp)
        list(APPEND CORE_SOURCES "${CMAKE_CURRENT_BINARY_DIR}/spiece_embedded.cpp")
        set_source_files_properties("${CMAKE_CURRENT_BINARY_DIR}/spiece_embedded.cpp" PROPERTIES GENERATED TRUE)
        add_definitions(-DEMBED_MODEL)
    endif()
endif()

add_library(llmzip_core STATIC ${CORE_SOURCES})

# Fix trigraph warnings from BART vocab data
if(MSVC)
    target_compile_options(llmzip_core PRIVATE /wd4444) # Disable trigraph-like warnings in MSVC
else()
    target_compile_options(llmzip_core PRIVATE -Wno-trigraphs)
endif()

if(TARGET spiece_embedded)
    add_dependencies(llmzip_core spiece_embedded)
endif()

target_include_directories(llmzip_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(llmzip_core PUBLIC ZLIB::ZLIB CLI11::CLI11)

if(NOT WIN32)
    target_link_libraries(llmzip_core PUBLIC pthread)
endif()
if(TARGET SPIECE::SPIECE)
    target_link_libraries(llmzip_core PUBLIC SPIECE::SPIECE)
endif()

add_executable(llmzip src/cli/main.cpp)
target_link_libraries(llmzip PRIVATE llmzip_core)

install(TARGETS llmzip DESTINATION bin)
