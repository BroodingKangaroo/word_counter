add_executable(run_tests tests.cpp)

target_link_libraries(
        run_tests
        PRIVATE
            GTest::gtest_main
            naive_impl
)

include(GoogleTest)

gtest_discover_tests(run_tests)