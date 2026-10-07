# Compile the private controller directly so default headless CI stays SDL-free.
opensim_add_test(app_integration CATEGORY integration SOURCES session.cpp
    "${PROJECT_SOURCE_DIR}/src/app/session.cpp"
    LIBRARIES opensim::core opensim::math opensim::scene opensim::physics opensim::renderer)
target_include_directories(app_integration PRIVATE "${PROJECT_SOURCE_DIR}/src/app")
