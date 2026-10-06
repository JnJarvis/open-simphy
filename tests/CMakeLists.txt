# Module owners register explicit sources in their category/module CMakeLists.
foreach(category unit contract integration regression reference serialization compatibility malformed)
    file(GLOB registrations CONFIGURE_DEPENDS "${category}/*/CMakeLists.txt")
    list(SORT registrations)
    foreach(registration IN LISTS registrations)
        get_filename_component(directory "${registration}" DIRECTORY)
        file(RELATIVE_PATH relative "${CMAKE_CURRENT_SOURCE_DIR}" "${directory}")
        add_subdirectory("${relative}")
    endforeach()
endforeach()

add_subdirectory(support)
