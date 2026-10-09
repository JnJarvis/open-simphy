opensim_add_test(physics_unit CATEGORY unit SOURCES time.cpp header.cpp LIBRARIES opensim::physics)
# Private test seam is not exported by the physics target.
target_include_directories(physics_unit PRIVATE "${PROJECT_SOURCE_DIR}/src/physics")

if(OPENSIM_BUILD_COMPAT)
    target_sources(physics_unit PRIVATE mechanism.cpp electromagnetic.cpp)
endif()
