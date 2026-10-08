# This language-free DAG fixture needs no Visual Studio/SDK discovery.
# Use the project-required Ninja generator; isolate its cache from old probes.
execute_process(COMMAND "${CMAKE_COMMAND}" -G Ninja -S "${SOURCE_DIRECTORY}" -B "${BINARY_DIRECTORY}/ninja"
    "-DCASE=${CASE}" "-DMODULE_HELPERS=${MODULE_HELPERS}"
    RESULT_VARIABLE _result OUTPUT_VARIABLE _stdout ERROR_VARIABLE _stderr TIMEOUT 20)
if(CASE STREQUAL "valid")
    if(NOT _result EQUAL 0)
        message(FATAL_ERROR "Valid module dependency rejected (${_result}): ${_stdout}\n${_stderr}")
    endif()
else()
    set(_forbidden "Forbidden internal dependency: core -> math")
    set(_unknown "Unknown module: unknown")
    set(_missing "Dependency is not registered: scene -> core")
    if(_result EQUAL 0 OR NOT _stderr MATCHES "${_${CASE}}")
        message(FATAL_ERROR "Expected module rejection missing (${_result}): ${_stdout}\n${_stderr}")
    endif()
endif()
