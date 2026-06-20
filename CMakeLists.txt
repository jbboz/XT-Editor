cmake_minimum_required(VERSION 3.22)

project(mw2xt_editor
    VERSION 0.1.0
    DESCRIPTION "Waldorf Microwave II/XT Editor"
    HOMEPAGE_URL "https://github.com/jbboz/XT-Editor"
    LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "macOS deployment target")
endif()

# ─── mw2xtLib ──────────────────────────────────────────────────────────────
# Pure C++17 protocol library. No JUCE dependency.
# Linked by both the plugin and the test binaries.
add_library(mw2xtLib STATIC
    source/mw2xtLib/Protocol.cpp
    source/mw2xtLib/WaveCodec.cpp
)
target_include_directories(mw2xtLib PUBLIC source)
target_compile_features(mw2xtLib PUBLIC cxx_std_17)

# ─── JUCE plugin ───────────────────────────────────────────────────────────
add_subdirectory(submodules/JUCE)

# ─── Skin binary data ───────────────────────────────────────────────────────
juce_add_binary_data(mw2xtSkinData
    NAMESPACE BinaryData
    SOURCES
        source/mw2xtLib/parameterDescriptions_xt.json
        source/mw2xtUI/skins/xtDefault/xtDefault.json
        source/mw2xtUI/skins/xtDefault/xtDefaultBG.png
        source/mw2xtUI/skins/xtDefault/xtPageOsc.png
        source/mw2xtUI/skins/xtDefault/xt_encoder_ranged.png
        source/mw2xtUI/skins/xtDefault/xt_encoder_ranged_red.png
        source/mw2xtUI/skins/xtDefault/xtknob.png
        source/mw2xtUI/skins/xtDefault/xtknob_small.png
        source/mw2xtUI/skins/xtDefault/led.png
)

juce_add_plugin(mw2xtEditor
    PRODUCT_NAME                "MW2XT Editor"
    COMPANY_NAME                "Retro Pro Audio"
    COMPANY_WEBSITE             "https://github.com/jbboz/XT-Editor"
    BUNDLE_ID                   "com.retroproaudio.mw2xtEditor"
    PLUGIN_MANUFACTURER_CODE    RPAu
    PLUGIN_CODE                 MwXT
    FORMATS                     Standalone VST3 AU
    IS_SYNTH                    FALSE
    IS_MIDI_EFFECT              TRUE
    NEEDS_MIDI_INPUT            TRUE
    NEEDS_MIDI_OUTPUT           TRUE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    AU_MAIN_TYPE                kAudioUnitType_MIDIProcessor
    VST3_CATEGORIES             "Fx" "Tools"
    HARDENED_RUNTIME_ENABLED    TRUE
)

target_sources(mw2xtEditor PRIVATE
    source/mw2xtPlugin/PluginProcessor.cpp
    source/mw2xtPlugin/PluginProcessor.h
    source/mw2xtPlugin/PluginEditor.cpp
    source/mw2xtPlugin/PluginEditor.h
    source/mw2xtEditor/HardwareMidiDevice.cpp
    source/mw2xtEditor/HardwareMidiDevice.h
    source/mw2xtEditor/PatchModel.cpp
    source/mw2xtEditor/PatchModel.h
    source/mw2xtEditor/EditorController.cpp
    source/mw2xtEditor/EditorController.h
    source/mw2xtEditor/SpritesheetKnob.cpp
    source/mw2xtEditor/SpritesheetKnob.h
    source/mw2xtEditor/SpritesheetButton.cpp
    source/mw2xtEditor/SpritesheetButton.h
    source/mw2xtEditor/PageComponent.cpp
    source/mw2xtEditor/PageComponent.h
    source/mw2xtUI/ParamRegistry.cpp
    source/mw2xtUI/ParamRegistry.h
)

target_include_directories(mw2xtEditor PRIVATE source)

target_compile_definitions(mw2xtEditor PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(mw2xtEditor
    PRIVATE
        mw2xtLib
        mw2xtSkinData
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_audio_devices
        juce::juce_data_structures
        juce::juce_gui_extra
        juce::juce_core
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)

# ─── Tests ─────────────────────────────────────────────────────────────────
enable_testing()
add_subdirectory(tests)
