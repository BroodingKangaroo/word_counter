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
Boosts the overall program performance by approximately 2x compared to the naive streaming version, while still maintaining low memory usage.