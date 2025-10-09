#pragma once

#include <string>

struct IWordCounter {
    virtual ~IWordCounter() = default;
    virtual void count_words(const std::string& input_filename, const std::string& output_filename) = 0;
};