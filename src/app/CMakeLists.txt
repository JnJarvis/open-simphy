opensim_add_module(app SOURCES session.cpp HEADERS session.hpp PUBLIC_DEPS core math scene physics renderer editor)
opensim_setup_sdl()
add_executable(opensim_demo main.cpp sdl_host.cpp sdl_host.hpp editor_ui.cpp editor_ui.hpp)
opensim_configure_target(opensim_demo)
target_link_libraries(opensim_demo PRIVATE opensim::app SDL3::SDL3-static opensim::compat)
configure_file("${OPENSIM_SDL_LICENSE_FILE}" "${CMAKE_CURRENT_BINARY_DIR}/SDL-LICENSE.txt" COPYONLY)

configure_file("${OPENSIM_MINIZ_NOTICE}" "${CMAKE_CURRENT_BINARY_DIR}/MINIZ-LICENSE.txt" COPYONLY)
configure_file("${OPENSIM_PUGIXML_NOTICE}" "${CMAKE_CURRENT_BINARY_DIR}/PUGIXML-LICENSE.txt" COPYONLY)
