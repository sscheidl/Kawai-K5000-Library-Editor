cmake_minimum_required(VERSION 3.24)

project(K5000Librarian
    VERSION 0.1.0
    DESCRIPTION "Kawai K5000S/R librarian, bank builder, format converter and Gotek disk-image manager"
    LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(ProjectOptions)
include(CTest)

# ---------------------------------------------------------------------------
# Options
# ---------------------------------------------------------------------------
option(K5000_BUILD_GUI   "Build the Qt 6 Widgets desktop application" ON)
option(K5000_BUILD_CLI   "Build the k5000cli diagnostic tool"         ON)
option(K5000_BUILD_TESTS "Build the automated test suite"             ON)

# ---------------------------------------------------------------------------
# Subsystems are added as they land (see docs/ARCHITECTURE.md, spec section 40).
# The guard keeps a clean checkout configurable while the tree is still growing.
# ---------------------------------------------------------------------------
function(k5000_add_subdirectory_if_present dir)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${dir}/CMakeLists.txt")
        add_subdirectory("${dir}")
    else()
        message(STATUS "Skipping ${dir} (not implemented yet)")
    endif()
endfunction()

# Core libraries. These must stay free of any Qt GUI dependency.
k5000_add_subdirectory_if_present(src/core)
k5000_add_subdirectory_if_present(src/formats)
k5000_add_subdirectory_if_present(src/disk)
k5000_add_subdirectory_if_present(src/library)

if(K5000_BUILD_CLI)
    k5000_add_subdirectory_if_present(src/cli)
endif()

if(K5000_BUILD_GUI)
    find_package(Qt6 6.5 COMPONENTS Widgets QUIET)
    if(Qt6_FOUND)
        set(CMAKE_AUTOMOC ON)
        set(CMAKE_AUTORCC ON)
        set(CMAKE_AUTOUIC ON)
        k5000_add_subdirectory_if_present(src/gui)
        k5000_add_subdirectory_if_present(src/app)
    else()
        message(WARNING
            "Qt6 Widgets was not found - the desktop application will be skipped.\n"
            "Core libraries, the CLI tool and the tests still build.\n"
            "Point CMAKE_PREFIX_PATH at your Qt 6 installation to enable the GUI, e.g.\n"
            "  -DCMAKE_PREFIX_PATH=C:/Qt/6.8.0/msvc2022_64")
    endif()
endif()

if(K5000_BUILD_TESTS AND BUILD_TESTING)
    k5000_add_subdirectory_if_present(tests)
endif()

message(STATUS "----------------------------------------------------------")
message(STATUS "K5000Librarian ${PROJECT_VERSION}")
message(STATUS "  Compiler : ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  GUI      : ${K5000_BUILD_GUI} (Qt6 found: ${Qt6_FOUND})")
message(STATUS "  CLI      : ${K5000_BUILD_CLI}")
message(STATUS "  Tests    : ${K5000_BUILD_TESTS}")
message(STATUS "----------------------------------------------------------")
