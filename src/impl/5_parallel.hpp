#pragma once

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>
#include <thread>
#include <future>

#if defined(__APPLE__) && defined(__clang__)
    #include <oneapi/dpl/algorithm>
    #include <oneapi/dpl/execution>
#else
    #include <execution>
#endif

#include <sys/mman.h>
#include <boost/iostreams/device/mapped_file.hpp>

#include <third-party/robin-map/robin_map.h>

#include "word_counter.hpp"

namespace parallel {
struct string_hash {
    using is_transparent = void;

    [[nodiscard]] size_t operator()(const std::string_view txt) const {
        return std::hash<std::string_view>{}(txt);
    }

    [[nodiscard]] size_t operator()(const std::string &txt) const {
        return std::hash<std::string>{}(txt);
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

using word_count_map = tsl::robin_map<std::string, int, string_hash, std::equal_to<> >;


word_count_map count_words_in_chunk(const char *chunk_start, const char *chunk_end, const char *file_begin,
                                    const char *file_end, const int thread_id) {
    word_count_map local_counts;
    if (thread_id > 0) {
        // Processing first word that could potentially be split between chunks
        while (chunk_start > file_begin && is_english_letter(*(chunk_start - 1))) {
            --chunk_start;
        }
    }

    const char *word_start = nullptr;
    constexpr size_t StackBufferSize = 128;
    char stack_buffer[StackBufferSize];
    std::string long_word_buffer;

    for (const char *p = chunk_start; p < chunk_end; ++p) {
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
                    if (const auto it = local_counts.find(key_sv); it != local_counts.end()) {
                        it.value()++;
                    } else {
                        local_counts.insert({std::string(key_sv), 1});
                    }
                } else {
                    long_word_buffer.assign(word_start, p);
                    std::ranges::transform(long_word_buffer, long_word_buffer.begin(), tolower);
                    local_counts[long_word_buffer]++;
                }
                word_start = nullptr;
            }
        }
    }

    // Last word of the chunk is only added if it's the last chunk
    if (chunk_end == file_end && word_start) {
        long_word_buffer.assign(word_start, chunk_end);
        std::ranges::transform(long_word_buffer, long_word_buffer.begin(), tolower);
        local_counts[long_word_buffer]++;
    }

    return local_counts;
}

struct ParallelWordCounter : public IWordCounter {
    void count_words(const std::string &input_filename, const std::string &output_filename) override {

        // -------------------- Preparing input file --------------------

        const auto file_size = std::filesystem::file_size(input_filename);
        if (file_size == 0) {
            std::ofstream out(output_filename);
            return;
        }

        boost::iostreams::mapped_file_source file;
        file.open(input_filename);
        if (!file.is_open()) {
            throw std::runtime_error("Could not open input file: " + input_filename);
        }
        madvise(const_cast<char *>(file.data()), file.size(), MADV_SEQUENTIAL);

        // -------------------- Calculating frequencies --------------------

        const char* const begin = file.data();
        const char* const end = begin + file.size();
        const unsigned int num_threads = std::thread::hardware_concurrency();
        const size_t chunk_size = file_size / num_threads;
        std::vector<std::future<word_count_map>> futures;
        for (int i = 0; i < num_threads; ++i) {
            const char* chunk_start = begin + i * chunk_size;
            const char* chunk_end = (i == num_threads - 1) ? end : (begin + (i + 1) * chunk_size);
            futures.push_back(std::async(std::launch::async, count_words_in_chunk, chunk_start, chunk_end, begin, end, i));
        }

        word_count_map final_counts;
        for (auto& future : futures) {
            for (const auto& [word, count] : future.get()) {
                final_counts[word] += count;
            }
        }

        file.close();

        // -------------------- Aggregating results --------------------

        std::vector<std::pair<std::string, int> > sorted_counts(final_counts.begin(), final_counts.end());

        std::sort(std::execution::par_unseq, sorted_counts.begin(), sorted_counts.end(), [](const auto &lhs, const auto &rhs) {
            if (lhs.second != rhs.second) {
                return lhs.second > rhs.second;
            }
            return lhs.first < rhs.first;
        });

        // -------------------- Saving to output file --------------------

        std::ofstream out(output_filename);
        if (!out.is_open()) {
            throw std::runtime_error("Could not open output file: " + output_filename);
        }

        std::string output_buffer;
        constexpr size_t BufferCapacity = 1024 * 128; // 128KB buffer
        output_buffer.reserve(BufferCapacity);

        for (const auto &[word, frequency]: sorted_counts) {
            output_buffer.append(std::to_string(frequency));
            output_buffer.push_back(' ');
            output_buffer.append(word);
            output_buffer.push_back('\n');

            if (output_buffer.size() >= BufferCapacity) {
                out.write(output_buffer.data(), output_buffer.size());
                output_buffer.clear();
            }
        }
        if (!output_buffer.empty()) {
            out.write(output_buffer.data(), output_buffer.size());
        }
    }
};
}
