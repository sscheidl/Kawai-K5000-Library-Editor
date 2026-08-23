add_library(k5000_core STATIC
    Diagnostics.cpp)
add_library(k5000::core ALIAS k5000_core)

target_include_directories(k5000_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(k5000_core PUBLIC k5000::project_options)
