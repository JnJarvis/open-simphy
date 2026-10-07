opensim_add_test(renderer_unit CATEGORY unit SOURCES limits.cpp header.cpp LIBRARIES opensim::renderer)
target_include_directories(renderer_unit PRIVATE "${PROJECT_SOURCE_DIR}/src/renderer")
