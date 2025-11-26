cmake_minimum_required(VERSION 3.16)
project(NeuroBoost VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Options
option(BUILD_VST3 "Build VST3 plugin" ON)
option(BUILD_STANDALONE "Build standalone application" ON)
option(BUILD_AU "Build Audio Unit plugin" OFF)

# Set paths for dependencies
set(IPLUG2_ROOT "${CMAKE_SOURCE_DIR}/deps/iPlug2" CACHE PATH "Path to iPlug2")
set(VISAGE_ROOT "${CMAKE_SOURCE_DIR}/deps/visage" CACHE PATH "Path to Visage")

# Plugin info from config
set(PLUG_NAME "NeuroBoost")
set(PLUG_MFR "Neuroklast")
set(PLUG_VERSION "1.0.0")

# Source files
set(PLUGIN_SOURCES
    src/NeuroBoost.cpp
    src/NeuroBoost.h
    config.h
)

# Check if dependencies exist
if(NOT EXISTS "${IPLUG2_ROOT}")
    message(FATAL_ERROR 
        "iPlug2 not found at ${IPLUG2_ROOT}\n"
        "Please run scripts/setup_deps.sh (Unix) or scripts/setup_deps.bat (Windows) first.")
endif()

if(NOT EXISTS "${VISAGE_ROOT}")
    message(FATAL_ERROR 
        "Visage not found at ${VISAGE_ROOT}\n"
        "Please run scripts/setup_deps.sh (Unix) or scripts/setup_deps.bat (Windows) first.")
endif()

# iPlug2 include directories
set(IPLUG_INCLUDES
    ${IPLUG2_ROOT}
    ${IPLUG2_ROOT}/IPlug
    ${IPLUG2_ROOT}/IPlug/Extras
    ${IPLUG2_ROOT}/WDL
    ${IPLUG2_ROOT}/Dependencies/IPlug/VST3_SDK
)

# Visage include directories
set(VISAGE_INCLUDES
    ${VISAGE_ROOT}
)

# Common include directories
set(COMMON_INCLUDES
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${IPLUG_INCLUDES}
    ${VISAGE_INCLUDES}
)

# Find Visage library
set(VISAGE_LIBRARY "")
if(EXISTS "${VISAGE_ROOT}/build/libvisage.a")
    set(VISAGE_LIBRARY "${VISAGE_ROOT}/build/libvisage.a")
elseif(EXISTS "${VISAGE_ROOT}/build/Release/visage.lib")
    set(VISAGE_LIBRARY "${VISAGE_ROOT}/build/Release/visage.lib")
elseif(EXISTS "${VISAGE_ROOT}/build/Debug/visage.lib")
    set(VISAGE_LIBRARY "${VISAGE_ROOT}/build/Debug/visage.lib")
endif()

if(NOT VISAGE_LIBRARY)
    message(WARNING "Visage library not found. Run scripts/setup_deps.sh to build Visage.")
endif()

# Common compile definitions
set(COMMON_DEFINITIONS
    PLUG_NAME="${PLUG_NAME}"
    PLUG_MFR="${PLUG_MFR}"
    IPLUG_DSP=1
    IPLUG_EDITOR=1
    NO_IGRAPHICS
)

# Platform-specific settings
if(APPLE)
    list(APPEND COMMON_DEFINITIONS OS_MAC)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum macOS version")
    
    # macOS frameworks required for Visage
    find_library(COCOA_FRAMEWORK Cocoa REQUIRED)
    find_library(METAL_FRAMEWORK Metal REQUIRED)
    find_library(METALKIT_FRAMEWORK MetalKit REQUIRED)
    find_library(QUARTZCORE_FRAMEWORK QuartzCore REQUIRED)
    
    set(PLATFORM_LIBS
        ${COCOA_FRAMEWORK}
        ${METAL_FRAMEWORK}
        ${METALKIT_FRAMEWORK}
        ${QUARTZCORE_FRAMEWORK}
    )
elseif(WIN32)
    list(APPEND COMMON_DEFINITIONS OS_WIN NOMINMAX WIN32_LEAN_AND_MEAN)
    
    set(PLATFORM_LIBS
        d3d11
        dxgi
        dxguid
        d3dcompiler
    )
elseif(UNIX)
    list(APPEND COMMON_DEFINITIONS OS_LINUX)
    
    find_package(X11 REQUIRED)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
    
    set(PLATFORM_LIBS
        ${X11_LIBRARIES}
        ${GTK3_LIBRARIES}
        GL
        pthread
    )
endif()

# Create a library for the plugin common code
add_library(NeuroBoost_common OBJECT ${PLUGIN_SOURCES})
target_include_directories(NeuroBoost_common PRIVATE ${COMMON_INCLUDES})
target_compile_definitions(NeuroBoost_common PRIVATE ${COMMON_DEFINITIONS})

message(STATUS "NeuroBoost CMake configuration complete")
message(STATUS "  - VST3 build: ${BUILD_VST3}")
message(STATUS "  - Standalone build: ${BUILD_STANDALONE}")
message(STATUS "  - Audio Unit build: ${BUILD_AU}")
message(STATUS "  - iPlug2 root: ${IPLUG2_ROOT}")
message(STATUS "  - Visage root: ${VISAGE_ROOT}")
if(VISAGE_LIBRARY)
    message(STATUS "  - Visage library: ${VISAGE_LIBRARY}")
endif()

# Note: Full VST3 and Standalone targets require the complete iPlug2 build system.
# For production builds, use the iPlug2 project generator or Visual Studio/Xcode projects.
# This CMake configuration serves as a reference for the project structure.
