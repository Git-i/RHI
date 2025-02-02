#pragma once
#if defined(_MSC_VER) && defined(USE_DLL)
#ifdef RHI_DLL
#define RHI_API __declspec(dllexport)
#else
#define RHI_API __declspec(dllimport)
#endif // RHI_DLL
#else
#define RHI_API  
#endif
typedef void* Internal_ID;
typedef unsigned int RESULT;

#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN
	#define NOMINMAX
	#include <Windows.h>
#endif



#include <cstdint>
#include <cstddef>
#include <cstring>
#include <limits.h>
#include <float.h>
#include "result.hpp"

#ifndef DEINFE_ENUM_FLAG_OPERATORS
template <size_t S> struct _ENUM_FLAG_INTEGER_FOR_SIZE;



#ifdef _MSC_VER
#define DEBUG_BREAK __debugbreak()
#else
#include <signal.h>
#define DEBUG_BREAK raise(SIGTRAP);
#endif

#define DEFINE_ENUM_FLAG_OPERATORS(ENUMTYPE) \
extern "C++" { \
inline ENUMTYPE operator | (ENUMTYPE a, ENUMTYPE b) noexcept { return ENUMTYPE(((std::underlying_type<ENUMTYPE>::type)a) | ((std::underlying_type<ENUMTYPE>::type)b)); } \
inline ENUMTYPE &operator |= (ENUMTYPE &a, ENUMTYPE b) noexcept { return (ENUMTYPE &)(((std::underlying_type<ENUMTYPE>::type &)a) |= ((std::underlying_type<ENUMTYPE>::type)b)); } \
inline ENUMTYPE operator & (ENUMTYPE a, ENUMTYPE b) noexcept { return ENUMTYPE(((std::underlying_type<ENUMTYPE>::type)a) & ((std::underlying_type<ENUMTYPE>::type)b)); } \
inline ENUMTYPE &operator &= (ENUMTYPE &a, ENUMTYPE b) noexcept { return (ENUMTYPE &)(((std::underlying_type<ENUMTYPE>::type &)a) &= ((std::underlying_type<ENUMTYPE>::type)b)); } \
inline ENUMTYPE operator ~ (ENUMTYPE a) noexcept { return ENUMTYPE(~((std::underlying_type<ENUMTYPE>::type)a)); } \
inline ENUMTYPE operator ^ (ENUMTYPE a, ENUMTYPE b) noexcept { return ENUMTYPE(((std::underlying_type<ENUMTYPE>::type)a) ^ ((std::underlying_type<ENUMTYPE>::type)b)); } \
inline ENUMTYPE &operator ^= (ENUMTYPE &a, ENUMTYPE b) noexcept { return (ENUMTYPE &)(((std::underlying_type<ENUMTYPE>::type &)a) ^= ((std::underlying_type<ENUMTYPE>::type)b)); } \
}
#endif



#define DECL_STRUCT_CONSTRUCTORS(x) x(){}; x(Default_t); x(Zero_t);
#define DECL_CLASS_CONSTRUCTORS(x) x() = default;x(const x&) = default;x(x&&) = default;

