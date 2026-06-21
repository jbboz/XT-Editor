add_executable(test_protocol test_protocol.cpp)
target_link_libraries(test_protocol PRIVATE mw2xtLib)
add_test(NAME ProtocolTests COMMAND test_protocol)

add_executable(test_wave_codec test_wave_codec.cpp)
target_link_libraries(test_wave_codec PRIVATE mw2xtLib)
add_test(NAME WaveCodecTests COMMAND test_wave_codec)

# PatchModel tests require JUCE (UndoManager, ChangeBroadcaster).
# juce_add_console_app sets up the required module defines without any GUI.
juce_add_console_app(test_patch_model PRODUCT_NAME "PatchModel Tests")
target_sources(test_patch_model PRIVATE
    test_patch_model.cpp
    ../source/mw2xtEditor/PatchModel.cpp
)
target_include_directories(test_patch_model PRIVATE
    ${CMAKE_SOURCE_DIR}/source
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(test_patch_model PRIVATE
    mw2xtLib
    juce::juce_data_structures
    juce::juce_recommended_warning_flags
    juce::juce_recommended_config_flags
)
add_test(NAME PatchModelTests COMMAND test_patch_model)

juce_add_console_app(test_conditional_image PRODUCT_NAME "ConditionalImage Tests")
target_sources(test_conditional_image PRIVATE
    test_conditional_image.cpp
    ../source/mw2xtEditor/ConditionalImage.cpp
)
target_include_directories(test_conditional_image PRIVATE
    ${CMAKE_SOURCE_DIR}/source
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(test_conditional_image PRIVATE
    juce::juce_gui_basics
    juce::juce_recommended_warning_flags
    juce::juce_recommended_config_flags
)
add_test(NAME ConditionalImageTests COMMAND test_conditional_image)
