add_executable(k5000_unit_tests
    test_ka1_parser.cpp
    test_kaa_bank.cpp
    test_verification.cpp
    Fixtures.cpp)

target_include_directories(k5000_unit_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(k5000_unit_tests PRIVATE
    k5000_test_support
    k5000::core
    k5000::ka1
    k5000::kaa)

catch_discover_tests(k5000_unit_tests)
