opensim_add_test(renderer_contract CATEGORY contract SOURCES pixels.cpp lifecycle.cpp LIBRARIES opensim::renderer)
target_include_directories(renderer_contract PRIVATE "${PROJECT_SOURCE_DIR}/src/renderer")
