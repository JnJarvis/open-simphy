cmake_minimum_required(VERSION 3.28)
project(OpenSimphy VERSION 0.1.0 LANGUAGES CXX)

if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
    message(FATAL_ERROR "Use an out-of-source build, preferably a documented preset.")
endif()

option(OPENSIM_BUILD_APP "Build desktop composition and its presentation/host modules" OFF)
option(OPENSIM_BUILD_TESTS "Build headless test executables and repository checks" ON)
option(OPENSIM_WARNINGS_AS_ERRORS "Treat warnings in project-owned targets as errors" OFF)

message(STATUS "Host: ${CMAKE_HOST_SYSTEM_NAME} ${CMAKE_HOST_SYSTEM_PROCESSOR}")
message(STATUS "Target: ${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "C++: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
if(DEFINED OPENSIM_EXPECT_COMPILER_ID AND
   NOT CMAKE_CXX_COMPILER_ID STREQUAL OPENSIM_EXPECT_COMPILER_ID)
    message(FATAL_ERROR "Selected compiler does not match the validation toolchain.")
endif()

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
include(OpenSimModules)
include(OpenSimSDL)

if(OPENSIM_BUILD_TESTS)
    enable_testing()
    include(OpenSimDependencies)
    opensim_setup_catch2()
    include(OpenSimTests)
    find_package(Python3 3.11 REQUIRED COMPONENTS Interpreter)
endif()

# Explicit topological order. Missing CMakeLists are allowed only for placeholders.
set(_opensim_modules core math scene collision constraints physics visualization
    analysis serialization compat renderer editor)
if(OPENSIM_BUILD_APP)
    list(APPEND _opensim_modules platform app)
endif()
foreach(_module IN LISTS _opensim_modules)
    if(EXISTS "${PROJECT_SOURCE_DIR}/src/${_module}/CMakeLists.txt")
        add_subdirectory("src/${_module}")
    else()
        # This glob only detects unregistered code; it never supplies target sources.
        file(GLOB_RECURSE _unregistered CONFIGURE_DEPENDS
            "${PROJECT_SOURCE_DIR}/src/${_module}/*.cpp"
            "${PROJECT_SOURCE_DIR}/src/${_module}/*.cc"
            "${PROJECT_SOURCE_DIR}/src/${_module}/*.hpp"
            "${PROJECT_SOURCE_DIR}/src/${_module}/*.h")
        if(_unregistered)
            message(FATAL_ERROR "Module ${_module} has code but no CMakeLists.txt registration")
        endif()
    endif()
endforeach()
if(OPENSIM_BUILD_APP AND NOT EXISTS "${PROJECT_SOURCE_DIR}/src/app/CMakeLists.txt")
    message(FATAL_ERROR "The application is not implemented yet. Use OPENSIM_BUILD_APP=OFF.")
endif()

if(OPENSIM_BUILD_TESTS)
    add_subdirectory(cmake/checks)
    # TEST-001 owns this entry point. Its absence must not block the scaffold.
    if(EXISTS "${PROJECT_SOURCE_DIR}/tests/CMakeLists.txt")
        add_subdirectory(tests)
    endif()
endif()
