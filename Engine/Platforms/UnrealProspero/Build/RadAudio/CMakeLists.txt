cmake_minimum_required(VERSION 3.24)
project(UnrealProsperoRadAudio LANGUAGES C CXX)

find_package(AnyDevKit CONFIG REQUIRED)
cmake_path(CONVERT "${RAD_AUDIO_SDK_DIRECTORY}" TO_CMAKE_PATH_LIST RAD_AUDIO_SDK_DIRECTORY NORMALIZE)
if(NOT EXISTS "${RAD_AUDIO_SDK_DIRECTORY}/Include/rada_decode.h")
    message(FATAL_ERROR "Set RAD_AUDIO_SDK_DIRECTORY to the RAD Audio SDK supplied with Unreal")
endif()

set(codec "${RAD_AUDIO_SDK_DIRECTORY}/Src/RadAudio")
set(avx2 "${codec}/radaudio_decoder_avx2.c" "${codec}/radaudio_mdct_avx2.cpp")
add_library(radaudio_decoder STATIC
    src/Interleave.cpp
    "${codec}/cpux86.cpp"
    "${codec}/radaudio_decoder.c"
    "${codec}/radaudio_decoder_sse2.c"
    "${codec}/radaudio_decoder_sse4.c"
    "${codec}/radaudio_mdct.cpp"
    "${codec}/radaudio_mdct_sse2.cpp"
    "${RAD_AUDIO_SDK_DIRECTORY}/Src/RadA/rada_decode.cpp"
    ${avx2}
)
target_include_directories(radaudio_decoder PRIVATE "${codec}" "${RAD_AUDIO_SDK_DIRECTORY}/Include")
target_compile_definitions(radaudio_decoder PRIVATE USING_EGT __RADINSTATICLIB__ RADAUDIO_WRAP=UERA RADA_WRAP=UERA)
target_compile_options(radaudio_decoder PRIVATE -ffp-contract=off -fno-exceptions -fno-strict-aliasing -fno-stack-protector $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>)
if(WIN32)
    target_compile_options(radaudio_decoder PRIVATE -fasm-blocks)
endif()
set_source_files_properties(${avx2} PROPERTIES COMPILE_OPTIONS "-mavx2")
set_source_files_properties("${codec}/radaudio_decoder_sse4.c" PROPERTIES COMPILE_OPTIONS "-msse4.1")
set_target_properties(radaudio_decoder PROPERTIES POSITION_INDEPENDENT_CODE ON C_STANDARD 99 C_STANDARD_REQUIRED ON CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
target_link_libraries(radaudio_decoder PRIVATE AnyLibcxx::Runtime)
if(NOT RAD_AUDIO_PLATFORM STREQUAL "AnyPS5")
    message(FATAL_ERROR "Set RAD_AUDIO_PLATFORM to AnyPS5")
endif()
set_target_properties(radaudio_decoder PROPERTIES OUTPUT_NAME "radaudio_decoder_${RAD_AUDIO_PLATFORM}")
install(TARGETS radaudio_decoder ARCHIVE DESTINATION Lib)
