#include <gtest/gtest.h>
#include <fstream>
#include <memory>
#include <cstdio>
#include <map>
#include <string>
#include <sstream>

#include "impl/0_naive.hpp"
#include "impl/1_buffered.hpp"
#include "impl/2_trie.hpp"
#include "impl/3_mmap.hpp"

template <typename T>
class WordCounterTest : public ::testing::Test {
protected:
    void SetUp() override {
        word_counter_ = std::make_unique<T>();
        temp_input_filename_ = "test_input.tmp";
        temp_output_filename_ = "test_output.tmp";
    }

    void TearDown() override {
        std::remove(temp_input_filename_.c_str());
        std::remove(temp_output_filename_.c_str());
    }

    void WriteInputFile(const std::string& content) const {
        std::ofstream out(temp_input_filename_);
        out << content;
    }

    std::vector<std::pair<int, std::string>> ReadOutputFile() const {
        std::vector<std::pair<int, std::string>> result;
        std::ifstream in(temp_output_filename_);

        int count;
        std::string word;
        while (in >> count >> word) {
            result.emplace_back(count, word);
        }
        return result;
    }

    std::unique_ptr<IWordCounter> word_counter_;
    std::string temp_input_filename_;
    std::string temp_output_filename_;
};

TYPED_TEST_SUITE_P(WordCounterTest);

TYPED_TEST_P(WordCounterTest, CountsSimpleWords) {
    // Given
    this->WriteInputFile("hello world hello");

    // When
    this->word_counter_->count_words(this->temp_input_filename_, this->temp_output_filename_);

    // Then
    auto result = this->ReadOutputFile();

    std::vector<std::pair<int, std::string>> expected = {
        {2, "hello"},
        {1, "world"}
    };

    ASSERT_EQ(result, expected);
}

TYPED_TEST_P(WordCounterTest, HandlesComplexSeparatorsAndPunctuation) {
    // Given
    this->WriteInputFile("word,word!! another-word... word.");

    // When
    this->word_counter_->count_words(this->temp_input_filename_, this->temp_output_filename_);

    // Then
    auto result = this->ReadOutputFile();

    std::vector<std::pair<int, std::string>> expected = {
        {4, "word"},
        {1, "another"}
    };

    ASSERT_EQ(result, expected);
}

TYPED_TEST_P(WordCounterTest, HandlesExample) {
    // Given
    this->WriteInputFile(R""""(The time has come, the Walrus said,
to talk of many things...)"""");

    // When
    this->word_counter_->count_words(this->temp_input_filename_, this->temp_output_filename_);

    // Then
    auto result = this->ReadOutputFile();

    std::vector<std::pair<int, std::string>> expected = {
        {2, "the"},
        {1, "come"},
        {1, "has"},
        {1, "many"},
        {1, "of"},
        {1, "said"},
        {1, "talk"},
        {1, "things"},
        {1, "time"},
        {1, "to"},
        {1, "walrus"},
    };

    ASSERT_EQ(result, expected);
}

TYPED_TEST_P(WordCounterTest, HandlesEmptyInput) {
    // Given
    this->WriteInputFile("");

    // When
    this->word_counter_->count_words(this->temp_input_filename_, this->temp_output_filename_);

    // Then
    auto result = this->ReadOutputFile();
    ASSERT_TRUE(result.empty());
}

TYPED_TEST_P(WordCounterTest, HandlesCrossBufferWords) {
    // Given
    this->WriteInputFile("somel123123anothersomelongword123!!23somel31 23anothersomelongword");

    // When
    this->word_counter_->count_words(this->temp_input_filename_, this->temp_output_filename_);

    // Then
    auto result = this->ReadOutputFile();
    std::vector<std::pair<int, std::string>> expected = {
        {2, "anothersomelongword"},
        {2, "somel"},
    };

    ASSERT_EQ(result, expected);
}

REGISTER_TYPED_TEST_SUITE_P(WordCounterTest,
                           CountsSimpleWords,
                           HandlesComplexSeparatorsAndPunctuation,
                           HandlesExample,
                           HandlesEmptyInput,
                           HandlesCrossBufferWords);

// New implementations should be added here
using Implementations = ::testing::Types<
    naive::NaiveWordCounter,
    buffered::BufferedWordCounter<2>,
    trie::TrieWordCounter<2>,
    mmap_impl::MmapWordCounter
>;
INSTANTIATE_TYPED_TEST_SUITE_P(MyImplementations, WordCounterTest, Implementations);