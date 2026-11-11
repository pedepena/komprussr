cmake_minimum_required(VERSION 3.22)
project(KOMPRUSSR VERSION 0.1.0 LANGUAGES C CXX)

add_subdirectory(JUCE)

juce_add_plugin(KOMPRUSSR
    COMPANY_NAME "KOMPRUSSR"
    BUNDLE_ID "com.pedepena.komprussr"
    PLUGIN_MANUFACTURER_CODE Kmpr
    PLUGIN_CODE K001
    FORMATS VST3
    PRODUCT_NAME "KOMPRUSSR KU-01"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
)
juce_generate_juce_header(KOMPRUSSR)

target_sources(KOMPRUSSR PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
)

target_compile_features(KOMPRUSSR PRIVATE cxx_std_17)
target_compile_definitions(KOMPRUSSR PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)
target_link_libraries(KOMPRUSSR PRIVATE
    juce::juce_audio_utils
    PUBLIC juce::juce_recommended_config_flags
           juce::juce_recommended_lto_flags
           juce::juce_recommended_warning_flags
)
