#include <iostream>
#include <memory>
#include <string>

#include "impl/naive.hpp"

using BenchmarkTarget = NaiveWordCounter;

int main(const int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <input_file> <output_file>" << std::endl;
        return 1;
    }

    const std::string input_file = argv[1];
    const std::string output_file = argv[2];

    try {
        const auto counter = std::make_unique<BenchmarkTarget>();
        counter->count_words(input_file, output_file);
    } catch (const std::exception& e) {
        std::cerr << "An error occurred during execution: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}