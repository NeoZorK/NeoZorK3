# Minimum required CMake version
cmake_minimum_required(VERSION 3.16)

# Project name
project(NeoZorK3 VERSION 1.0 LANGUAGES CXX)

# Set C++ standard (C++17)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- Dependencies ---
include(FetchContent)

FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG        v3.11.3 # Use an up-to-date stable tag/commit
)
FetchContent_MakeAvailable(nlohmann_json)
# nlohmann_json::nlohmann_json target is now available but not linked yet


# --- Executable ---
# Просто собираем main.cpp в исполняемый файл neozork3_cli
add_executable(neozork3_cli
                            src/main.cpp
                            src/config_manager.cpp
                            src/cli_parser.cpp)

# Specify the include path for the executable
# Указываем путь к нашим заголовкам (папка include/)
target_include_directories(neozork3_cli PUBLIC include)

# Явно добавляем путь к заголовкам nlohmann_json для neozork3_cli,
# так как config_manager.cpp сейчас часть этой цели и включает json.hpp
target_include_directories(neozork3_cli PRIVATE ${nlohmann_json_SOURCE_DIR}/include)


# --- Compile/Link Options ---

# Общие опции для всех сборок (предупреждения)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(neozork3_cli PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Опции для конкретных конфигураций с использованием $<CONFIG:...>

# Debug: добавляем только отладочные символы
target_compile_options(neozork3_cli PRIVATE $<$<CONFIG:Debug>:-g>)

# Release: высокая оптимизация (-O3), отключаем NDEBUG
target_compile_options(neozork3_cli PRIVATE $<$<CONFIG:Release>:-O3 -DNDEBUG>)
# Можно добавить удаление символов (strip) для Release, если нужно уменьшить размер
# target_link_options(neozork3_cli PRIVATE $<$<CONFIG:Release>:-s>)

# RelWithDebInfo: оптимизация (-O2 часто лучше для отладки, чем O3) + символы
target_compile_options(neozork3_cli PRIVATE $<$<CONFIG:RelWithDebInfo>:-O2 -g -DNDEBUG>)

# MinSizeRel: оптимизация для размера (-Os), отключаем NDEBUG, удаляем символы (strip)
target_compile_options(neozork3_cli PRIVATE $<$<CONFIG:MinSizeRel>:-Os -DNDEBUG>)
target_link_options(neozork3_cli PRIVATE $<$<CONFIG:MinSizeRel>:-s>) # Удаляем символы для минимального размера

# --- Опционально: Link Time Optimization (LTO) ---
# Может уменьшить размер и улучшить производительность, но увеличивает время линковки.
# Можно добавить для Release или MinSizeRel. Раскомментируйте для включения.
# target_compile_options(neozork3_cli PRIVATE $<$<CONFIG:Release>:-flto>)
# target_link_options(neozork3_cli PRIVATE $<$<CONFIG:Release>:-flto>)
# target_compile_options(neozork3_cli PRIVATE $<$<CONFIG:MinSizeRel>:-flto>)
# target_link_options(neozork3_cli PRIVATE $<$<CONFIG:MinSizeRel>:-flto>)

# Пере-Сборка одной командой (находясь в папке build)
# cd .. && rm -rf build && mkdir build && cd build && cmake .. -G Xcode -DCMAKE_BUILD_TYPE=Release && cmake --build . --config Release”
