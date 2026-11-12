#ifndef UNREALPROSPERO_TEXT_SECURE_STRING_H
#define UNREALPROSPERO_TEXT_SECURE_STRING_H

#include <cstddef>

#if !UNREALPROSPERO_TARGET_WINDOWS
extern "C" CORE_API std::size_t strnlen_s(const char* text, std::size_t limit);
extern "C" CORE_API std::size_t wcsnlen_s(const wchar_t* text, std::size_t limit);
#endif

#endif
