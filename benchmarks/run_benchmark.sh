#!/bin/bash

# Exit immediately if any command fails.
set -e

# --- Configuration Section ---

if [[ "$CI" == "true" ]]; then
  echo "--- CI environment detected. Using full dataset. ---"
  DATA_URL="https://dumps.wikimedia.org/enwikinews/20251001/enwikinews-20251001-pages-articles-multistream.xml.bz2"
else
  echo "--- Local environment detected. Using smaller dataset. ---"
  DATA_URL="https://dumps.wikimedia.org/enwikinews/20251001/enwikinews-20251001-pages-articles-multistream1.xml-p1500001p3000000.bz2"
fi


# --- AUTOMATIC BENCHMARK NAME CONSTRUCTION ---
echo "--- Discovering benchmark target from main.cpp ---"
IMPL_CLASS_NAME=$(grep "using BenchmarkTarget" main.cpp | sed 's/.*= *//;s/;//')
IMPL_BASE_NAME=$(echo "$IMPL_CLASS_NAME" | sed 's/WordCounter//')
BENCHMARK_NAME="$IMPL_BASE_NAME Word Counter Implementation (Time)"
echo "Benchmark target set to: '$BENCHMARK_NAME'"
# --- END CONSTRUCTION ---


COMPRESSED_DATA_FILE="enwiki-multistream.xml.bz2"
UNCOMPRESSED_DATA_FILE="enwiki-multistream.xml"
BUILD_DIR="../build"
BENCH_EXE="$BUILD_DIR/benchmarks/word_counter_bench"
HYPERFINE_RESULTS="benchmark_results.json"
ACTION_RESULTS="benchmark_custom.json"


# --- Step 1: Download and Decompress Data ---
echo "--- Checking for benchmark data ---"
if [ ! -f "$UNCOMPRESSED_DATA_FILE" ]; then
    echo "Benchmark data not found. Downloading and decompressing..."
    WGET_FLAGS=""
    if [[ "$CI" == "true" ]]; then
      WGET_FLAGS="-nv"
    fi
    wget $WGET_FLAGS -O "$COMPRESSED_DATA_FILE" "$DATA_URL"
    echo "Download complete."
    echo "Decompressing data..."
    bunzip2 "$COMPRESSED_DATA_FILE"
    echo "Decompression complete."
else
    echo "Data file ($UNCOMPRESSED_DATA_FILE) already exists. Skipping download."
fi


# --- Step 2: Build the Executable ---
echo -e "\n--- Building the benchmark executable in Release mode ---"
cmake -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release ..
cmake --build "$BUILD_DIR"


# --- Step 3: Run the Benchmark with Hyperfine ---
echo -e "\n--- Running Benchmark using hyperfine ---"
OUTPUT_FILE="output.txt"

hyperfine --warmup 2 \
  --export-json "$HYPERFINE_RESULTS" \
  "'$BENCH_EXE' '$UNCOMPRESSED_DATA_FILE' '$OUTPUT_FILE'"

rm "$OUTPUT_FILE"


# --- Step 4: Convert Hyperfine Output ---
echo -e "\n--- Converting hyperfine output for GitHub Action ---"

jq --arg name "$BENCHMARK_NAME" \
   '[.results[] | {name: $name, unit: "s", value: ((.mean * 100 | round) / 100)}]' \
   "$HYPERFINE_RESULTS" > "$ACTION_RESULTS"

echo "Conversion complete. Final results for CI are in $ACTION_RESULTS"
echo -e "\n--- Benchmark run finished successfully ---"