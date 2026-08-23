add_library(k5000_kaa STATIC
    Bank.cpp)
add_library(k5000::kaa ALIAS k5000_kaa)

target_include_directories(k5000_kaa PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(k5000_kaa PUBLIC k5000::core k5000::ka1)
