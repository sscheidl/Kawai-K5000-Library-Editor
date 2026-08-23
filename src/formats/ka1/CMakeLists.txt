add_library(k5000_ka1 STATIC
    Parser.cpp)
add_library(k5000::ka1 ALIAS k5000_ka1)

target_include_directories(k5000_ka1 PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(k5000_ka1 PUBLIC k5000::core)
