#pragma once

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <vector>

#include <sys/mman.h>

#include <boost/iostreams/device/mapped_file.hpp>

#include "word_counter.hpp"

namespace mmap_impl {
inline bool is_english_letter(const char ch) {
    return (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z');
}

inline char tolower(const char ch) {
    if (ch >= 'A' && ch <= 'Z') {
        return ch - ('Z' - 'z');
    }
    return ch;
}

struct MmapWordCounter : public IWordCounter {
    void count_words(const std::string &input_filename, const std::string &output_filename) override {
        std::unordered_map<std::string, int> counts;
        std::string cross_buffer_word;

        if (std::filesystem::file_size(input_filename) != 0) {
            boost::iostreams::mapped_file_source file;
            file.open(input_filename);
            if (!file.is_open()) {
                throw std::runtime_error("Could not open input file: " + input_filename);
            }

            if (file.is_open() && file.size() > 0) {
                madvise(const_cast<char *>(file.data()), file.size(), MADV_SEQUENTIAL);

                const char *begin = file.data();
                const char *end = begin + file.size();

                const char *word_start = nullptr;
                for (const char *p = begin; p < end; ++p) {
                    if (is_english_letter(*p)) {
                        if (!word_start) {
                            word_start = p;
                        }
                    } else {
                        if (word_start) {
                            std::string word(word_start, p);
                            std::ranges::transform(word, word.begin(), mmap_impl::tolower);
                            counts[word]++;
                            word_start = nullptr;
                        }
                    }
                }
                if (word_start) {
                    std::string word(word_start, end);
                    std::ranges::transform(word, word.begin(), mmap_impl::tolower);
                    counts[word]++;
                }
            }
        }

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
}
