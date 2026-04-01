cmake_minimum_required(VERSION 3.16)
project(NeuroBoost VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Options
option(BUILD_DSP_TEST "Build DSP test executable (no dependencies required)" ON)
option(BUILD_UI_TEST "Build UI structure test executable (no Visage/iPlug2 required)" ON)
option(BUILD_VST3 "Build VST3 plugin (requires iPlug2 and Visage)" OFF)
option(BUILD_STANDALONE "Build standalone application (requires iPlug2 and Visage)" OFF)
option(BUILD_AU "Build Audio Unit plugin" OFF)

# Set paths for dependencies
set(DEPS_DIR "${CMAKE_SOURCE_DIR}/deps")
set(IPLUG2_ROOT "${DEPS_DIR}/iPlug2" CACHE PATH "Path to iPlug2")
set(VISAGE_ROOT "${DEPS_DIR}/visage" CACHE PATH "Path to Visage")

# Plugin info from config
set(PLUG_NAME "NeuroBoost")
set(PLUG_MFR "Neuroklast")
set(PLUG_VERSION "1.0.0")

# ============================================================================
# DSP Test Target (no external dependencies - always buildable)
# ============================================================================
if(BUILD_DSP_TEST)
    message(STATUS "Configuring sequencer test executable...")

    # Sequencer test executable (no iPlug2 / Visage dependency)
    add_executable(test_sequencer
        ${CMAKE_SOURCE_DIR}/tests/test_sequencer.cpp
        ${CMAKE_SOURCE_DIR}/src/dsp/SequencerEngine.cpp
        ${CMAKE_SOURCE_DIR}/src/dsp/Algorithms.cpp
    )

    target_include_directories(test_sequencer PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/src/common
        ${CMAKE_SOURCE_DIR}/src/dsp
    )

    target_compile_definitions(test_sequencer PRIVATE
        _USE_MATH_DEFINES
    )

    if(UNIX AND NOT APPLE)
        target_link_libraries(test_sequencer PRIVATE m)
    endif()

    set_target_properties(test_sequencer PROPERTIES
        OUTPUT_NAME "test_sequencer"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )

    # Enable testing
    enable_testing()

    # Sequencer tests (authoritative test suite)
    add_test(NAME Sequencer_Tests
             COMMAND test_sequencer)
endif()

# ============================================================================
# UI Structure Test Target (no Visage/iPlug2 required)
# ============================================================================
if(BUILD_UI_TEST)
    message(STATUS "Configuring UI structure test executable...")

    add_executable(test_ui_structure
        ${CMAKE_SOURCE_DIR}/tests/test_ui_structure.cpp
    )

    target_include_directories(test_ui_structure PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/src/common
        ${CMAKE_SOURCE_DIR}/src/dsp
    )

    target_compile_definitions(test_ui_structure PRIVATE
        _USE_MATH_DEFINES
    )

    if(UNIX AND NOT APPLE)
        target_link_libraries(test_ui_structure PRIVATE m)
    endif()

    set_target_properties(test_ui_structure PROPERTIES
        OUTPUT_NAME "test_ui_structure"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )

    # Enable testing if not already enabled
    if(NOT BUILD_DSP_TEST)
        enable_testing()
    endif()

    add_test(NAME UI_Structure_Tests
             COMMAND test_ui_structure
             WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
endif()

# ============================================================================
# Full Plugin Targets (requires iPlug2 and Visage)
# ============================================================================
if(BUILD_VST3 OR BUILD_STANDALONE)
    # Include FetchContent for downloading dependencies
    include(FetchContent)
    
    # Fetch iPlug2 if not present
    if(NOT EXISTS "${IPLUG2_ROOT}")
        message(STATUS "Downloading iPlug2...")
        FetchContent_Declare(
            iPlug2
            GIT_REPOSITORY https://github.com/iPlug2/iPlug2.git
            GIT_TAG 4ca7bc02d1d0bc05e7d1c2c6b0d7c1aa0eca2bef
            GIT_SHALLOW TRUE
            SOURCE_DIR "${IPLUG2_ROOT}"
        )
        FetchContent_MakeAvailable(iPlug2)
    endif()
    
    # Check for Visage
    if(NOT EXISTS "${VISAGE_ROOT}")
        message(STATUS "Visage not found. Please run scripts/setup_deps.sh first to download and build Visage.")
        message(STATUS "Visage requires additional system dependencies:")
        message(STATUS "  - Linux: libx11-dev, libgl1-mesa-dev, libfreetype6-dev, libgtk-3-dev")
        message(STATUS "  - macOS: Xcode Command Line Tools")
        message(STATUS "  - Windows: Visual Studio with C++ support")
    endif()
    
    # Plugin source files
    set(PLUGIN_SOURCES
        ${CMAKE_SOURCE_DIR}/src/NeuroBoost.cpp
        ${CMAKE_SOURCE_DIR}/src/NeuroBoost.h
        ${CMAKE_SOURCE_DIR}/config.h
        # DSP
        ${CMAKE_SOURCE_DIR}/src/dsp/SequencerEngine.cpp
        ${CMAKE_SOURCE_DIR}/src/dsp/Algorithms.cpp
        # UI
        ${CMAKE_SOURCE_DIR}/src/ui/EditorView.cpp
        ${CMAKE_SOURCE_DIR}/src/ui/components/ParameterKnob.cpp
        ${CMAKE_SOURCE_DIR}/src/ui/components/StepGrid.cpp
        ${CMAKE_SOURCE_DIR}/src/ui/components/StepCell.cpp
        ${CMAKE_SOURCE_DIR}/src/ui/components/ModeSelector.cpp
        ${CMAKE_SOURCE_DIR}/src/ui/components/TransportBar.cpp
    )
    
    # iPlug2 core source files
    set(IPLUG_CORE_SOURCES
        ${IPLUG2_ROOT}/IPlug/IPlugAPIBase.cpp
        ${IPLUG2_ROOT}/IPlug/IPlugParameter.cpp
        ${IPLUG2_ROOT}/IPlug/IPlugPaths.cpp
        ${IPLUG2_ROOT}/IPlug/IPlugPluginBase.cpp
        ${IPLUG2_ROOT}/IPlug/IPlugProcessor.cpp
        ${IPLUG2_ROOT}/IPlug/IPlugTimer.cpp
    )
    
    # iPlug2 include directories
    set(IPLUG_INCLUDES
        ${IPLUG2_ROOT}
        ${IPLUG2_ROOT}/IPlug
        ${IPLUG2_ROOT}/IPlug/APP
        ${IPLUG2_ROOT}/IPlug/Extras
        ${IPLUG2_ROOT}/IPlug/VST3
        ${IPLUG2_ROOT}/WDL
        ${IPLUG2_ROOT}/WDL/swell
        ${IPLUG2_ROOT}/Dependencies/IPlug/VST3_SDK
        ${IPLUG2_ROOT}/Dependencies/IPlug/RTAudio
        ${IPLUG2_ROOT}/Dependencies/IPlug/RTAudio/include
        ${IPLUG2_ROOT}/Dependencies/IPlug/RTMidi
    )
    
    # Visage include directories
    set(VISAGE_INCLUDES
        ${VISAGE_ROOT}
        ${VISAGE_ROOT}/visage_app
        ${VISAGE_ROOT}/visage_core
        ${VISAGE_ROOT}/visage_graphics
        ${VISAGE_ROOT}/visage_ui
        ${VISAGE_ROOT}/visage_utils
        ${VISAGE_ROOT}/visage_windowing
        ${VISAGE_ROOT}/third_party
    )
    
    # Common include directories
    set(COMMON_INCLUDES
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/src/common
        ${CMAKE_SOURCE_DIR}/src/dsp
        ${CMAKE_SOURCE_DIR}/src/ui
        ${CMAKE_SOURCE_DIR}/src/ui/components
        ${CMAKE_SOURCE_DIR}/src/ui/theme
        ${IPLUG_INCLUDES}
        ${VISAGE_INCLUDES}
    )
    # resource.h is at project root — iPlug2 Windows builds expect it in the include path.
    # CMAKE_SOURCE_DIR is already listed above, so resource.h will be found automatically.
    
    # Common compile definitions
    set(COMMON_DEFINITIONS
        PLUG_NAME="${PLUG_NAME}"
        PLUG_MFR="${PLUG_MFR}"
        IPLUG_DSP=1
        IPLUG_EDITOR=1
        NO_IGRAPHICS
        SAMPLE_TYPE_FLOAT
        _USE_MATH_DEFINES
    )
    
    # Platform-specific settings
    if(APPLE)
        list(APPEND COMMON_DEFINITIONS SWELL_PROVIDED_BY_APP)
        set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum macOS version")
        
        # macOS frameworks
        find_library(COCOA_FRAMEWORK Cocoa REQUIRED)
        find_library(METAL_FRAMEWORK Metal REQUIRED)
        find_library(METALKIT_FRAMEWORK MetalKit REQUIRED)
        find_library(QUARTZCORE_FRAMEWORK QuartzCore REQUIRED)
        find_library(COREAUDIO_FRAMEWORK CoreAudio REQUIRED)
        find_library(COREMIDI_FRAMEWORK CoreMIDI REQUIRED)
        find_library(AUDIOTOOLBOX_FRAMEWORK AudioToolbox REQUIRED)
        find_library(ACCELERATE_FRAMEWORK Accelerate REQUIRED)
        
        set(PLATFORM_LIBS
            ${COCOA_FRAMEWORK}
            ${METAL_FRAMEWORK}
            ${METALKIT_FRAMEWORK}
            ${QUARTZCORE_FRAMEWORK}
            ${COREAUDIO_FRAMEWORK}
            ${COREMIDI_FRAMEWORK}
            ${AUDIOTOOLBOX_FRAMEWORK}
            ${ACCELERATE_FRAMEWORK}
        )
        
        set(SWELL_SOURCES
            ${IPLUG2_ROOT}/WDL/swell/swell-modstub-generic.cpp
        )
    elseif(WIN32)
        list(APPEND COMMON_DEFINITIONS NOMINMAX WIN32_LEAN_AND_MEAN _CRT_SECURE_NO_WARNINGS)
        
        set(PLATFORM_LIBS
            d3d11
            dxgi
            dxguid
            d3dcompiler
            winmm
            ole32
            uuid
        )
        
        set(SWELL_SOURCES "")
    elseif(UNIX)
        list(APPEND COMMON_DEFINITIONS SWELL_PROVIDED_BY_APP)
        
        find_package(X11 REQUIRED)
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(GTK3 gtk+-3.0)
        pkg_check_modules(ALSA alsa)
        
        set(PLATFORM_LIBS
            ${X11_LIBRARIES}
            GL
            pthread
            dl
        )
        
        if(GTK3_FOUND)
            list(APPEND PLATFORM_LIBS ${GTK3_LIBRARIES})
            list(APPEND COMMON_INCLUDES ${GTK3_INCLUDE_DIRS})
        endif()
        
        if(ALSA_FOUND)
            list(APPEND PLATFORM_LIBS ${ALSA_LIBRARIES})
            list(APPEND COMMON_INCLUDES ${ALSA_INCLUDE_DIRS})
        endif()
        
        set(SWELL_SOURCES
            ${IPLUG2_ROOT}/WDL/swell/swell-modstub-generic.cpp
        )
    endif()
    
    # Find Visage library - check multiple candidate locations
    set(VISAGE_LIBRARY "")
    foreach(_visage_candidate
        "${VISAGE_ROOT}/build/libvisage.a"
        "${VISAGE_ROOT}/build/Release/libvisage.a"
        "${VISAGE_ROOT}/build/Release/visage.lib"
        "${VISAGE_ROOT}/build/Debug/visage.lib"
        "${VISAGE_ROOT}/build/lib/libvisage.a"
        "${VISAGE_ROOT}/build/visage/libvisage.a"
    )
        if(EXISTS "${_visage_candidate}")
            set(VISAGE_LIBRARY "${_visage_candidate}")
            message(STATUS "Found Visage library: ${VISAGE_LIBRARY}")
            break()
        endif()
    endforeach()
    if(NOT VISAGE_LIBRARY AND TARGET visage)
        set(VISAGE_LIBRARY visage)
    endif()
    # Fallback: search recursively under build/
    if(NOT VISAGE_LIBRARY AND EXISTS "${VISAGE_ROOT}/build")
        file(GLOB_RECURSE _visage_search
            "${VISAGE_ROOT}/build/**/libvisage.a"
            "${VISAGE_ROOT}/build/**/visage.lib"
        )
        if(_visage_search)
            list(GET _visage_search 0 VISAGE_LIBRARY)
            message(STATUS "Found Visage library via search: ${VISAGE_LIBRARY}")
        endif()
    endif()
    
    # Build Visage if not already built
    if(NOT VISAGE_LIBRARY AND EXISTS "${VISAGE_ROOT}/CMakeLists.txt")
        message(STATUS "Building Visage library...")
        set(VISAGE_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        set(VISAGE_BUILD_TESTS OFF CACHE BOOL "" FORCE)
        add_subdirectory(${VISAGE_ROOT} ${CMAKE_BINARY_DIR}/visage_build EXCLUDE_FROM_ALL)
        if(TARGET visage)
            set(VISAGE_LIBRARY visage)
        endif()
    endif()
    
    # Create standalone target
    if(BUILD_STANDALONE AND EXISTS "${IPLUG2_ROOT}" AND VISAGE_LIBRARY)
        if(UNIX AND NOT APPLE)
            # iPlug2 APP layer does not support Linux yet (see iPlug2/Scripts/cmake/APP.cmake)
            message(STATUS "Skipping standalone build: iPlug2 APP layer does not support Linux")
        else()
            message(STATUS "Configuring standalone application...")

            set(IPLUG_APP_SOURCES
                ${IPLUG2_ROOT}/IPlug/APP/IPlugAPP.cpp
                ${IPLUG2_ROOT}/IPlug/APP/IPlugAPP_dialog.cpp
                ${IPLUG2_ROOT}/IPlug/APP/IPlugAPP_host.cpp
                ${IPLUG2_ROOT}/IPlug/APP/IPlugAPP_main.cpp
                ${IPLUG2_ROOT}/Dependencies/IPlug/RTAudio/RtAudio.cpp
                ${IPLUG2_ROOT}/Dependencies/IPlug/RTMidi/RtMidi.cpp
            )

            if(APPLE)
                set(SWELL_DIR ${IPLUG2_ROOT}/WDL/swell)
                set(SWELL_STANDALONE_SOURCES
                    ${SWELL_DIR}/swell-appstub.mm
                    ${SWELL_DIR}/swellappmain.mm
                    ${SWELL_DIR}/swell-ini.cpp
                    ${SWELL_DIR}/swell-dlg.mm
                    ${SWELL_DIR}/swell-kb.mm
                    ${SWELL_DIR}/swell-miscdlg.mm
                    ${SWELL_DIR}/swell-menu.mm
                    ${SWELL_DIR}/swell-wnd.mm
                    ${SWELL_DIR}/swell.cpp
                    ${SWELL_DIR}/swell-misc.mm
                    ${SWELL_DIR}/swell-gdi.mm
                )
                set(APP_AUDIO_DEFS __MACOSX_CORE__ SWELL_COMPILED)
                set(APP_EXTRA_LIBS
                    "-framework AppKit"
                    "-framework Carbon"
                    "-framework CoreMIDI"
                    "-framework CoreAudio"
                )
            elseif(WIN32)
                set(SWELL_STANDALONE_SOURCES "")
                list(APPEND IPLUG_APP_SOURCES
                    ${IPLUG2_ROOT}/Dependencies/IPlug/RTAudio/include/asio.cpp
                    ${IPLUG2_ROOT}/Dependencies/IPlug/RTAudio/include/asiodrivers.cpp
                    ${IPLUG2_ROOT}/Dependencies/IPlug/RTAudio/include/asiolist.cpp
                    ${IPLUG2_ROOT}/Dependencies/IPlug/RTAudio/include/iasiothiscallresolver.cpp
                )
                set(APP_AUDIO_DEFS __WINDOWS_DS__ __WINDOWS_MM__ __WINDOWS_ASIO__)
                set(APP_EXTRA_LIBS dsound winmm)
            endif()

            set(STANDALONE_SOURCES
                ${PLUGIN_SOURCES}
                ${IPLUG_CORE_SOURCES}
                ${IPLUG_APP_SOURCES}
                ${SWELL_STANDALONE_SOURCES}
            )

            add_executable(NeuroBoost_Standalone ${STANDALONE_SOURCES})

            target_include_directories(NeuroBoost_Standalone PRIVATE ${COMMON_INCLUDES})
            target_compile_definitions(NeuroBoost_Standalone PRIVATE
                ${COMMON_DEFINITIONS}
                APP_API
                IPLUG_APP
                ${APP_AUDIO_DEFS}
            )

            target_link_libraries(NeuroBoost_Standalone PRIVATE
                ${VISAGE_LIBRARY}
                ${PLATFORM_LIBS}
                ${APP_EXTRA_LIBS}
            )

            set_target_properties(NeuroBoost_Standalone PROPERTIES
                OUTPUT_NAME "NeuroBoost"
                RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
            )
        endif()
    elseif(BUILD_STANDALONE)
        message(WARNING "Cannot build standalone: iPlug2 or Visage not available")
    endif()
    
    # Create VST3 target
    if(BUILD_VST3 AND EXISTS "${IPLUG2_ROOT}")
        if(NOT VISAGE_LIBRARY)
            message(FATAL_ERROR "BUILD_VST3=ON but Visage library not found. "
                "Build Visage first or set VISAGE_LIBRARY manually.")
        endif()
        message(STATUS "Configuring VST3 plugin...")
        
        set(VST3_SDK_ROOT "${IPLUG2_ROOT}/Dependencies/IPlug/VST3_SDK")
        
        if(NOT EXISTS "${VST3_SDK_ROOT}/public.sdk/source/vst/vstsinglecomponenteffect.h")
            message(FATAL_ERROR "VST3 SDK header not found at "
                "${VST3_SDK_ROOT}/public.sdk/source/vst/vstsinglecomponenteffect.h\n"
                "Run: cd ${IPLUG2_ROOT} && git submodule update --init --recursive Dependencies/IPlug/VST3_SDK")
        endif()
        
        set(VST3_INCLUDES
            ${VST3_SDK_ROOT}
            ${VST3_SDK_ROOT}/base/source
            ${VST3_SDK_ROOT}/pluginterfaces
            ${VST3_SDK_ROOT}/public.sdk/source/common
            ${VST3_SDK_ROOT}/public.sdk/source/vst
            ${VST3_SDK_ROOT}/public.sdk/source/vst/hosting
        )
        
        file(GLOB VST3_SDK_SOURCES
            "${VST3_SDK_ROOT}/base/source/*.cpp"
            "${VST3_SDK_ROOT}/public.sdk/source/common/*.cpp"
            "${VST3_SDK_ROOT}/public.sdk/source/vst/*.cpp"
        )
        
        set(IPLUG_VST3_SOURCES
            ${IPLUG2_ROOT}/IPlug/VST3/IPlugVST3.cpp
            ${IPLUG2_ROOT}/IPlug/VST3/IPlugVST3_Controller.cpp
            ${IPLUG2_ROOT}/IPlug/VST3/IPlugVST3_Processor.cpp
            ${IPLUG2_ROOT}/IPlug/VST3/IPlugVST3_ProcessorBase.cpp
        )
        
        set(VST3_SOURCES
            ${PLUGIN_SOURCES}
            ${IPLUG_CORE_SOURCES}
            ${IPLUG_VST3_SOURCES}
            ${VST3_SDK_SOURCES}
            ${SWELL_SOURCES}
        )
        
        add_library(NeuroBoost_VST3 MODULE ${VST3_SOURCES})
        
        target_include_directories(NeuroBoost_VST3 PRIVATE 
            ${COMMON_INCLUDES}
            ${VST3_INCLUDES}
        )
        
        target_compile_definitions(NeuroBoost_VST3 PRIVATE 
            ${COMMON_DEFINITIONS}
            VST3_API
            IPLUG_VST3
        )

        # Define the embedded Lato font path for UI text rendering
        set(_lato_font_path "${VISAGE_ROOT}/visage_graphics/fonts/Lato-Regular.ttf")
        if(EXISTS "${_lato_font_path}")
            target_compile_definitions(NeuroBoost_VST3 PRIVATE
                NEUROBOOST_FONT_PATH="${_lato_font_path}")
            message(STATUS "Font path: ${_lato_font_path}")
        else()
            message(WARNING "Lato-Regular.ttf not found at ${_lato_font_path}. "
                "UI text will not render. Ensure Visage is cloned at VISAGE_ROOT.")
        endif()
        
        # MSVC-specific options
        if(MSVC)
            target_compile_options(NeuroBoost_VST3 PRIVATE
                /wd4068   # suppress unknown pragma warnings (#pragma mark)
                /wd4100   # suppress unreferenced formal parameter
            )
            # Windows SDK headers for ASIO/RTAudio COM support
            target_compile_definitions(NeuroBoost_VST3 PRIVATE
                WIN32_LEAN_AND_MEAN
                NOMINMAX
            )
        endif()
        
        target_link_libraries(NeuroBoost_VST3 PRIVATE ${VISAGE_LIBRARY} ${PLATFORM_LIBS})
        
        set_target_properties(NeuroBoost_VST3 PROPERTIES
            OUTPUT_NAME "NeuroBoost"
            LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/vst3"
            PREFIX ""
        )
        
        if(APPLE)
            set_target_properties(NeuroBoost_VST3 PROPERTIES
                SUFFIX ".vst3"
            )
        elseif(WIN32)
            set_target_properties(NeuroBoost_VST3 PROPERTIES
                SUFFIX ".vst3"
            )
        else()
            set_target_properties(NeuroBoost_VST3 PROPERTIES
                SUFFIX ".so"
            )
        endif()
    elseif(BUILD_VST3)
        message(FATAL_ERROR "Cannot build VST3: iPlug2 not found at ${IPLUG2_ROOT}")
    endif()
endif()

message(STATUS "")
message(STATUS "NeuroBoost CMake configuration complete")
message(STATUS "  - DSP Test (sequencer): ${BUILD_DSP_TEST}")
message(STATUS "  - UI Structure Test: ${BUILD_UI_TEST}")
message(STATUS "  - VST3 build: ${BUILD_VST3}")
message(STATUS "  - Standalone build: ${BUILD_STANDALONE}")
message(STATUS "  - iPlug2 root: ${IPLUG2_ROOT}")
message(STATUS "  - Visage root: ${VISAGE_ROOT}")
