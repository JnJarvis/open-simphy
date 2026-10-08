if(TARGET opensim::compat)
    opensim_add_test(ssim_compatibility CATEGORY compatibility SOURCES reader.cpp LIBRARIES opensim::compat miniz)
endif()
