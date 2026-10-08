opensim_add_module(physics SOURCES world.cpp
    HEADERS include/opensim/physics/world.hpp step_time.hpp PUBLIC_DEPS core math scene)

if(OPENSIM_BUILD_COMPAT)
    include(OpenSimMechanics)
    opensim_setup_mechanics()
    target_sources(opensim_physics PRIVATE mechanism.cpp include/opensim/physics/mechanism.hpp)
    target_link_libraries(opensim_physics PRIVATE box2d)
endif()
