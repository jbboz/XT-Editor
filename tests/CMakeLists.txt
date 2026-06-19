add_executable(test_protocol test_protocol.cpp)
target_link_libraries(test_protocol PRIVATE mw2xtLib)
add_test(NAME ProtocolTests COMMAND test_protocol)

add_executable(test_wave_codec test_wave_codec.cpp)
target_link_libraries(test_wave_codec PRIVATE mw2xtLib)
add_test(NAME WaveCodecTests COMMAND test_wave_codec)
