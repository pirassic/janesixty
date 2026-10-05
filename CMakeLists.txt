cmake_minimum_required(VERSION 3.25)

# Universal binary on macOS; must be set before project().
if(APPLE AND NOT CMAKE_OSX_ARCHITECTURES)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
endif()
if(APPLE AND NOT CMAKE_OSX_DEPLOYMENT_TARGET)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "" FORCE)
endif()

project(Jane60
    VERSION 0.0.1
    DESCRIPTION "Jane-Sixty: a component-modelled Roland Juno-60 synthesizer plugin"
    LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(JANE60_BUILD_PLUGIN "Build the JUCE plugin and standalone targets" ON)
option(JANE60_BUILD_TESTS  "Build unit tests" ON)

include(FetchContent)

# ---------------------------------------------------------------------------
# Third-party: nlohmann/json (MIT), used by the calibration loader.
# ---------------------------------------------------------------------------
FetchContent_Declare(nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
    GIT_SHALLOW    TRUE)
set(JSON_BuildTests OFF CACHE INTERNAL "")
FetchContent_MakeAvailable(nlohmann_json)

# ---------------------------------------------------------------------------
# jane60_dsp: framework-free DSP and calibration library.
# ---------------------------------------------------------------------------
add_library(jane60_dsp STATIC
    src/dsp/Calibration.cpp
    src/dsp/Synth.cpp
    src/dsp/presets/FactoryPatches.cpp
    src/dsp/presets/PresetFormat.cpp)
target_include_directories(jane60_dsp PUBLIC src)
target_link_libraries(jane60_dsp PUBLIC nlohmann_json::nlohmann_json)
target_compile_features(jane60_dsp PUBLIC cxx_std_20)
if(MSVC)
    target_compile_options(jane60_dsp PRIVATE /W4)
else()
    target_compile_options(jane60_dsp PRIVATE -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion)
endif()

# ---------------------------------------------------------------------------
# Plugin (JUCE 9 under AGPLv3; project is GPLv3).
# ---------------------------------------------------------------------------
if(JANE60_BUILD_PLUGIN)
    set(JUCE_BUILD_EXTRAS OFF CACHE BOOL "" FORCE)
    set(JUCE_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    add_subdirectory(libs/JUCE)

    set(JANE60_FORMATS Standalone VST3)
    if(APPLE)
        list(APPEND JANE60_FORMATS AU)
    endif()

    juce_add_plugin(Jane60
        PRODUCT_NAME                "Jane-Sixty"
        COMPANY_NAME                "clevergear"
        BUNDLE_ID                   "com.clevergear.jane60"
        PLUGIN_MANUFACTURER_CODE    Clvg
        PLUGIN_CODE                 Jn60
        FORMATS                     ${JANE60_FORMATS}
        IS_SYNTH                    TRUE
        NEEDS_MIDI_INPUT            TRUE
        NEEDS_MIDI_OUTPUT           FALSE
        IS_MIDI_EFFECT              FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS TRUE
        COPY_PLUGIN_AFTER_BUILD     FALSE
        VST3_CATEGORIES             Instrument Synth
        AU_MAIN_TYPE                kAudioUnitType_MusicDevice
        HARDENED_RUNTIME_ENABLED    TRUE
        MICROPHONE_PERMISSION_ENABLED FALSE)

    juce_generate_juce_header(Jane60)

    target_sources(Jane60 PRIVATE
        src/plugin/PluginProcessor.cpp
        src/plugin/Parameters.cpp
        src/plugin/PresetManager.cpp
        src/plugin/PluginEditor.cpp
        src/ui/Controls.cpp
        src/ui/PanelComponent.cpp)

    juce_add_binary_data(Jane60Data
        SOURCES calibration/juno60.json calibration/factory_patches.csv)

    target_compile_definitions(Jane60 PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_REPORT_APP_USAGE=0)

    target_link_libraries(Jane60 PRIVATE
        jane60_dsp
        Jane60Data
        juce::juce_audio_utils
        juce::juce_dsp
        PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
endif()

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------
if(JANE60_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
