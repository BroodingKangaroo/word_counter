# Word counter

This project explores various C++ implementations for a word frequency counter, progressively optimizing the solution from a naive approach to a parallel implementation.

All implementations can be found in the `src/impl` directory, numbered in order of increasing complexity.

An interactive performance graph for every push to `main` is available here: https://broodingkangaroo.github.io/word_counter/

## Building and Running

### Dependencies

The project relies on a few external libraries. Please ensure they are installed before building the project.

```bash
# linux
sudo apt-get install libboost-all-dev

# macos (using Homebrew)
brew install boost
# additionally, if running on MacOS oneDPL is required
brew install onedpl
```

### Building the Project

The project uses CMake. To build the optimized release version, run the following commands from the project's root directory:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### Running the Benchmark
A simple benchmark script is provided to test the performance on a small sample file. To run it, navigate to the benchmarks directory and execute the script:
```bash
cd benchmarks
./run_benchmark.sh
```

Benchmark additionally requires `wget`, `hyperfine` and `jq`.

## Implementations

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

This version replaces the `std::unordered_map` with a [Trie](https://en.wikipedia.org/wiki/Trie) data structure.

- The initial implementation relied on dynamic allocations of each individual TrieNode which killed the performance. To counter it the segmented memory pool was introduced (`std::deque<std::vector<TrieNode>>`). Allocating a giant vector upfront doesn't work because of the potential reallocation and reference invalidation.
- The expensive character-by-character `std::string` creation (`current_word += ...`) was eliminated. Instead, words are identified by raw pointers (`buffer_pointer`, `buffer_end`) directly within the I/O buffer. This avoids unnecessary heap reallocations. However, this increases the complexity since now cross-buffer words need be correctly processed.
- Potentially expensive calls to `std::isalpha` and `std::tolower` were substitutes by simple arithmetic functions.

### 4. Memory-Mapped I/O Implementation

This version replaces the buffered approach with memory-mapped I/O using `boost::iostreams::mapped_file`.
`madvise` system call was also added to give advice to
the kernel about the address range beginning and its size. `MADV_SEQUENTIAL` was used
so that kernel expects page references in sequential order. (Hence, pages
in the given range can be aggressively read ahead, and may
be freed soon after they are accessed.)

### 5. Robin Map Implementation

This version replaces `std::unordered_map` with the more cache-friendly [tsl::robin_map](https://github.com/Tessil/robin-map/tree/master).

- `std::unordered_map` typically uses separate chaining, which stores elements in linked lists. Accessing these elements can lead to pointer chasing and frequent cache misses. `tsl::robin_map` uses open addressing, storing all elements in a single, contiguous array. This improves cache locality, as iterating through buckets or resolving collisions often means accessing memory that is already in the CPU cache.
- A `stack_buffer` optimisation helps counter the repeated heap allocation and copying of `std::string` keys for every word. Short words are converted to lowercase in a small, stack-allocated buffer, and a `std::string_view` of this buffer is used for map lookups. This completely avoids heap allocations for the majority of words.
- Experiments with faster hashing algorithms like `xxh3` showed no performance gains.

### 6. Parallel Implementation

This version introduces multi-threading to use all available CPU cores.

-   The memory-mapped file is divided into N chunks, where N is the number of available hardware cores. `std::async` is used to launch N concurrent tasks (in separate threads), with each task being responsible for counting the words in its assigned chunk. The results are aggregated into a final map after the completion of all threads.
-   The final sorting of unique words is also parallelized. On macOS it falls back to Intel's `oneDPL` since macOS's stl does not support parallel algorithms.
-   To reduce the number of expensive system calls the output is written to the final file in large chunks.
-   Experiments with dynamic load distribution showed no performance gains.
-   Some threads are still doing more work than others, which results in potential stalls while merging maps. Producer-consumer patter with a thread-safe queue can be a solution.