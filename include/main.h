// include/main.h

#ifndef MAIN_H
#define MAIN_H

// --- Function Declarations ---

/**
 * @brief Parses command line arguments.
 * @param argc Argument count.
 * @param argv Argument values.
 */
void parse_arguments(int argc, char* argv[]);

/**
 * @brief Queries the Fantom blockchain for the latest block number and latency.
 */
void query_fantom_block(); // ИЗМЕНЕНО

#endif // MAIN_H
