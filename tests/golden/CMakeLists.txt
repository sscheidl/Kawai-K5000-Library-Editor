add_executable(k5000_golden_tests
    test_corpus.cpp
    ../unit/Fixtures.cpp)

target_include_directories(k5000_golden_tests PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../unit")
target_link_libraries(k5000_golden_tests PRIVATE
    k5000_test_support
    k5000::core
    k5000::ka1
    k5000::kaa)

catch_discover_tests(k5000_golden_tests)
