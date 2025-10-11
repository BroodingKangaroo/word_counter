#pragma once

#include <algorithm>
#include <fstream>
#include <vector>
#include <unordered_map>

#include "word_counter.hpp"

struct BufferedWordCounter : public IWordCounter {
    void count_words(const std::string &input_filename, const std::string &output_filename) override {
        std::ifstream file(input_filename);
        if (!file.is_open()) {
            throw std::runtime_error("Could not open input file: " + input_filename);
        }

        std::unordered_map<std::string, int> counts;
        counts.max_load_factor(0.3);
        std::string current_word;

        // -- now reading in chunks of size 4kb (likely the size of a memory page)
        // -- even though the performance starts to decrease at about 100b and lower
        constexpr size_t buffer_size = 4096;
        std::vector<char> buffer(buffer_size);

        while (file.read(buffer.data(), buffer_size) || file.gcount() > 0) {
            for (size_t i = 0; i < file.gcount(); ++i) {
                if (std::isalpha(static_cast<unsigned char>(buffer[i]))) {
                    current_word += std::tolower(static_cast<unsigned char>(buffer[i]));
                } else {
                    if (!current_word.empty()) {
                        counts[current_word]++;
                        current_word.clear();
                    }
                }
            }
        }

        if (!current_word.empty()) {
            counts[current_word]++;
        }

        file.close();

        std::ofstream out(output_filename);
        if (!out.is_open()) {
            throw std::runtime_error("Could not open output file: " + output_filename);
        }

        std::vector<std::pair<std::string, int> > sorted_counts(counts.begin(), counts.end());
        std::ranges::sort(sorted_counts, [](const auto &lhs, const auto &rhs) {
            if (lhs.second != rhs.second) {
                return lhs.second > rhs.second;
            }
            return lhs.first < rhs.first;
        });

        for (const auto &pair: sorted_counts) {
            out << pair.second << " " << pair.first << "\n";
        }
    }
};
