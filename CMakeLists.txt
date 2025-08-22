cmake_minimum_required(VERSION 3.15)

project(GrooveGlide VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# CPM Package Manager for JUCE
include(FetchContent)
FetchContent_Declare(
  cpm
  GIT_REPOSITORY https://github.com/cpm-cmake/CPM.cmake.git
  GIT_TAG v0.38.3
)
FetchContent_MakeAvailable(cpm)
include(${cpm_SOURCE_DIR}/cmake/CPM.cmake)

# Download and configure JUCE
CPMAddPackage(
    NAME juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG origin/master
)

# JUCE JavaScript files will be copied during build if available
# The placeholder file will be used until real JUCE files are available

# Plugin configuration
juce_add_plugin(${PROJECT_NAME}
    COMPANY_NAME "Audio Innovators"
    BUNDLE_ID "com.audioinnovators.grooveglide"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE AUDI
    PLUGIN_CODE GrGl
    NEEDS_WEBVIEW2 TRUE
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "Groove Glide"
)

# Source files
juce_generate_juce_header(${PROJECT_NAME})

target_sources(${PROJECT_NAME}
    PRIVATE
        source/PluginEditor.cpp
        source/PluginProcessor.cpp
)

# Collect web UI files for binary data
file(GLOB_RECURSE RESOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/source/ui/public/*")

# Create binary data from web assets
juce_add_binary_data(BinaryData
    SOURCES
    ${RESOURCE_FILES}
)

# Link libraries
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        BinaryData
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_dsp
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# WebView2 support and other compile definitions
target_compile_definitions(${PROJECT_NAME}
    PUBLIC
        JUCE_WEB_BROWSER=1
        JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_REPORT_APP_USAGE=0
        JUCE_STRICT_REFCOUNTEDPOINTER=1
        JUCE_VST3_CAN_REPLACE_VST2=0
)

# Copy JUCE JavaScript files after JUCE is downloaded
add_custom_command(
    TARGET BinaryData PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_BINARY_DIR}/_deps/juce-src/modules/juce_gui_extra/native/javascript/"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/ui/public/js/juce/"
    COMMENT "Copying JUCE JavaScript files"
)

# Rename juce's index.js to juce_index.js to avoid conflicts
add_custom_command(
    TARGET BinaryData PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E rename
        "${CMAKE_CURRENT_SOURCE_DIR}/source/ui/public/js/juce/index.js"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/ui/public/js/juce/juce_index.js"
    COMMENT "Renaming JUCE index.js to juce_index.js"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/source/ui/public/js/juce/index.js"
)        