add_library(word_counter_interface INTERFACE)

target_include_directories(word_counter_interface
        INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
)

add_subdirectory(impl)