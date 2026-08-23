add_executable(k5000cli main.cpp)
target_link_libraries(k5000cli PRIVATE k5000::core k5000::ka1 k5000::kaa)
set_target_properties(k5000cli PROPERTIES OUTPUT_NAME k5000cli)
