cmake_minimum_required(VERSION 3.24)
project(UnrealProsperoBinkAudio LANGUAGES C CXX)

find_package(AnyDevKit CONFIG REQUIRED)
cmake_path(CONVERT "${BINK_AUDIO_SDK_DIRECTORY}" TO_CMAKE_PATH_LIST BINK_AUDIO_SDK_DIRECTORY NORMALIZE)
if(NOT EXISTS "${BINK_AUDIO_SDK_DIRECTORY}/Include/binka_ue_decode.h")
    message(FATAL_ERROR "Set BINK_AUDIO_SDK_DIRECTORY to the Bink Audio SDK supplied with Unreal")
endif()
if(NOT BINK_AUDIO_PLATFORM STREQUAL "AnyPS5")
    message(FATAL_ERROR "Set BINK_AUDIO_PLATFORM to AnyPS5")
endif()

add_library(binka_ue_decode STATIC
    "${BINK_AUDIO_SDK_DIRECTORY}/Src/binkacd.cpp"
    "${BINK_AUDIO_SDK_DIRECTORY}/Src/binka_ue_decode.cpp"
    "${BINK_AUDIO_SDK_DIRECTORY}/Src/radfft.cpp"
    "${BINK_AUDIO_SDK_DIRECTORY}/Src/x86_cpu.c"
)
target_include_directories(binka_ue_decode PRIVATE "${BINK_AUDIO_SDK_DIRECTORY}/Include")
target_compile_definitions(binka_ue_decode PRIVATE __GCC__ __RADINSTATICLIB__ WRAP_PUBLICS=BACDUE)
target_compile_options(binka_ue_decode PRIVATE -mssse3 -ffp-contract=off -fno-exceptions $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti> -fno-strict-aliasing -fno-stack-protector)
set_target_properties(binka_ue_decode PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON POSITION_INDEPENDENT_CODE ON OUTPUT_NAME "binka_ue_decode_${BINK_AUDIO_PLATFORM}")
target_link_libraries(binka_ue_decode PRIVATE AnyLibcxx::Runtime)
install(TARGETS binka_ue_decode ARCHIVE DESTINATION Lib)
