cmake_minimum_required(VERSION 3.24)
project(AnyUnrealShaderLoadingTests LANGUAGES CXX)

set(runtime_root "${CMAKE_CURRENT_SOURCE_DIR}/../../Engine/Platforms/AnyUnreal/Source/Runtime")
set(test_targets AgcHeaderCheck)
option(ANYUNREAL_TEST_CREATE_SHADER "Create AGC shaders through the release runtime" OFF)
if(ANYUNREAL_TEST_CREATE_SHADER)
    find_package(AnyDevKit CONFIG REQUIRED)
    adk_add_executable(AgcHeaderCheck AgcHeader.cpp
        "${runtime_root}/AnyUnrealRHI/Private/AnyUnreal/RHI/Shader/GraphicsShaderLink.cpp"
        "${runtime_root}/AnyUnrealRHI/Private/AnyUnreal/RHI/Shader/AgcShader.cpp"
        "${runtime_root}/AnyUnrealRHI/Private/AnyUnreal/RHI/Shader/ShaderCommands.cpp"
        "${runtime_root}/Core/Private/AnyUnreal/Kernel/DirectAllocation.cpp")
    adk_add_executable(ComputeCheck Compute.cpp
        "${runtime_root}/AnyUnrealRHI/Private/AnyUnreal/RHI/Command/Viewport.cpp"
        "${runtime_root}/AnyUnrealRHI/Private/AnyUnreal/RHI/Descriptor/SamplerDescriptor.cpp"
        "${runtime_root}/AnyUnrealRHI/Private/AnyUnreal/RHI/Texture/LinearTexture2D.cpp"
        "${runtime_root}/AnyUnrealRHI/Private/AnyUnreal/RHI/Memory/Upload.cpp"
        "${runtime_root}/AnyUnrealRHI/Private/AnyUnreal/RHI/Memory/Readback.cpp"
        "${runtime_root}/AnyUnrealRHI/Private/AnyUnreal/RHI/Command/BufferCopy.cpp"
        "${runtime_root}/AnyUnrealRHI/Private/AnyUnreal/RHI/Command/Completion.cpp"
        "${runtime_root}/AnyUnrealRHI/Private/AnyUnreal/RHI/Command/CommandStorage.cpp"
        "${runtime_root}/AnyUnrealRHI/Private/AnyUnreal/RHI/Command/MemoryBarrier.cpp"
        "${runtime_root}/AnyUnrealRHI/Private/AnyUnreal/RHI/Command/BufferFill.cpp"
        "${runtime_root}/AnyUnrealRHI/Private/AnyUnreal/RHI/Command/MemoryAcquire.cpp"
        "${runtime_root}/AnyUnrealShaderCommon/Private/AnyUnreal/Shader/ConstantData.cpp"
        "${runtime_root}/AnyUnrealRHI/Private/AnyUnreal/RHI/Command/ComputeDispatch.cpp"
        "${runtime_root}/AnyUnrealShaderCommon/Private/AnyUnreal/Shader/ShaderBinary.cpp"
        "${runtime_root}/AnyUnrealRHI/Private/AnyUnreal/RHI/Descriptor/Tables.cpp"
        "${runtime_root}/AnyUnrealRHI/Private/AnyUnreal/RHI/Descriptor/BufferDescriptor.cpp"
        "${runtime_root}/AnyUnrealRHI/Private/AnyUnreal/RHI/Command/Submission.cpp"
        "${runtime_root}/AnyUnrealRHI/Private/AnyUnreal/RHI/Shader/AgcShader.cpp"
        "${runtime_root}/AnyUnrealRHI/Private/AnyUnreal/RHI/Shader/ShaderCommands.cpp"
        "${runtime_root}/AnyUnrealRHI/Private/AnyUnreal/RHI/Memory/BufferAllocation.cpp"
        "${runtime_root}/Core/Private/AnyUnreal/Kernel/DirectAllocation.cpp")
    adk_add_executable(GraphicsCheck Graphics.cpp
        "${runtime_root}/AnyUnrealRHI/Private/AnyUnreal/RHI/Descriptor/VertexBufferTable.cpp"
        "${runtime_root}/AnyUnrealRHI/Private/AnyUnreal/RHI/Shader/GraphicsShaderLink.cpp"
        "${runtime_root}/AnyUnrealRHI/Private/AnyUnreal/RHI/Shader/ShaderCommands.cpp"
        "${runtime_root}/AnyUnrealRHI/Private/AnyUnreal/RHI/Shader/AgcShader.cpp"
        "${runtime_root}/AnyUnrealRHI/Private/AnyUnreal/RHI/Command/CommandStorage.cpp"
        "${runtime_root}/AnyUnrealRHI/Private/AnyUnreal/RHI/Command/MemoryAcquire.cpp"
        "${runtime_root}/AnyUnrealRHI/Private/AnyUnreal/RHI/Command/Submission.cpp"
        "${runtime_root}/AnyUnrealRHI/Private/AnyUnreal/RHI/Command/Viewport.cpp"
        "${runtime_root}/AnyUnrealRHI/Private/AnyUnreal/RHI/Descriptor/BufferDescriptor.cpp"
        "${runtime_root}/AnyUnrealRHI/Private/AnyUnreal/RHI/Memory/BufferAllocation.cpp"
        "${runtime_root}/Core/Private/AnyUnreal/Kernel/DirectAllocation.cpp")
    list(APPEND test_targets ComputeCheck GraphicsCheck)
    adk_link_libraries(GraphicsCheck libc libkernel libSceAgc libSceAgcDriver)
    adk_link_libraries(ComputeCheck libc libkernel libSceAgc libSceAgcDriver)
    adk_link_libraries(AgcHeaderCheck libc libkernel libSceAgc)
    foreach(test_target IN LISTS test_targets)
        set_target_properties(${test_target} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin")
        target_compile_definitions(${test_target} PRIVATE ANYUNREAL_TEST_CREATE_SHADER=1)
        if(ADK_TARGET STREQUAL "windows")
            target_compile_definitions(${test_target} PRIVATE ANYUNREAL_TEST_AGC_TARGET=Windows)
        elseif(ADK_TARGET STREQUAL "linux")
            target_compile_definitions(${test_target} PRIVATE ANYUNREAL_TEST_AGC_TARGET=Linux)
        elseif(ADK_TARGET STREQUAL "ps5")
            target_compile_definitions(${test_target} PRIVATE ANYUNREAL_TEST_AGC_TARGET=Ps5)
        else()
            message(FATAL_ERROR "Unknown AGC test platform")
        endif()
    endforeach()
else()
    if(NOT EXISTS "${ADK_HEADERS}/SceShaders.hpp")
        message(FATAL_ERROR "ADK_HEADERS must point to AnyPS5 public release headers")
    endif()
    add_executable(AgcHeaderCheck AgcHeader.cpp)
    target_include_directories(AgcHeaderCheck SYSTEM PRIVATE "${ADK_HEADERS}")
    target_compile_definitions(AgcHeaderCheck PRIVATE ANYUNREAL_TEST_CREATE_SHADER=0)
endif()
foreach(test_target IN LISTS test_targets)
    target_sources(${test_target} PRIVATE "${runtime_root}/AnyUnrealRHI/Private/AnyUnreal/RHI/Shader/AgcHeader.cpp")
    target_sources(${test_target} PRIVATE "${runtime_root}/AnyUnrealShaderCommon/Private/AnyUnreal/Shader/UserData.cpp")
    target_include_directories(${test_target} PRIVATE "${runtime_root}/AnyUnrealRHI/Public" "${runtime_root}/AnyUnrealShaderCommon/Public" "${runtime_root}/Core/Public")
    target_compile_definitions(${test_target} PRIVATE ANYUNREALRHI_API= ANYUNREALSHADERCOMMON_API=)
    target_compile_features(${test_target} PRIVATE cxx_std_20)
    set_target_properties(${test_target} PROPERTIES CXX_EXTENSIONS OFF)
    if(MSVC)
        target_compile_options(${test_target} PRIVATE /W4 /WX)
    else()
        target_compile_options(${test_target} PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()

endforeach()
