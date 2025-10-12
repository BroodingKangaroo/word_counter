### Word counter

Interactive performance graph of every push to main
https://broodingkangaroo.github.io/word_counter/

### 1. Naive Implementation

This is a baseline implementation that processes the input file character-by-character.
It utilises `std::unordered_map` to count and store words frequencies.

- An earlier version of this implementation read the entire file into a `std::string` before processing. This simplified parsing logic, but its high memory consumption made it completely unsuitable for large files.
- Pre-allocating memory with `std::string::reserve()` and `std::unordered_map::reserve()` showed no significant performance gains.
- Lowering the map's `max_load_factor` to `0.3` (chosen empirically) provided a minor speedup by reducing hash collisions at the cost of higher memory usage.

### 2. Buffered Implementation

Improves I/O performance. It reads the input file in fixed-size chunks (4KB) into a buffer (`std::vector`) and then processes it element-by-element.
This way it reduces the number of expensive system calls and improves cache locality.
Boosts the overall program performance by approximately **2x** compared to the naive streaming version, while still maintaining low memory usage.

### 3. Trie-Based Implementation

This version replaces the `std::unordered_map` with [Trie](https://en.wikipedia.org/wiki/Trie) data structure.
This approach improves performance on Mac (M4) by approximately **70%** over the Buffered implementation. However, the result doesn't persist across different platforms
with Ubuntu showing even a slightly worse performance then the Buffered implementation.

- The initial implementation relied on dynamic allocations of each individual TrieNode which killed the performance. To counter it the segmented memory pool was introduced (`std::deque<std::vector<TrieNode>>`). Allocating a giant vector upfront doesn't work because of the potential reallocation and reference invalidation.
- The expensive character-by-character `std::string` creation (`current_word += ...`) was eliminated. Instead, words are identified by raw pointers (`buffer_pointer`, `buffer_end`) directly within the I/O buffer. This avoids unnecessary heap reallocations. However, this increases the complexity since now cross-buffer words need be correctly processed.
- Potentially expensive calls to `std::isalpha` and `std::tolower` were substitutes by simple arithmetic functions.