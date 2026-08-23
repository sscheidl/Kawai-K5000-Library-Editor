# External dependencies.
#
# Policy (specification §1): reuse what is installed, add only what is actually
# required, document everything in docs/THIRD_PARTY_NOTICES.md.

include(FetchContent)

# ---------------------------------------------------------------------------
# Catch2 v3 - test framework (OPEN_QUESTIONS Q6, decided 2026-08-23)
#
# Pinned to a fixed release tag on purpose. Golden tests must not shift
# underneath a release because a dependency moved; changing the pin is a
# deliberate, committed change.
#
# FIND_PACKAGE_ARGS lets an already-installed Catch2 satisfy this without a
# download. K5000_FETCH_CATCH2=OFF disables the download path entirely for
# offline builds, in which case tests are skipped rather than failing configure.
# ---------------------------------------------------------------------------
set(K5000_CATCH2_TAG "v3.7.1" CACHE STRING "Pinned Catch2 release tag")
option(K5000_FETCH_CATCH2 "Download Catch2 if it is not installed locally" ON)

function(k5000_provide_catch2 out_available)
    find_package(Catch2 3 QUIET)
    if(Catch2_FOUND)
        message(STATUS "Catch2: using installed ${Catch2_VERSION}")
        set(${out_available} TRUE PARENT_SCOPE)
        return()
    endif()

    if(NOT K5000_FETCH_CATCH2)
        message(WARNING
            "Catch2 was not found and K5000_FETCH_CATCH2 is OFF - tests will be skipped.")
        set(${out_available} FALSE PARENT_SCOPE)
        return()
    endif()

    message(STATUS "Catch2: fetching ${K5000_CATCH2_TAG}")
    FetchContent_Declare(Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        ${K5000_CATCH2_TAG}
        GIT_SHALLOW    TRUE)
    FetchContent_MakeAvailable(Catch2)

    if(TARGET Catch2::Catch2WithMain)
        list(APPEND CMAKE_MODULE_PATH "${catch2_SOURCE_DIR}/extras")
        set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" PARENT_SCOPE)
        set(${out_available} TRUE PARENT_SCOPE)
    else()
        message(WARNING "Catch2 fetch did not produce Catch2::Catch2WithMain - tests skipped.")
        set(${out_available} FALSE PARENT_SCOPE)
    endif()
endfunction()
