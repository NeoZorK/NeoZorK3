# Minimum required CMake version
cmake_minimum_required(VERSION 3.16)

# Project name
project(NeoZorK3 VERSION 1.0 LANGUAGES CXX)

# Set C++ standard (C++17)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- Executable ---
# Просто собираем main.cpp в исполняемый файл neozork3_cli
add_executable(neozork3_cli src/main.cpp)

# Specify the include path for the executable
# Указываем путь к нашим заголовкам (папка include/)
target_include_directories(neozork3_cli PUBLIC include)

# --- Optional: Basic Compile Options ---
# Можно добавить позже, если минимальная сборка пройдет
# if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
#     target_compile_options(neozork3_cli PRIVATE -Wall -Wextra -Wpedantic)
# endif()
# if(CMAKE_BUILD_TYPE MATCHES Debug)
#     target_compile_options(neozork3_cli PRIVATE -g)
# endif()