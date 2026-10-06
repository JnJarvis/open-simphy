include_guard(GLOBAL)
include(Catch)

# TEST-001 owns category conventions. This hook creates no module-specific tests.
function(opensim_add_test target)
    cmake_parse_arguments(PARSE_ARGV 1 ARG "" "CATEGORY" "SOURCES;LIBRARIES")
    if(ARG_UNPARSED_ARGUMENTS OR ARG_KEYWORDS_MISSING_VALUES OR NOT ARG_SOURCES)
        message(FATAL_ERROR "Invalid opensim_add_test arguments for ${target}")
    endif()
    set(_categories unit contract integration regression reference serialization compatibility malformed)
    if(NOT ARG_CATEGORY IN_LIST _categories)
        message(FATAL_ERROR "Unknown test category: ${ARG_CATEGORY}")
    endif()
    add_executable("${target}" ${ARG_SOURCES})
    opensim_configure_target("${target}")
    target_link_libraries("${target}" PRIVATE Catch2::Catch2WithMain ${ARG_LIBRARIES})
    catch_discover_tests("${target}" TEST_PREFIX "${target}."
        PROPERTIES LABELS "${ARG_CATEGORY}" TIMEOUT 30)
endfunction()
