cmake_minimum_required(VERSION 3.22)

# ──────────────────────────────────────────────────────────────────────────────
# NFReverb — Deep House Spring Reverb (NeonFameReverberation)
# UI: WebView (WebView2 on Win, WKWebView on Mac)  |  Framework: JUCE 8
# ──────────────────────────────────────────────────────────────────────────────

# Platform-specific configuration
if(WIN32)
    set(PLUGIN_FORMATS VST3 Standalone)
    set(NEEDS_WEBVIEW2 TRUE)
    set(NEEDS_WEB_BROWSER FALSE)
    set(WEBVIEW_BACKEND "WebView2")
elseif(APPLE)
    set(PLUGIN_FORMATS VST3 AU Standalone)
    set(NEEDS_WEBVIEW2 FALSE)
    set(NEEDS_WEB_BROWSER FALSE)
    set(AU_MAIN_TYPE kAudioUnitType_Effect)
    set(WEBVIEW_BACKEND "WKWebView")
else()
    message(FATAL_ERROR "Unsupported platform")
endif()

message(STATUS "NFReverb: Building for ${PLUGIN_FORMATS}")
message(STATUS "NFReverb: WebView backend: ${WEBVIEW_BACKEND}")

juce_add_plugin(NFReverb
    COMPANY_NAME            "APC"
    PLUGIN_MANUFACTURER_CODE Apco
    PLUGIN_CODE             MRvb
    FORMATS                 ${PLUGIN_FORMATS}
    PRODUCT_NAME            "NeonFameReverberation"
    VERSION                 "1.1.0"
    NEEDS_WEBVIEW2          ${NEEDS_WEBVIEW2}
    NEEDS_WEB_BROWSER       ${NEEDS_WEB_BROWSER}
    VST3_CATEGORIES         Fx Reverb
    AU_MAIN_TYPE            ${AU_MAIN_TYPE}
)

# ──────────────────────────────────────────────────────────────────────────────
# Embed web UI files as binary data
# NOTE: JUCE mangles duplicate filenames — two files named "index.js" become
#       index_js (js/index.js) and index_js2 (js/juce/index.js)
# ──────────────────────────────────────────────────────────────────────────────
juce_add_binary_data(NFReverb_WebUI
    SOURCES
        Source/ui/public/index.html
        Source/ui/public/js/index.js
        Source/ui/public/js/juce/index.js
        Source/ui/public/js/juce/check_native_interop.js
)

# ──────────────────────────────────────────────────────────────────────────────
# Source files
# ──────────────────────────────────────────────────────────────────────────────
target_sources(NFReverb
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
)

target_include_directories(NFReverb
    PRIVATE
        Source
)

# ──────────────────────────────────────────────────────────────────────────────
# Link libraries
# ──────────────────────────────────────────────────────────────────────────────
target_link_libraries(NFReverb
    PRIVATE
        NFReverb_WebUI
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_plugin_client
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_core
        juce::juce_data_structures
        juce::juce_dsp
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# ──────────────────────────────────────────────────────────────────────────────
# Compile definitions
# ──────────────────────────────────────────────────────────────────────────────
target_compile_definitions(NFReverb
    PUBLIC
        JUCE_WEB_BROWSER=1
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

if(WIN32)
    target_compile_definitions(NFReverb
        PUBLIC
            JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
    )
endif()
