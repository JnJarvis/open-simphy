include_guard(GLOBAL)
include(FetchContent)
function(opensim_setup_ssim)
    # Official archives, SHA256 verified 2026-10-08; MIT notices packaged by app.
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(INSTALL_PROJECT OFF CACHE BOOL "" FORCE)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(miniz
        URL "https://codeload.github.com/richgel999/miniz/tar.gz/refs/tags/3.1.2"
        URL_HASH SHA256=98468f8924934b723276680f85238b6c78bf1f8b49b4459cc9b7214a20e2e9fb
        TLS_VERIFY TRUE DOWNLOAD_EXTRACT_TIMESTAMP FALSE SYSTEM EXCLUDE_FROM_ALL)
    FetchContent_Declare(pugixml
        URL "https://codeload.github.com/zeux/pugixml/tar.gz/refs/tags/v1.16"
        URL_HASH SHA256=357bcab8877dc9943f355d3a72daba1b053238ba955f50fa81586afb65090219
        TLS_VERIFY TRUE DOWNLOAD_EXTRACT_TIMESTAMP FALSE SYSTEM EXCLUDE_FROM_ALL)
    FetchContent_MakeAvailable(miniz pugixml)
    set(OPENSIM_MINIZ_NOTICE "${miniz_SOURCE_DIR}/LICENSE" PARENT_SCOPE)
    set(OPENSIM_PUGIXML_NOTICE "${pugixml_SOURCE_DIR}/LICENSE.md" PARENT_SCOPE)
endfunction()
