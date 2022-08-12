cmake_minimum_required(VERSION 3.20)
project(Nyxus)

# ==== Compiler Options
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# CUDA detection and configuration.
find_package(CUDA)

if(CUDA_FOUND)
    try_run(RUN_RESULT_VAR COMPILE_RESULT_VAR
        ${CMAKE_BINARY_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/src/nyx/gpu/find_cuda_gpu.c
        CMAKE_FLAGS
        -DINCLUDE_DIRECTORIES:STRING=${CUDA_TOOLKIT_INCLUDE}
        -DLINK_LIBRARIES:STRING=${CUDA_CUDART_LIBRARY}
        COMPILE_OUTPUT_VARIABLE COMPILE_OUTPUT_VAR
        RUN_OUTPUT_VARIABLE RUN_OUTPUT_VAR)
    message("---------${CUDA_TOOLKIT_INCLUDE}--------")
    message("${RUN_OUTPUT_VAR}") # Display number of GPUs found

    # COMPILE_RESULT_VAR is TRUE when compile succeeds
    # RUN_RESULT_VAR is zero when a GPU is found
    if(COMPILE_RESULT_VAR AND NOT RUN_RESULT_VAR)
        option(USEGPU "Use GPU" ON)
    else()
        option(USEGPU "Use GPU" OFF)
    endif()
else()
    option(USEGPU "Use GPU" OFF)
endif(CUDA_FOUND)

if(USEGPU)
    set(CMAKE_CUDA_STANDARD 14)
    set(CMAKE_CUDA_STANDARD_REQUIRED True)

    set(CMAKE_CUDA_HOST_COMPILER ${CMAKE_CXX_COMPILER})
    set(CMAKE_CUDA_HOST_LINK_LAUNCHER ${CMAKE_CXX_COMPILER})
    enable_language("CUDA")
    add_definitions(-DUSE_GPU)

    include(FindCUDA/select_compute_arch)
    CUDA_DETECT_INSTALLED_GPUS(INSTALLED_GPU_CCS_1)
    string(STRIP "${INSTALLED_GPU_CCS_1}" INSTALLED_GPU_CCS_2)
    string(REPLACE " " ";" INSTALLED_GPU_CCS_3 "${INSTALLED_GPU_CCS_2}")
    string(REPLACE "." "" CUDA_ARCH_LIST "${INSTALLED_GPU_CCS_3}")
    SET(CMAKE_CUDA_ARCHITECTURES ${CUDA_ARCH_LIST})
    set_property(GLOBAL PROPERTY CUDA_ARCHITECTURES "${CUDA_ARCH_LIST}")
    message("Found GPU with compute capability ${CUDA_ARCH_LIST}.")
endif()

# Default to release build.
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Libtiff needed.
find_package(TIFF 4.3.0 REQUIRED)
include_directories(${TIFF_INCLUDE_DIRS})
link_libraries(${TIFF_LIBRARIES})

# PyBind11 needed.
find_package(Python 3.6 COMPONENTS Interpreter Development REQUIRED)
find_package(pybind11 CONFIG REQUIRED)

# OpenMP for multithreading.
find_package(OpenMP COMPONENTS C CXX REQUIRED)

# Include our own deps.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake-modules")

# Fastloader for tiled TIFF.
find_package(FastLoader REQUIRED)
list(APPEND Nyxus_LIBRARIES ${FastLoader_LIBRARIES})
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${FastLoader_CXX_FLAGS}")
include_directories(${FastLoader_INCLUDE_DIR})

# For OME Tiff
find_package(Boost REQUIRED)

# ==== Source files
set(SOURCE
    src/nyx/features/basic_morphology.cpp
    src/nyx/features/caliper_feret.cpp
    src/nyx/features/caliper_martin.cpp
    src/nyx/features/caliper_nassenstein.cpp
    src/nyx/features/chords.cpp
    src/nyx/features/chords_nontriv.cpp
    src/nyx/features/circle.cpp
    src/nyx/features/contour.cpp
    src/nyx/features/convex_hull_nontriv.cpp
    src/nyx/features/ellipse_fitting.cpp
    src/nyx/features/erosion_pixels.cpp
    src/nyx/features/euler_number.cpp
    src/nyx/features/extrema.cpp
    src/nyx/features/fractal_dim.cpp
    src/nyx/features/gabor.cpp
    src/nyx/features/gabor_nontriv.cpp
    src/nyx/features/geo_len_thickness.cpp
    src/nyx/features/glcm.cpp
    src/nyx/features/glcm_nontriv.cpp
    src/nyx/features/gldm.cpp
    src/nyx/features/glrlm.cpp
    src/nyx/features/glszm.cpp
    src/nyx/features/hexagonality_polygonality.cpp
    src/nyx/features/image_matrix.cpp
    src/nyx/features/image_matrix_nontriv.cpp
    src/nyx/features/image_moments.cpp
    src/nyx/features/image_moments_nontriv.cpp
    src/nyx/features/intensity.cpp
    src/nyx/features/neighbors.cpp
    src/nyx/features/ngtdm.cpp
    src/nyx/features/radial_distribution.cpp
    src/nyx/features/roi_label.cpp
    src/nyx/features/roi_radius.cpp
    src/nyx/features/rotation.cpp
    src/nyx/features/specfunc.cpp
    src/nyx/features/zernike.cpp
    src/nyx/features/zernike_nontriv.cpp
    src/nyx/helpers/timing.cpp
    src/nyx/common_stats.cpp
    src/nyx/dirs_and_files.cpp
    src/nyx/environment.cpp
    src/nyx/environment_basic.cpp
    src/nyx/feature_method.cpp
    src/nyx/feature_mgr.cpp
    src/nyx/feature_mgr_init.cpp
    src/nyx/features_calc_workflow.cpp
    src/nyx/featureset.cpp
    src/nyx/globals.cpp
    src/nyx/image_loader.cpp
    src/nyx/output_2_buffer.cpp
    src/nyx/output_2_csv.cpp
    src/nyx/parallel.cpp
    src/nyx/phase1.cpp
    src/nyx/phase2.cpp
    src/nyx/phase3.cpp
    src/nyx/pixel_feed.cpp
    src/nyx/reduce_by_feature.cpp
    src/nyx/reduce_trivial_rois.cpp
    src/nyx/roi_cache.cpp
    src/nyx/roi_cache_basic.cpp
    src/nyx/scan_fastloader_way.cpp
)

option(BUILD_CLI "Build Nyxus CLI" ON)

if(BUILD_CLI)
    add_executable(nyxus ${SOURCE} src/nyx/main_nyxus.cpp)
    add_executable(nyxushie
        src/nyx/dirs_and_files.cpp
        src/nyx/environment_basic.cpp
        src/nyx/image_loader1x.cpp
        src/nyx/main_nyxushie.cpp
        src/nyx/nested_roi.cpp
        src/nyx/python/nested_roi_py.cpp
        src/nyx/roi_cache_basic.cpp
    )
endif()
