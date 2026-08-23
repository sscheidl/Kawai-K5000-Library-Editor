# Shared compile settings for every target in this project.
# Kept in one place so binary-format code is compiled with identical assumptions.

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
endif()

add_library(k5000_project_options INTERFACE)
add_library(k5000::project_options ALIAS k5000_project_options)

target_compile_features(k5000_project_options INTERFACE cxx_std_20)

if(MSVC)
    target_compile_options(k5000_project_options INTERFACE
        /W4 /permissive- /utf-8 /Zc:__cplusplus /EHsc /MP)
    target_compile_definitions(k5000_project_options INTERFACE
        NOMINMAX WIN32_LEAN_AND_MEAN UNICODE _UNICODE)
else()
    target_compile_options(k5000_project_options INTERFACE
        -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
endif()

add_library(k5000_warnings_as_errors INTERFACE)
add_library(k5000::warnings_as_errors ALIAS k5000_warnings_as_errors)
if(MSVC)
    target_compile_options(k5000_warnings_as_errors INTERFACE /WX)
else()
    target_compile_options(k5000_warnings_as_errors INTERFACE -Werror)
endif()
