add_library(robin_map INTERFACE)
# Use tsl::robin_map as target, more consistent with other libraries conventions (Boost, Qt, ...)
add_library(tsl::robin_map ALIAS robin_map)

target_include_directories(word_counter_impl INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})