add_executable(opensim_scaffold_checks scaffold.cpp)
opensim_configure_target(opensim_scaffold_checks)
target_link_libraries(opensim_scaffold_checks PRIVATE Catch2::Catch2WithMain)
add_test(NAME build.scaffold COMMAND opensim_scaffold_checks "[scaffold]")
set_tests_properties(build.scaffold PROPERTIES LABELS build TIMEOUT 30)

add_test(NAME build.failure_reporting
    COMMAND "${CMAKE_COMMAND}"
        "-DPROBE_EXECUTABLE=$<TARGET_FILE:opensim_scaffold_checks>"
        "-DPROBE_DIRECTORY=${CMAKE_CURRENT_BINARY_DIR}/failure-probe/$<CONFIG>"
        "-DCTEST_EXECUTABLE=${CMAKE_CTEST_COMMAND}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/verify_failure.cmake")
set_tests_properties(build.failure_reporting PROPERTIES LABELS build TIMEOUT 30)

foreach(_case valid forbidden unknown missing)
    add_test(NAME "build.module_${_case}"
        COMMAND "${CMAKE_COMMAND}"
            "-DCASE=${_case}"
            "-DMODULE_HELPERS=${PROJECT_SOURCE_DIR}/cmake/OpenSimModules.cmake"
            "-DSOURCE_DIRECTORY=${CMAKE_CURRENT_SOURCE_DIR}/module_contract"
            "-DBINARY_DIRECTORY=${CMAKE_CURRENT_BINARY_DIR}/module-${_case}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/verify_module.cmake")
    set_tests_properties("build.module_${_case}" PROPERTIES LABELS build TIMEOUT 30)
endforeach()

# A worker registry is a snapshot. Checks validate it but never grant claims here.
add_test(NAME build.registry
    COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/tools/tasks.py" validate)
add_test(NAME build.task_workflow
    COMMAND "${Python3_EXECUTABLE}" -m unittest discover
        -s "${PROJECT_SOURCE_DIR}/tests" -p test_task_workflow.py -v)
set_tests_properties(build.registry build.task_workflow PROPERTIES LABELS build TIMEOUT 30)
