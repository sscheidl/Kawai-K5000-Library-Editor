# Test suite (OPEN_QUESTIONS Q6: Catch2 v3, pinned).
#
# Subdirectories are added as they gain content, so a clean checkout with no
# tests yet still configures and runs `ctest` successfully.

k5000_provide_catch2(K5000_CATCH2_AVAILABLE)

if(NOT K5000_CATCH2_AVAILABLE)
    message(STATUS "Tests: skipped (Catch2 unavailable)")
    return()
endif()

include(Catch)

# Golden tests need the private reference corpus (OPEN_QUESTIONS Q2). It is
# read-only and never committed. Tests that require it skip with an explanatory
# message when it is absent, so a clean public checkout passes.
add_library(k5000_test_support INTERFACE)
target_link_libraries(k5000_test_support INTERFACE
    Catch2::Catch2WithMain
    k5000::project_options)
target_compile_definitions(k5000_test_support INTERFACE
    K5000_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../testdata/fixtures"
    K5000_REFERENCE_CORPUS_DIR="${K5000_REFERENCE_CORPUS}")

foreach(dir unit integration golden)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${dir}/CMakeLists.txt")
        add_subdirectory("${dir}")
    endif()
endforeach()
