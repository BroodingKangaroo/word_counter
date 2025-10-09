add_executable(word_counter_bench main.cpp)

target_link_libraries(word_counter_bench PRIVATE
        naive_impl
)