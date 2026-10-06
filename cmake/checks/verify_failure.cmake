if(NOT EXISTS "${PROBE_EXECUTABLE}")
    message(FATAL_ERROR "Failure probe executable does not exist")
endif()
file(MAKE_DIRECTORY "${PROBE_DIRECTORY}")
file(WRITE "${PROBE_DIRECTORY}/CTestTestfile.cmake"
    "add_test(intentional_failure [==[${PROBE_EXECUTABLE}]==] [==[[.failure-probe]]==])\n")
execute_process(COMMAND "${CTEST_EXECUTABLE}" --test-dir "${PROBE_DIRECTORY}"
    --output-on-failure --no-tests=error
    RESULT_VARIABLE _result OUTPUT_VARIABLE _stdout ERROR_VARIABLE _stderr TIMEOUT 20)
if(NOT _result EQUAL 8 OR NOT _stdout MATCHES "intentional BUILD-002 failure probe"
   OR NOT _stdout MATCHES "0% tests passed, 1 tests failed")
    message(FATAL_ERROR "CTest did not report the expected assertion failure:\n${_stdout}\n${_stderr}")
endif()
message(STATUS "CTest reported the intentional assertion failure correctly")
