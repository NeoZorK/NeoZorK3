#include <iostream>
#include <chrono>
#include "main.h" // Подключаем наш пустой заголовок

int main(int argc, char* argv[]) {
    
    auto start_time = std::chrono::high_resolution_clock::now();

    // Подавляем предупреждения о неиспользуемых параметрах
    (void)argc;
    (void)argv;

    std::cout << "NeoZorK3 Minimal Build - OK" << std::endl;
    
    auto end_time = std::chrono::high_resolution_clock::now();

      auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

      std::cout << "Время работы программы: " << duration << " мс" << std::endl;

    return 0;
}
