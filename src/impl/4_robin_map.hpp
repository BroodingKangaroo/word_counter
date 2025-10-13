#pragma once

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

#include <sys/mman.h>
#include <boost/iostreams/device/mapped_file.hpp>

#include <third-party/robin-map/robin_map.h>

#include "word_counter.hpp"

namespace robin_map {

struct string_hash {
    using is_transparent = void;
    [[nodiscard]] size_t operator()(const std::string_view txt) const {
        return std::hash<std::string_view>{}(txt);
    }
};

inline bool is_english_letter(const char ch) {
    return (ch >= 'A' && 'Z' >= ch) || (ch >= 'a' && 'z' >= ch);
}

inline char tolower(const char ch) {
    if (ch >= 'A' && 'Z' >= ch) {
        return ch - ('Z' - 'z');
    }
    return ch;
}

struct RobinMapWordCounter : public IWordCounter {
    void count_words(const std::string &input_filename, const std::string &output_filename) override {
        tsl::robin_map<std::string, int, string_hash, std::equal_to<>> counts;

        if (std::filesystem::file_size(input_filename) == 0) {
            std::ofstream out(output_filename);
            return;
        }

        boost::iostreams::mapped_file_source file;
        file.open(input_filename);
        if (!file.is_open()) {
            throw std::runtime_error("Could not open input file: " + input_filename);
        }

        madvise(const_cast<char *>(file.data()), file.size(), MADV_SEQUENTIAL);

        const char *begin = file.data();
        const char *end = begin + file.size();
        const char *word_start = nullptr;

        constexpr size_t StackBufferSize = 64;
        char stack_buffer[StackBufferSize];
        std::string long_word_buffer;

        for (const char *p = begin; p < end; ++p) {
            if (is_english_letter(*p)) {
                if (!word_start) {
                    word_start = p;
                }
            } else {
                if (word_start) {
                    const size_t word_len = p - word_start;

                    if (word_len < StackBufferSize) {
                        std::transform(word_start, p, stack_buffer, tolower);
                        std::string_view key_sv(stack_buffer, word_len);

                        if (const auto it = counts.find(key_sv); it != counts.end()) {
                            // Fast path doesn't allocate memory
                            it.value<>()++;
                        } else {
                            counts.insert({std::string(key_sv), 1});
                        }
                    } else {
                        long_word_buffer.assign(word_start, p);
                        std::ranges::transform(long_word_buffer, long_word_buffer.begin(), tolower);
                        counts[long_word_buffer]++;
                    }
                    word_start = nullptr;
                }
            }
        }

        if (word_start) {
             const size_t word_len = end - word_start;
             if (word_len < StackBufferSize) {
                std::transform(word_start, end, stack_buffer, tolower);
                std::string_view key_sv(stack_buffer, word_len);

                if (const auto it = counts.find(key_sv); it != counts.end()) {
                    it.value<>()++;
                } else {
                    counts.insert({std::string(key_sv), 1});
                }
             } else {
                long_word_buffer.assign(word_start, end);
                std::ranges::transform(long_word_buffer, long_word_buffer.begin(), tolower);
                counts[long_word_buffer]++;
             }
        }

        file.close();

        std::ofstream out(output_filename);
        if (!out.is_open()) {
            throw std::runtime_error("Could not open output file: " + output_filename);
        }

        std::vector<std::pair<std::string, int>> sorted_counts(counts.begin(), counts.end());
        std::ranges::sort(sorted_counts, [](const auto &lhs, const auto &rhs) {
            if (lhs.second != rhs.second) {
                return lhs.second > rhs.second;
            }
            return lhs.first < rhs.first;
        });

        for (const auto &pair : sorted_counts) {
            out << pair.second << " " << pair.first << "\n";
        }
    }
};
} // namespace robin_map