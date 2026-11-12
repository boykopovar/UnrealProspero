cmake_minimum_required(VERSION 3.24)
project(UnrealProsperoOodleArchive LANGUAGES NONE)
foreach(input OODLE_ARCHIVE LIBC_IMPORT_LIBRARY)
    if(NOT DEFINED ${input} OR NOT EXISTS "${${input}}" OR IS_DIRECTORY "${${input}}")
        message(FATAL_ERROR "Set ${input} to an existing archive or import library")
    endif()
endforeach()
find_program(LLVM_NM NAMES llvm-nm REQUIRED)
find_program(LLVM_OBJCOPY NAMES llvm-objcopy REQUIRED)
set(output "${CMAKE_CURRENT_BINARY_DIR}/liboo2core.a")
add_custom_command(OUTPUT "${output}"
    COMMAND "${CMAKE_COMMAND}" "-DOODLE_ARCHIVE=${OODLE_ARCHIVE}" "-DLIBC_IMPORT_LIBRARY=${LIBC_IMPORT_LIBRARY}" "-DLLVM_NM=${LLVM_NM}" "-DLLVM_OBJCOPY=${LLVM_OBJCOPY}" "-DOUTPUT=${output}" -P "${CMAKE_CURRENT_SOURCE_DIR}/PrepareArchive.cmake"
    DEPENDS "${OODLE_ARCHIVE}" "${LIBC_IMPORT_LIBRARY}" "${LLVM_NM}" "${LLVM_OBJCOPY}" "${CMAKE_CURRENT_SOURCE_DIR}/PrepareArchive.cmake"
    VERBATIM)
add_custom_target(OodleArchive ALL DEPENDS "${output}")
install(FILES "${output}" DESTINATION lib)
