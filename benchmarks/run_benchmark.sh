#!/bin/bash

# Exit immediately if any command fails.
set -e

# --- Configuration Section ---

# This is a more stable URL for a recent Wikipedia dump. It is compressed.
#DATA_URL="https://dumps.wikimedia.org/enwikinews/20251001/enwikinews-20251001-pages-articles-multistream.xml.bz2"
DATA_URL="https://dumps.wikimedia.your.org/huwiki/20220520/huwiki-20220520-pages-meta-current.xml.bz2"
# For local debug
#DATA_URL="https://dumps.wikimedia.org/enwikinews/20251001/enwikinews-20251001-pages-articles-multistream1.xml-p1500001p3000000.bz2"

# The name of the compressed file we will download.
COMPRESSED_DATA_FILE="enwiki-multistream.xml.bz2"
# The final, uncompressed data file that the benchmark will run on.
UNCOMPRESSED_DATA_FILE="enwiki-multistream.xml"

BUILD_DIR="../build"
BENCH_EXE="$BUILD_DIR/benchmarks/word_counter_bench"
HYPERFINE_RESULTS="benchmark_results.json"
ACTION_RESULTS="benchmark_custom.json"


# --- Step 1: Download and Decompress Data ---
echo "--- Checking for benchmark data ---"
# We check for the final, uncompressed file.
if [ ! -f "$UNCOMPRESSED_DATA_FILE" ]; then
    echo "Benchmark data not found. Downloading and decompressing..."

    # Download the compressed file.
    wget -O "$COMPRESSED_DATA_FILE" "$DATA_URL"
    echo "Download complete."

    # Decompress the file. The 'bunzip2' command removes the .bz2 file by default.
    echo "Decompressing data..."
    bunzip2 "$COMPRESSED_DATA_FILE"
    echo "Decompression complete."
else
    echo "Data file ($UNCOMPRESSED_DATA_FILE) already exists. Skipping download."
fi


# --- Step 2: Build the Executable (No changes here) ---
echo -e "\n--- Building the benchmark executable in Release mode ---"
cmake -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release ..
cmake --build "$BUILD_DIR"


# --- Step 3: Run the Benchmark with Hyperfine (Now points to the correct file) ---
echo -e "\n--- Running Benchmark using hyperfine ---"
OUTPUT_FILE="output.txt"

# The command now uses the uncompressed data file as input.
hyperfine --warmup 5 \
  --export-json "$HYPERFINE_RESULTS" \
  "'$BENCH_EXE' '$UNCOMPRESSED_DATA_FILE' '$OUTPUT_FILE'"

rm "$OUTPUT_FILE"


# --- Step 4: Convert Hyperfine Output (No changes here) ---
echo -e "\n--- Converting hyperfine output for GitHub Action ---"
jq '[.results[] | {name: .command, unit: "s", value: .mean}]' "$HYPERFINE_RESULTS" > "$ACTION_RESULTS"

echo "Conversion complete. Final results for CI are in $ACTION_RESULTS"
echo -e "\n--- Benchmark run finished successfully ---"