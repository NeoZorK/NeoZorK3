// include/main.h

#ifndef MAIN_H // Стандартный include guard (вариант 1)
#define MAIN_H

// #pragma once // Альтернативный include guard (вариант 2, менее переносимый но короче)

// Если объявлениям функций нужны стандартные типы, включите их здесь
// Например:
// #include <string>
// #include <vector>

// --- Function Declarations ---
// Объявления функций, которые реализованы в main.cpp

/**
 * @brief Parses command line arguments.
 * @param argc Argument count.
 * @param argv Argument values.
 */
void parse_arguments(int argc, char* argv[]);

/**
 * @brief Runs the main arbitrage logic of the application.
 */
void run_arbitrage_logic();

// Можно добавить сюда другие объявления, если они относятся к main

#endif // MAIN_H