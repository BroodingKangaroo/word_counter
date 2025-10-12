#pragma once

#include <algorithm>
#include <array>
#include <deque>
#include <fstream>
#include <vector>

#include "word_counter.hpp"

class Trie {
    static constexpr size_t alphabet_size_ = 26;
    static constexpr size_t CHUNK_SIZE = 4096;
    struct TrieNode {
        int counter{};
        std::array<TrieNode*, alphabet_size_> children;
    };

    std::deque<std::vector<TrieNode>> node_pool_;
    TrieNode* head_;
    int number_of_words_{};
public:
    Trie() {
        head_ = get_new_node();
    }

    TrieNode* get_new_node() {
        if (node_pool_.empty() || node_pool_.back().size() == node_pool_.back().capacity()) {
            node_pool_.emplace_back();
            node_pool_.back().reserve(CHUNK_SIZE);
        }

        std::vector<TrieNode>& current_chunk = node_pool_.back();
        current_chunk.emplace_back();

        return &current_chunk.back();
    }

    void add_word(const std::string& str) {
        add_word(str.data(), str.data() + str.size());
    }

    void add_word(const char* begin, const char* end) {
        if (begin == end) {
            return;
        }
        TrieNode* current = head_;
        for (const char* p = begin; p != end; ++p) {
            const size_t char_index = get_index_from_char(*p);
            if (!current->children[char_index]) {
                current->children[char_index] = get_new_node();
            }
            current = current->children[char_index];
        }
        if (current->counter == 0) {
            number_of_words_++;
        }
        current->counter++;
    }

    std::vector<std::pair<std::string, int>> get_frequencies() const {
        std::string word;
        std::vector<std::pair<std::string, int>> result;
        result.reserve(number_of_words_);
        get_frequencies(word, result, head_);
        return result;
    }

private:
    static void get_frequencies(std::string &current_word,
                                std::vector<std::pair<std::string, int> > &result,
                                const TrieNode *current_node) {
        if (current_node->counter != 0) {
            result.emplace_back(current_word, current_node->counter);
        }
        for (size_t i = 0; i < current_node->children.size(); ++i) {
            if (!current_node->children[i]) continue;

            current_word.push_back(get_char_from_index(i));
            get_frequencies(current_word, result, current_node->children[i]);
            current_word.pop_back();
        }
    }

    static size_t get_index_from_char(const char ch) {
        if (ch >= 'A' && ch <= 'Z') {
            return ch - ('Z' - 'z') - 'a';
        }
        return ch - 'a';
    }

    static char get_char_from_index(const size_t index) {
        return index + 'a';
    }
};

inline bool is_english_letter(const char ch) {
    return (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z');
}

inline void process_buffer(const char* buffer_pointer, const char* const buffer_end,
                           Trie& trie, std::string& cross_buffer_word) {
    if (!cross_buffer_word.empty()) {
        const char* word_start = buffer_pointer;
        while (buffer_pointer < buffer_end && is_english_letter(*buffer_pointer)) {
            ++buffer_pointer;
        }
        cross_buffer_word.append(word_start, buffer_pointer);

        if (buffer_pointer < buffer_end) {
            trie.add_word(cross_buffer_word);
            cross_buffer_word.clear();
        }
    }

    const char* word_start = nullptr;
    for (const char* p = buffer_pointer; p < buffer_end; ++p) {
        if (is_english_letter(*p)) {
            if (!word_start) {
                word_start = p;
            }
        } else {
            if (word_start) {
                trie.add_word(word_start, p);
                word_start = nullptr;
            }
        }
    }
    if (word_start) {
        cross_buffer_word.assign(word_start, buffer_end);
    }
}


template <size_t BufferSize = 4096>
struct TrieWordCounter : public IWordCounter {
    void count_words(const std::string &input_filename, const std::string &output_filename) override {
        std::ifstream file(input_filename);
        if (!file.is_open()) {
            throw std::runtime_error("Could not open input file: " + input_filename);
        }

        Trie trie;
        std::vector<char> buffer(BufferSize);
        std::string cross_buffer_word;

        while (file.read(buffer.data(), BufferSize) || file.gcount() > 0) {
            process_buffer(buffer.data(), buffer.data() + file.gcount(), trie, cross_buffer_word);
        }

        if (!cross_buffer_word.empty()) {
            trie.add_word(cross_buffer_word);
        }

        file.close();

        std::ofstream out(output_filename);
        if (!out.is_open()) {
            throw std::runtime_error("Could not open output file: " + output_filename);
        }

        std::vector<std::pair<std::string, int> > sorted_counts = trie.get_frequencies();
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
