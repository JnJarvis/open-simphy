add_library(opensim_test_support INTERFACE)
target_include_directories(opensim_test_support INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(opensim_test_support INTERFACE Catch2::Catch2)
opensim_add_test(opensim_test_unit CATEGORY unit SOURCES numeric_tests.cpp
    LIBRARIES opensim_test_support)
opensim_add_test(opensim_test_contract CATEGORY contract SOURCES contract_sample.cpp
    LIBRARIES opensim_test_support)
add_test(NAME support.numeric_diagnostics COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/verify_diagnostics.py" "$<TARGET_FILE:opensim_test_unit>")
set_tests_properties(support.numeric_diagnostics PROPERTIES LABELS unit TIMEOUT 30)
add_test(NAME support.fixture_metadata COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/fixture_metadata.py")
set_tests_properties(support.fixture_metadata PROPERTIES LABELS contract TIMEOUT 30)
