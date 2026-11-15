# SPDX-License-Identifier: AGPL-3.0-or-later
cmake_minimum_required(VERSION 3.24)
project(SpectralDecay VERSION 0.1.0 LANGUAGES C CXX)

include(CTest)
include(FetchContent)
option(SPECTRAL_DECAY_BUILD_PLUGIN "Build the JUCE VST3 and processor tests" ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

FetchContent_Declare(pffft
    URL https://codeload.github.com/marton78/pffft/tar.gz/179a48d486cbee8ec5416e5c163ad575c2efe1f1
    URL_HASH SHA256=4db32d295436cf4a60f40258b4eab2d849cb39adef1c287b49f05729fe2b7481
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_GetProperties(pffft)
if(NOT pffft_POPULATED)
    FetchContent_Populate(pffft)
endif()

# Compile only the single-precision FFT, not upstream benchmarks or other DSP.
add_library(spectral_pffft STATIC
    "${pffft_SOURCE_DIR}/src/pffft.c"
    "${pffft_SOURCE_DIR}/src/pffft_common.c")
target_include_directories(spectral_pffft PUBLIC "${pffft_SOURCE_DIR}/include")
target_include_directories(spectral_pffft PRIVATE "${pffft_SOURCE_DIR}/include/pffft")
target_compile_definitions(spectral_pffft PUBLIC PFFFT_STATIC_DEFINE PRIVATE _USE_MATH_DEFINES)
if(UNIX)
    target_link_libraries(spectral_pffft PUBLIC m)
endif()

add_library(spectral_decay_dsp STATIC src/dsp/PeakBandStft.cpp)
target_include_directories(spectral_decay_dsp PUBLIC src)
target_link_libraries(spectral_decay_dsp PRIVATE spectral_pffft)
target_compile_features(spectral_decay_dsp PUBLIC cxx_std_17)

if(BUILD_TESTING)
    add_executable(spectral_decay_offline_tests tests/OfflineTests.cpp)
    target_link_libraries(spectral_decay_offline_tests PRIVATE spectral_decay_dsp)
    add_test(NAME neutral_offline
        COMMAND spectral_decay_offline_tests --report "${CMAKE_BINARY_DIR}/offline-metrics.csv")
endif()

if(SPECTRAL_DECAY_BUILD_PLUGIN)
    if(WIN32)
        if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8
           OR (MSVC AND NOT MSVC_CXX_ARCHITECTURE_ID STREQUAL "x64"))
            message(FATAL_ERROR "The Windows prototype requires an x86-64 build (-A x64).")
        endif()
    endif()
    FetchContent_Declare(JUCE
        URL https://codeload.github.com/juce-framework/JUCE/tar.gz/d60b7a78030661574a615b586d9325ba7a7fa7dd
        URL_HASH SHA256=8237b2976871982eb9107fd67b576d95bf02355d99540a0127c4e7c001ecd9f5
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
    set(JUCE_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(JUCE_BUILD_EXTRAS OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(JUCE)

    juce_add_plugin(SpectralDecay
        COMPANY_NAME "Spectral Decay"
        BUNDLE_ID "org.spectraldecay.prototype"
        PLUGIN_MANUFACTURER_CODE SpDc
        PLUGIN_CODE SdN1
        FORMATS VST3
        PRODUCT_NAME "Spectral Decay Neutral"
        IS_SYNTH FALSE
        NEEDS_MIDI_INPUT FALSE
        NEEDS_MIDI_OUTPUT FALSE
        IS_MIDI_EFFECT FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE
        COPY_PLUGIN_AFTER_BUILD FALSE
        VST3_CATEGORIES Fx)
    target_sources(SpectralDecay PRIVATE src/plugin/PluginProcessor.cpp src/plugin/PluginEditor.cpp)
    target_include_directories(SpectralDecay PUBLIC src)
    target_compile_definitions(SpectralDecay PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_REPORT_APP_USAGE=0
        JUCE_DISPLAY_SPLASH_SCREEN=0)
    target_link_libraries(SpectralDecay PRIVATE
        spectral_decay_dsp
        juce::juce_audio_processors
        juce::juce_gui_basics
        PUBLIC juce::juce_recommended_config_flags)

    if(BUILD_TESTING)
        add_executable(spectral_decay_processor_tests tests/ProcessorTests.cpp)
        target_link_libraries(spectral_decay_processor_tests PRIVATE
            SpectralDecay juce::juce_audio_processors juce::juce_gui_basics)
        add_test(NAME neutral_processor COMMAND spectral_decay_processor_tests)

        add_executable(spectral_decay_vst3_host_tests tests/Vst3HostTests.cpp)
        target_compile_definitions(spectral_decay_vst3_host_tests PRIVATE
            JUCE_PLUGINHOST_VST3=1 JUCE_USE_CURL=0 JUCE_WEB_BROWSER=0)
        target_link_libraries(spectral_decay_vst3_host_tests PRIVATE
            juce::juce_audio_processors_headless juce::juce_gui_basics juce::juce_core juce::juce_data_structures)
        add_dependencies(spectral_decay_vst3_host_tests SpectralDecay_VST3)
        add_test(NAME neutral_vst3_host COMMAND spectral_decay_vst3_host_tests
            "$<TARGET_FILE_DIR:SpectralDecay_VST3>/../..")
    endif()
endif()
