include_guard(GLOBAL)
include(FetchContent)

function(opensim_setup_catch2)
    set(OPENSIM_CATCH2_SOURCE_DIR "" CACHE PATH "Explicit offline source for Catch2 3.7.1")
    if(OPENSIM_CATCH2_SOURCE_DIR)
        cmake_path(ABSOLUTE_PATH OPENSIM_CATCH2_SOURCE_DIR NORMALIZE OUTPUT_VARIABLE _source)
        set(_version_header "${_source}/src/catch2/catch_version_macros.hpp")
        if(NOT EXISTS "${_version_header}")
            message(FATAL_ERROR "Catch2 offline source lacks catch_version_macros.hpp")
        endif()
        file(READ "${_version_header}" _version)
        foreach(_pair MAJOR=3 MINOR=7 PATCH=1)
            string(REPLACE "=" ";" _parts "${_pair}")
            list(GET _parts 0 _part)
            list(GET _parts 1 _value)
            if(NOT _version MATCHES "#define CATCH_VERSION_${_part} ${_value}([\r\n]|$)")
                message(FATAL_ERROR "Offline source must be Catch2 3.7.1 (${_part} mismatch)")
            endif()
        endforeach()
        message(STATUS "Catch2 explicit offline override: ${_source}; caller supplies provenance")
        FetchContent_Declare(Catch2 SOURCE_DIR "${_source}" SYSTEM EXCLUDE_FROM_ALL)
    else()
        # Official v3.7.1 archive, SHA-256 calculated 2026-10-06.
        FetchContent_Declare(Catch2
            URL "https://codeload.github.com/catchorg/Catch2/tar.gz/refs/tags/v3.7.1"
            URL_HASH SHA256=c991b247a1a0d7bb9c39aa35faf0fe9e19764213f28ffba3109388e62ee0269c
            TLS_VERIFY TRUE
            DOWNLOAD_EXTRACT_TIMESTAMP FALSE
            SYSTEM EXCLUDE_FROM_ALL)
    endif()
    FetchContent_MakeAvailable(Catch2)
    list(APPEND CMAKE_MODULE_PATH "${catch2_SOURCE_DIR}/extras")
    set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" PARENT_SCOPE)
endfunction()
