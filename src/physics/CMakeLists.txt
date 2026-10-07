opensim_add_module(physics SOURCES world.cpp
    HEADERS include/opensim/physics/world.hpp step_time.hpp PUBLIC_DEPS core math scene)
