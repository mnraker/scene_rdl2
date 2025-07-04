// Copyright 2023-2025 DreamWorks Animation LLC
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <atomic>
#include <type_traits>
#include <memory>

// In C++17, std::atomic<float>, std::atomic<double>, and std::atomic<long
// double> behave more like the integral atomic types. We're going to add this
// behavior. A C++17 library that supports this behavior will define this
// preprocessor directive.
#if !defined(__cpp_lib_atomic_float)

#if defined(__GNUC__) && !defined(__INTEL_COMPILER)
// GCC complains about some of the pointers being passed into the intrinsics
// aren't being used, which is ironic, as those are GCC intrinsics.
#pragma GCC diagnostic ignored "-Wunused-but-set-parameter"
#endif

#ifdef _MSC_VER

// Copied from MS STL std::atomic

/*
enum {
    _Atomic_memory_order_relaxed,
    _Atomic_memory_order_consume,
    _Atomic_memory_order_acquire,
    _Atomic_memory_order_release,
    _Atomic_memory_order_acq_rel,
    _Atomic_memory_order_seq_cst,
};
*/

// Simple mapping to GCC constants

static const int __ATOMIC_RELAXED = _Atomic_memory_order_relaxed;
static const int __ATOMIC_CONSUME = _Atomic_memory_order_consume;
static const int __ATOMIC_ACQUIRE = _Atomic_memory_order_acquire;
static const int __ATOMIC_RELEASE = _Atomic_memory_order_release;
static const int __ATOMIC_ACQ_REL = _Atomic_memory_order_acq_rel;
static const int __ATOMIC_SEQ_CST = _Atomic_memory_order_seq_cst;

#endif // _MSC_VER

namespace af_detail {

// We do a lot of casting to integer values for the intrinsics interface. Let's make sure we are casting the values we
// think we are.
static_assert(__ATOMIC_RELAXED == static_cast<int>(std::memory_order_relaxed));
static_assert(__ATOMIC_CONSUME == static_cast<int>(std::memory_order_consume));
static_assert(__ATOMIC_ACQUIRE == static_cast<int>(std::memory_order_acquire));
static_assert(__ATOMIC_RELEASE == static_cast<int>(std::memory_order_release));
static_assert(__ATOMIC_ACQ_REL == static_cast<int>(std::memory_order_acq_rel));
static_assert(__ATOMIC_SEQ_CST == static_cast<int>(std::memory_order_seq_cst));

#ifdef _MSC_VER

// Copied from MS STL std::atomic which is the type passed to InterlockedExchange128
struct Int128 {
    alignas(16) long long _Low;
    long long _High;
};

static_assert(sizeof(Int128) == 16,
                "If it's bigger, our architecture (at the time of this writing) won't make it atomic");

// The following is code adapted from MSVC's STL to work on floats/doubles and
// wrapped into GCC's __atomic built-in functions to be fairly 1:1 with the
// current implementation.

// Copyright (c) Microsoft Corporation.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

// Copied out of MSVC STL's std::atomic

// Controls whether ARM64 ldar/ldapr/stlr should be used
#ifndef _STD_ATOMIC_USE_ARM64_LDAR_STLR
#if defined(_M_ARM64) || defined(_M_ARM64EC)
#ifdef __clang__ // TRANSITION, LLVM-62103
#define _STD_ATOMIC_USE_ARM64_LDAR_STLR 0
#else // ^^^ Clang doesn't support new intrinsics / __load_acquire/__stlr intrinsics are available vvv
#define _STD_ATOMIC_USE_ARM64_LDAR_STLR 1
#endif // ^^^ __load_acquire/__stlr intrinsics are available ^^^
#else // ^^^ ARM64/ARM64EC / Other architectures vvv
#define _STD_ATOMIC_USE_ARM64_LDAR_STLR 0
#endif // ^^^ Other architectures ^^^
#endif // _STD_ATOMIC_USE_ARM64_LDAR_STLR

// The following code is SHARED with vcruntime and any updates
// should be mirrored. Also: if any macros are added they should be
// #undefed in vcruntime as well

#ifndef _INVALID_MEMORY_ORDER
#ifdef _DEBUG
#define _INVALID_MEMORY_ORDER _STL_REPORT_ERROR("Invalid memory order")
#else // ^^^ defined(_DEBUG) / !defined(_DEBUG) vvv
#define _INVALID_MEMORY_ORDER
#endif // ^^^ !defined(_DEBUG) ^^^
#endif // _INVALID_MEMORY_ORDER

#if defined(_M_IX86) || (defined(_M_X64) && !defined(_M_ARM64EC))
#define _ATOMIC_CHOOSE_INTRINSIC(_Order, _Result, _Intrinsic, ...) \
    _Check_memory_order(_Order);                                   \
    _Result = _Intrinsic(__VA_ARGS__)
#elif defined(_M_ARM) || defined(_M_ARM64) || defined(_M_ARM64EC)
#define _ATOMIC_CHOOSE_INTRINSIC(_Order, _Result, _Intrinsic, ...) \
    switch (_Order) {                                              \
    case _Atomic_memory_order_relaxed:                             \
        _Result = _INTRIN_RELAXED(_Intrinsic)(__VA_ARGS__);        \
        break;                                                     \
    case _Atomic_memory_order_consume:                             \
    case _Atomic_memory_order_acquire:                             \
        _Result = _INTRIN_ACQUIRE(_Intrinsic)(__VA_ARGS__);        \
        break;                                                     \
    case _Atomic_memory_order_release:                             \
        _Result = _INTRIN_RELEASE(_Intrinsic)(__VA_ARGS__);        \
        break;                                                     \
    default:                                                       \
        _INVALID_MEMORY_ORDER;                                     \
        _FALLTHROUGH;                                              \
    case _Atomic_memory_order_acq_rel:                             \
    case _Atomic_memory_order_seq_cst:                             \
        _Result = _Intrinsic(__VA_ARGS__);                         \
        break;                                                     \
    }
#endif // hardware


#define _ATOMIC_POST_LOAD_BARRIER_AS_NEEDED(_Order_var) \
    switch (_Order_var) {                               \
    case _Atomic_memory_order_relaxed:                  \
        break;                                          \
    case _Atomic_memory_order_consume:                  \
    case _Atomic_memory_order_acquire:                  \
    case _Atomic_memory_order_seq_cst:                  \
        _Compiler_or_memory_barrier();                  \
        break;                                          \
    case _Atomic_memory_order_release:                  \
    case _Atomic_memory_order_acq_rel:                  \
    default:                                            \
        _INVALID_MEMORY_ORDER;                          \
        break;                                          \
    }

#if _STD_ATOMIC_USE_ARM64_LDAR_STLR == 1

#define __STORE_RELEASE(_Width, _Ptr, _Desired) \
    _Compiler_barrier();                        \
    __stlr##_Width(                             \
        reinterpret_cast<volatile unsigned __int##_Width*>(_Ptr), static_cast<unsigned __int##_Width>(_Desired));

#else // ^^^ _STD_ATOMIC_USE_ARM64_LDAR_STLR == 1 / _STD_ATOMIC_USE_ARM64_LDAR_STLR == 0 vvv

#define __STORE_RELEASE(_Width, _Ptr, _Desired) \
    _Compiler_or_memory_barrier();              \
    __iso_volatile_store##_Width((_Ptr), (_Desired));

#endif // ^^^ _STD_ATOMIC_USE_ARM64_LDAR_STLR == 0 ^^^


#define _ATOMIC_STORE_PREFIX(_Width, _Ptr, _Desired)      \
    case _Atomic_memory_order_relaxed:                    \
        __iso_volatile_store##_Width((_Ptr), (_Desired)); \
        return;                                           \
    case _Atomic_memory_order_release:                    \
        __STORE_RELEASE(_Width, _Ptr, _Desired)           \
        return;                                           \
    default:                                              \
    case _Atomic_memory_order_consume:                    \
    case _Atomic_memory_order_acquire:                    \
    case _Atomic_memory_order_acq_rel:                    \
        _INVALID_MEMORY_ORDER;                            \
        _FALLTHROUGH;

#define _ATOMIC_STORE_SEQ_CST_ARM(_Width, _Ptr, _Desired) \
    _Memory_barrier();                                    \
    __iso_volatile_store##_Width((_Ptr), (_Desired));     \
    _Memory_barrier();

#if _STD_ATOMIC_USE_ARM64_LDAR_STLR == 1
#define _ATOMIC_STORE_SEQ_CST_ARM64(_Width, _Ptr, _Desired)                                                       \
    _Compiler_barrier();                                                                                          \
    __stlr##_Width(                                                                                               \
        reinterpret_cast<volatile unsigned __int##_Width*>(_Ptr), static_cast<unsigned __int##_Width>(_Desired)); \
    _Memory_barrier();
#else // ^^^ _STD_ATOMIC_USE_ARM64_LDAR_STLR == 1 / _STD_ATOMIC_USE_ARM64_LDAR_STLR == 0 vvv
#define _ATOMIC_STORE_SEQ_CST_ARM64 _ATOMIC_STORE_SEQ_CST_ARM
#endif // ^^^ _STD_ATOMIC_USE_ARM64_LDAR_STLR == 0 ^^^

#define _ATOMIC_STORE_SEQ_CST_X86_X64(_Width, _Ptr, _Desired) (void) _InterlockedExchange##_Width((_Ptr), (_Desired));
#define _ATOMIC_STORE_32_SEQ_CST_X86_X64(_Ptr, _Desired) \
    (void) _InterlockedExchange(reinterpret_cast<volatile long*>(_Ptr), static_cast<long>(_Desired));

#define _ATOMIC_STORE_64_SEQ_CST_IX86(_Ptr, _Desired) \
    _Compiler_barrier();                              \
    __iso_volatile_store64((_Ptr), (_Desired));       \
    _Atomic_thread_fence(_Atomic_memory_order_seq_cst);

#if defined(_M_ARM)
#define _ATOMIC_STORE_SEQ_CST(_Width, _Ptr, _Desired) _ATOMIC_STORE_SEQ_CST_ARM(_Width, (_Ptr), (_Desired))
#define _ATOMIC_STORE_32_SEQ_CST(_Ptr, _Desired)      _ATOMIC_STORE_SEQ_CST_ARM(32, (_Ptr), (_Desired))
#define _ATOMIC_STORE_64_SEQ_CST(_Ptr, _Desired)      _ATOMIC_STORE_SEQ_CST_ARM(64, (_Ptr), (_Desired))
#elif defined(_M_ARM64) || defined(_M_ARM64EC) // ^^^ ARM32 / ARM64/ARM64EC vvv
#define _ATOMIC_STORE_SEQ_CST(_Width, _Ptr, _Desired) _ATOMIC_STORE_SEQ_CST_ARM64(_Width, (_Ptr), (_Desired))
#define _ATOMIC_STORE_32_SEQ_CST(_Ptr, _Desired)      _ATOMIC_STORE_SEQ_CST_ARM64(32, (_Ptr), (_Desired))
#define _ATOMIC_STORE_64_SEQ_CST(_Ptr, _Desired)      _ATOMIC_STORE_SEQ_CST_ARM64(64, (_Ptr), (_Desired))
#elif defined(_M_IX86) || defined(_M_X64) // ^^^ ARM64/ARM64EC / x86/x64 vvv
#define _ATOMIC_STORE_SEQ_CST(_Width, _Ptr, _Desired) _ATOMIC_STORE_SEQ_CST_X86_X64(_Width, (_Ptr), (_Desired))
#define _ATOMIC_STORE_32_SEQ_CST(_Ptr, _Desired)      _ATOMIC_STORE_32_SEQ_CST_X86_X64((_Ptr), (_Desired))
#ifdef _M_IX86
#define _ATOMIC_STORE_64_SEQ_CST(_Ptr, _Desired) _ATOMIC_STORE_64_SEQ_CST_IX86((_Ptr), (_Desired))
#else // ^^^ x86 / x64 vvv
#define _ATOMIC_STORE_64_SEQ_CST(_Ptr, _Desired) _ATOMIC_STORE_SEQ_CST_X86_X64(64, (_Ptr), (_Desired))
#endif // ^^^ x64 ^^^
#else // ^^^ x86/x64 / Unsupported hardware vvv
#error "Unsupported hardware"
#endif // ^^^ Unsupported hardware ^^^

template <class _Integral, class _Ty>
_NODISCARD _Integral _Atomic_reinterpret_as(const _Ty& _Source) noexcept {
    // interprets _Source as the supplied integral type
    static_assert(is_integral_v<_Integral>, "Tried to reinterpret memory as non-integral");
    if constexpr (is_integral_v<_Ty> && sizeof(_Integral) == sizeof(_Ty)) {
        return static_cast<_Integral>(_Source);
    } else if constexpr (is_pointer_v<_Ty> && sizeof(_Integral) == sizeof(_Ty)) {
        return reinterpret_cast<_Integral>(_Source);
    } else {
        _Integral _Result{}; // zero padding bits
        ::memcpy(&_Result, std::addressof(_Source), sizeof(_Source));
        return _Result;
    }
}

// Padding bits should not participate in cmpxchg comparison starting in C++20.
// Clang does not have __builtin_zero_non_value_bits to exclude these bits to implement this C++20 feature.
// The EDG front-end substitutes everything and runs into incomplete types passed to atomic<T>.
#if __cplusplus >= 202002L && !defined(__clang__) /* TRANSITION, LLVM-46685 */ && !defined(__EDG__)
#define _CMPXCHG_MASK_OUT_PADDING_BITS 1
#else
#define _CMPXCHG_MASK_OUT_PADDING_BITS 0
#endif

#if _CMPXCHG_MASK_OUT_PADDING_BITS
struct _Form_mask_t {};
_INLINE_VAR constexpr _Form_mask_t _Form_mask{};
#endif // _CMPXCHG_MASK_OUT_PADDING_BITS

template <class _Ty>
struct _Storage_for {
    // uninitialized space to store a _Ty
    alignas(_Ty) unsigned char _Storage[sizeof(_Ty)];

    _Storage_for()                               = default;
    _Storage_for(const _Storage_for&)            = delete;
    _Storage_for& operator=(const _Storage_for&) = delete;

#if _CMPXCHG_MASK_OUT_PADDING_BITS
    explicit _Storage_for(_Form_mask_t) noexcept {
        ::memset(_Storage, 0xff, sizeof(_Ty));
        __builtin_zero_non_value_bits(_Ptr());
    }
#endif // _CMPXCHG_MASK_OUT_PADDING_BITS

    _NODISCARD _Ty& _Ref() noexcept {
        return reinterpret_cast<_Ty&>(_Storage);
    }

    _NODISCARD _Ty* _Ptr() noexcept {
        return reinterpret_cast<_Ty*>(&_Storage);
    }
};

#if _CMPXCHG_MASK_OUT_PADDING_BITS
template <class _Ty>
inline constexpr bool _Might_have_non_value_bits =
    !has_unique_object_representations_v<_Ty> && !is_floating_point_v<_Ty>;
#endif // _CMPXCHG_MASK_OUT_PADDING_BITS

// These are based on MS STL's std::atomic but tweaked to emulate the GCC __atomic built-ins

static inline void store(char *ptr, const char *_Value, const std::memory_order _Order = std::memory_order_seq_cst) noexcept {
    const char _As_bytes = *_Value;
    switch (static_cast<unsigned int>(_Order)) {
        _ATOMIC_STORE_PREFIX(8, ptr, _As_bytes)
    case std::memory_order_seq_cst:
        _ATOMIC_STORE_SEQ_CST(8, ptr, _As_bytes);
        return;
    }
}

static inline void store(short *ptr, const short *_Value, const std::memory_order _Order = std::memory_order_seq_cst) noexcept {
    const short _As_bytes = *_Value;
    switch (static_cast<unsigned int>(_Order)) {
        _ATOMIC_STORE_PREFIX(16, ptr, _As_bytes)
    case std::memory_order_seq_cst:
        _ATOMIC_STORE_SEQ_CST(16, ptr, _As_bytes);
        return;
    }
}

static inline void store(int *ptr, const int *_Value, const std::memory_order _Order = std::memory_order_seq_cst) noexcept {
    const int _As_bytes = *_Value;
    switch (static_cast<unsigned int>(_Order)) {
        _ATOMIC_STORE_PREFIX(32, ptr, _As_bytes)
    case std::memory_order_seq_cst:
        _ATOMIC_STORE_32_SEQ_CST(ptr, _As_bytes);
        return;
    }
}

static inline void store(long long *ptr, const long long *_Value, const std::memory_order _Order = std::memory_order_seq_cst) noexcept {
    const long long _As_bytes = *_Value;
    switch (static_cast<unsigned int>(_Order)) {
        _ATOMIC_STORE_PREFIX(64, ptr, _As_bytes)
    case std::memory_order_seq_cst:
        _ATOMIC_STORE_64_SEQ_CST(ptr, _As_bytes);
        return;
    }
}

template<typename T>
static inline void store(Int128 *ptr, const Int128 *_Value, const std::memory_order _Order = std::memory_order_seq_cst) noexcept {
    _Check_store_memory_order(_Order);
    (void) exchange<T>(ptr, _Value, _Order);
}

static inline char load(const char *ptr, const std::memory_order _Order = std::memory_order_seq_cst) noexcept {
    char _As_bytes;
#if _STD_ATOMIC_USE_ARM64_LDAR_STLR == 1
    _ATOMIC_LOAD_ARM64(_As_bytes, 8, ptr, static_cast<unsigned int>(_Order))
#else // ^^^ _STD_ATOMIC_USE_ARM64_LDAR_STLR == 1 / _STD_ATOMIC_USE_ARM64_LDAR_STLR == 0 vvv
    _As_bytes = __iso_volatile_load8(ptr);
    _ATOMIC_POST_LOAD_BARRIER_AS_NEEDED(static_cast<unsigned int>(_Order))
#endif // ^^^ _STD_ATOMIC_USE_ARM64_LDAR_STLR == 0 ^^^
    return _As_bytes;
}

static inline short load(const short *ptr, const std::memory_order _Order = std::memory_order_seq_cst) noexcept {
    short _As_bytes;
#if _STD_ATOMIC_USE_ARM64_LDAR_STLR == 1
    _ATOMIC_LOAD_ARM64(_As_bytes, 16, ptr, static_cast<unsigned int>(_Order))
#else // ^^^ _STD_ATOMIC_USE_ARM64_LDAR_STLR == 1 / _STD_ATOMIC_USE_ARM64_LDAR_STLR == 0 vvv
    _As_bytes = __iso_volatile_load16(ptr);
    _ATOMIC_POST_LOAD_BARRIER_AS_NEEDED(static_cast<unsigned int>(_Order))
#endif // ^^^ _STD_ATOMIC_USE_ARM64_LDAR_STLR == 0 ^^^
    return _As_bytes;
}

static inline int load(const int *ptr, const std::memory_order _Order = std::memory_order_seq_cst) noexcept {
    int _As_bytes;
#if _STD_ATOMIC_USE_ARM64_LDAR_STLR == 1
    _ATOMIC_LOAD_ARM64(_As_bytes, 32, ptr, static_cast<unsigned int>(_Order))
#else // ^^^ _STD_ATOMIC_USE_ARM64_LDAR_STLR == 1 / _STD_ATOMIC_USE_ARM64_LDAR_STLR == 0 vvv
    _As_bytes = __iso_volatile_load32(ptr);
    _ATOMIC_POST_LOAD_BARRIER_AS_NEEDED(static_cast<unsigned int>(_Order))
#endif // ^^^ _STD_ATOMIC_USE_ARM64_LDAR_STLR == 0 ^^^
    return _As_bytes;
}

static inline long long load(const long long *ptr, const std::memory_order _Order = std::memory_order_seq_cst) noexcept {
    long long _As_bytes;
#if _STD_ATOMIC_USE_ARM64_LDAR_STLR == 1
    _ATOMIC_LOAD_ARM64(_As_bytes, 64, ptr, static_cast<unsigned int>(_Order))
#else // ^^^ _STD_ATOMIC_USE_ARM64_LDAR_STLR == 1 / _STD_ATOMIC_USE_ARM64_LDAR_STLR != 1 vvv

#ifdef _M_ARM
    _As_bytes           = __ldrexd(ptr);
#else
    _As_bytes = __iso_volatile_load64(ptr);
#endif

    _ATOMIC_POST_LOAD_BARRIER_AS_NEEDED(static_cast<unsigned int>(_Order))
#endif // ^^^ _STD_ATOMIC_USE_ARM64_LDAR_STLR != 1 ^^^
    return _As_bytes;
}

static inline Int128 load(Int128 const *ptr, const std::memory_order _Order = std::memory_order_seq_cst) noexcept {
#if defined(_M_ARM64) || defined(_M_ARM64EC)
    long long* const _ptr = (long long* const)ptr;
    Int128 _Result{}; // atomic CAS 0 with 0
    switch (_Order) {
    case memory_order_relaxed:
        (void) _INTRIN_RELAXED(_InterlockedCompareExchange128)(_ptr, 0, 0, &_Result._Low);
        break;
    case memory_order_consume:
    case memory_order_acquire:
        (void) _INTRIN_ACQUIRE(_InterlockedCompareExchange128)(_ptr, 0, 0, &_Result._Low);
        break;
    default:
    case memory_order_release:
    case memory_order_acq_rel:
        _INVALID_MEMORY_ORDER;
        _FALLTHROUGH;
    case memory_order_seq_cst:
        (void) _InterlockedCompareExchange128(_ptr, 0, 0, &_Result._Low);
        break;
    }

    return _Result;
#else // ^^^ _M_ARM64, _M_ARM64EC / _M_X64 vvv
    long long* const _ptr = (long long* const)ptr;
    Int128 _Result{}; // atomic CAS 0 with 0
    _Check_load_memory_order(_Order);
    (void) _InterlockedCompareExchange128(_ptr, 0, 0, &_Result._Low);
    return _Result;
#endif // ^^^ _M_X64 ^^^
}

static inline char exchange(char *ptr, const char *_Value, const std::memory_order _Order = std::memory_order_seq_cst) noexcept {
    // exchange with given memory order
    char _As_bytes;
    const char _Value_bytes = *_Value;
    _ATOMIC_CHOOSE_INTRINSIC(static_cast<unsigned int>(_Order), _As_bytes, _InterlockedExchange8,
        ptr, _Value_bytes);
    return _As_bytes;
}

static inline short exchange(short *ptr, const short *_Value, const std::memory_order _Order = std::memory_order_seq_cst) noexcept {
    // exchange with given memory order
    short _As_bytes;
    const short _Value_bytes = *_Value;
    _ATOMIC_CHOOSE_INTRINSIC(static_cast<unsigned int>(_Order), _As_bytes, _InterlockedExchange16,
        ptr, _Value_bytes);
    return _As_bytes;
}

static inline long exchange(long *ptr, const long *_Value, const std::memory_order _Order = std::memory_order_seq_cst) noexcept {
    // exchange with given memory order
    long _As_bytes;
    const long _Value_bytes = *_Value;
    _ATOMIC_CHOOSE_INTRINSIC(static_cast<unsigned int>(_Order), _As_bytes, _InterlockedExchange,
        ptr, _Value_bytes);
    return _As_bytes;
}

static inline long long exchange(long long *ptr, const long long *_Value, const std::memory_order _Order = std::memory_order_seq_cst) noexcept {
    // exchange with given memory order
    long long _As_bytes;
    const long long _Value_bytes = *_Value;
    _ATOMIC_CHOOSE_INTRINSIC(static_cast<unsigned int>(_Order), _As_bytes, _InterlockedExchange64,
        ptr, _Value_bytes);
    return _Value_bytes;
}

template<typename T>
static inline Int128 exchange(Int128 *ptr, const Int128 *_Value, const std::memory_order _Order = std::memory_order_seq_cst) noexcept {
    Int128 _Result{_Value};
    while (!compare_exchange_strong<T>(ptr, &_Result, _Value, _Order)) { // keep trying
    }

    return _Result;
}

template<typename T>
static inline bool compare_exchange_strong(char *ptr, char *_Expected, const char *_Desired,
    const std::memory_order _Order = std::memory_order_seq_cst) noexcept { // CAS with given memory order
    char _Expected_bytes = *_Expected; // read before atomic operation
    const char _Desired_bytes = *_Desired;
    char _Prev_bytes;

#if _CMPXCHG_MASK_OUT_PADDING_BITS
    if constexpr (_Might_have_non_value_bits<T>) {
        _Storage_for<T> _Mask{_Form_mask};
        const char _Mask_val = _Atomic_reinterpret_as<char>(_Mask);

        for (;;) {
            _ATOMIC_CHOOSE_INTRINSIC(static_cast<unsigned int>(_Order), _Prev_bytes, _InterlockedCompareExchange8,
                ptr, _Desired,
                _Expected_bytes);
            if (_Prev_bytes == _Expected_bytes) {
                return true;
            }

            if ((_Prev_bytes ^ _Expected_bytes) & _Mask_val) {
                reinterpret_cast<char&>(_Expected) = _Prev_bytes;
                return false;
            }
            _Expected_bytes = (_Expected_bytes & _Mask_val) | (_Prev_bytes & ~_Mask_val);
        }
    }
#endif // _CMPXCHG_MASK_OUT_PADDING_BITS

    _ATOMIC_CHOOSE_INTRINSIC(static_cast<unsigned int>(_Order), _Prev_bytes, _InterlockedCompareExchange8,
        ptr, _Desired_bytes, _Expected_bytes);
    if (_Prev_bytes == _Expected_bytes) {
        return true;
    }

    ::memcpy(_Expected, &_Prev_bytes, sizeof(char));
    return false;
}

template<typename T>
static inline bool compare_exchange_strong(short *ptr, short *_Expected, const short *_Desired,
    const std::memory_order _Order = std::memory_order_seq_cst) noexcept { // CAS with given memory order
    short _Expected_bytes = *_Expected; // read before atomic operation
    const short _Desired_bytes = *_Desired;
    short _Prev_bytes;

#if _CMPXCHG_MASK_OUT_PADDING_BITS
    if constexpr (_Might_have_non_value_bits<T>) {
        _Storage_for<T> _Mask{_Form_mask};
        const short _Mask_val = _Atomic_reinterpret_as<short>(_Mask);

        for (;;) {
            _ATOMIC_CHOOSE_INTRINSIC(static_cast<unsigned int>(_Order), _Prev_bytes, _InterlockedCompareExchange16,
                ptr, _Desired,
                _Expected_bytes);
            if (_Prev_bytes == _Expected_bytes) {
                return true;
            }

            if ((_Prev_bytes ^ _Expected_bytes) & _Mask_val) {
                ::memcpy(_Expected, &_Prev_bytes, sizeof(short));
                return false;
            }
            _Expected_bytes = (_Expected_bytes & _Mask_val) | (_Prev_bytes & ~_Mask_val);
        }
    }
#endif // _CMPXCHG_MASK_OUT_PADDING_BITS

    _ATOMIC_CHOOSE_INTRINSIC(static_cast<unsigned int>(_Order), _Prev_bytes, _InterlockedCompareExchange16,
        ptr, _Desired_bytes, _Expected_bytes);
    if (_Prev_bytes == _Expected_bytes) {
        return true;
    }

    ::memcpy(_Expected, &_Prev_bytes, sizeof(short));
    return false;
}

template<typename T>
static inline bool compare_exchange_strong(long *ptr, long *_Expected, const long *_Desired,
    const std::memory_order _Order = std::memory_order_seq_cst) noexcept { // CAS with given memory order
    long _Expected_bytes = *_Expected; // read before atomic operation
    const long _Desired_bytes = *_Desired;
    long _Prev_bytes;

#if _CMPXCHG_MASK_OUT_PADDING_BITS
    if constexpr (_Might_have_non_value_bits<T>) {
        _Storage_for<T> _Mask{_Form_mask};
        const long _Mask_val = _Atomic_reinterpret_as<long>(_Mask);

        for (;;) {
            _ATOMIC_CHOOSE_INTRINSIC(static_cast<unsigned int>(_Order), _Prev_bytes, _InterlockedCompareExchange,
                ptr, _Desired,
                _Expected_bytes);
            if (_Prev_bytes == _Expected_bytes) {
                return true;
            }

            if ((_Prev_bytes ^ _Expected_bytes) & _Mask_val) {
                ::memcpy(_Expected, &_Prev_bytes, sizeof(long));
                return false;
            }
            _Expected_bytes = (_Expected_bytes & _Mask_val) | (_Prev_bytes & ~_Mask_val);
        }
    }
#endif // _CMPXCHG_MASK_OUT_PADDING_BITS

    _ATOMIC_CHOOSE_INTRINSIC(static_cast<unsigned int>(_Order), _Prev_bytes, _InterlockedCompareExchange,
        ptr, _Desired_bytes, _Expected_bytes);
    if (_Prev_bytes == _Expected_bytes) {
        return true;
    }

    ::memcpy(_Expected, &_Prev_bytes, sizeof(long));
    return false;
}

template<typename T>
static inline bool compare_exchange_strong(long long *ptr, long long *_Expected, const long long *_Desired,
    const std::memory_order _Order = std::memory_order_seq_cst) noexcept { // CAS with given memory order
    long long _Expected_bytes = *_Expected; // read before atomic operation
    const long long _Desired_bytes = *_Desired;
    long long _Prev_bytes;

#if _CMPXCHG_MASK_OUT_PADDING_BITS
    if constexpr (_Might_have_non_value_bits<T>) {
        _Storage_for<T> _Mask{_Form_mask};
        const long long _Mask_val = _Atomic_reinterpret_as<long long>(_Mask);

        for (;;) {
            _ATOMIC_CHOOSE_INTRINSIC(static_cast<unsigned int>(_Order), _Prev_bytes, _InterlockedCompareExchange64,
                ptr, _Desired,
                _Expected_bytes);
            if (_Prev_bytes == _Expected_bytes) {
                return true;
            }

            if ((_Prev_bytes ^ _Expected_bytes) & _Mask_val) {
                ::memcpy(_Expected, &_Prev_bytes, sizeof(long long));
                return false;
            }
            _Expected_bytes = (_Expected_bytes & _Mask_val) | (_Prev_bytes & ~_Mask_val);
        }
    }
#endif // _CMPXCHG_MASK_OUT_PADDING_BITS

    _ATOMIC_CHOOSE_INTRINSIC(static_cast<unsigned int>(_Order), _Prev_bytes, _InterlockedCompareExchange64,
        ptr, _Desired_bytes, _Expected_bytes);
    if (_Prev_bytes == _Expected_bytes) {
        return true;
    }

    ::memcpy(_Expected, &_Prev_bytes, sizeof(long long));
    return false;
}

template<typename T>
static inline bool compare_exchange_strong(Int128 *ptr, Int128 *_Expected, const Int128 *_Desired,
    const std::memory_order _Order = std::memory_order_seq_cst) noexcept { // CAS with given memory order
    Int128 _Desired_bytes{};
    ::memcpy(&_Desired_bytes, _Desired, sizeof(Int128));
    Int128 _Expected_temp{};
    ::memcpy(&_Expected_temp, _Expected, sizeof(Int128));
    unsigned char _Result;
#if _CMPXCHG_MASK_OUT_PADDING_BITS
    if constexpr (_Might_have_non_value_bits<T>) {
        _Int128 _Expected_originally{};
        ::memcpy(&_Expected_originally, _Expected, sizeof(Int128));

        _Storage_for<T> _Mask{_Form_mask};
        _Int128 _Mask_val{};
        ::memcpy(&_Mask_val, _Mask._Ptr(), sizeof(T));
        for (;;) {
#if defined(_M_ARM64) || defined(_M_ARM64EC)
            _ATOMIC_CHOOSE_INTRINSIC(static_cast<unsigned int>(_Order), _Result, _InterlockedCompareExchange128,
                reinterpret_cast<long long>(ptr), _Desired_bytes._High, _Desired_bytes._Low,
                &_Expected_temp._Low);
#else // ^^^ _M_ARM64, _M_ARM64EC / _M_X64 vvv
            (void) _Order;
            _Result = _InterlockedCompareExchange128(&reinterpret_cast<long long&>(ptr), _Desired_bytes._High,
                _Desired_bytes._Low, &_Expected_temp._Low);
#endif // ^^^ _M_X64 ^^^
            if (_Result) {
                return true;
            }

            if (((_Expected_temp._Low ^ _Expected_originally._Low) & _Mask_val._Low) != 0
                || ((_Expected_temp._High ^ _Expected_originally._High) & _Mask_val._High) != 0) {
                ::memcpy(_Expected, &_Expected_temp, sizeof(Int128));
                return false;
            }

            _Expected_temp._Low =
                (_Expected_originally._Low & _Mask_val._Low) | (_Expected_temp._Low & ~_Mask_val._Low);
            _Expected_temp._High =
                (_Expected_originally._High & _Mask_val._High) | (_Expected_temp._High & ~_Mask_val._High);
        }
    }
#endif // _CMPXCHG_MASK_OUT_PADDING_BITS
#if defined(_M_ARM64) || defined(_M_ARM64EC)
    _ATOMIC_CHOOSE_INTRINSIC(static_cast<unsigned int>(_Order), _Result, _InterlockedCompareExchange128,
        reinterpret_cast<long long>(ptr), _Desired_bytes._High, _Desired_bytes._Low, &_Expected_temp._Low);
#else // ^^^ _M_ARM64, _M_ARM64EC / _M_X64 vvv
    (void) _Order;
    _Result = _InterlockedCompareExchange128(
        &reinterpret_cast<long long&>(ptr), _Desired_bytes._High, _Desired_bytes._Low, &_Expected_temp._Low);
#endif // ^^^ _M_X64 ^^^
    if (_Result == 0) {
        ::memcpy(_Expected, &_Expected_temp, Int128);
    }

    return _Result != 0;
}

// Remove the defines (MS STL's std::atomic does the same)
#undef _CMPXCHG_MASK_OUT_PADDING_BITS

#undef _ATOMIC_CHOOSE_INTRINSIC
#undef _ATOMIC_POST_LOAD_BARRIER_AS_NEEDED
#undef _ATOMIC_STORE_PREFIX
#undef _ATOMIC_STORE_SEQ_CST_ARM
#undef _ATOMIC_STORE_SEQ_CST_X86_X64
#undef _ATOMIC_STORE_32_SEQ_CST_X86_X64
#undef _ATOMIC_STORE_SEQ_CST
#undef _ATOMIC_STORE_32_SEQ_CST
#undef _ATOMIC_STORE_64_SEQ_CST
#undef _ATOMIC_STORE_64_SEQ_CST_IX86
#undef _ATOMIC_STORE_SEQ_CST_ARM64
#undef __LOAD_ACQUIRE_ARM64
#undef _ATOMIC_LOAD_ARM64
#undef __STORE_RELEASE
#undef _STD_ATOMIC_USE_ARM64_LDAR_STLR

#undef _INVALID_MEMORY_ORDER

#endif // #ifdef _MSC_VER

#ifdef _MSC_VER

static constexpr bool __atomic_always_lock_free(size_t size, [[maybe_unused]] void *ptr)
{
    return size <= 2 * sizeof(void*);
}

static bool __atomic_is_lock_free(size_t size, [[maybe_unused]] void *ptr) {
    return size <= 2 * sizeof(void*);
}

template<typename T>
static void __atomic_store(T *ptr, T *t, int m) noexcept
{
    if constexpr(sizeof(T) == 1) {
        af_detail::store(reinterpret_cast<char *>(ptr), reinterpret_cast<const char *>(t), static_cast<std::memory_order>(m));
    } else
    if constexpr(sizeof(T) == 2) {
        af_detail::store(reinterpret_cast<short *>(ptr), reinterpret_cast<const short *>(t), static_cast<std::memory_order>(m));
    } else
    if constexpr(sizeof(T) == 4) {
        af_detail::store(reinterpret_cast<int *>(ptr), reinterpret_cast<const int *>(t), static_cast<std::memory_order>(m));
    } else
    if constexpr(sizeof(T) == 8) {
        af_detail::store(reinterpret_cast<long long *>(ptr), reinterpret_cast<const long long *>(t), static_cast<std::memory_order>(m));
    } else
    if constexpr(sizeof(T) == 16) {
        af_detail::store((af_detail::Int128 *)(ptr), (const af_detail::Int128 *)(t), static_cast<std::memory_order>(m));
    }
}

template<typename T>
static void __atomic_load(T const *ptr, T *ret, int m) noexcept
{
    if constexpr(sizeof(T) == 1) {
        char _ret = af_detail::load(reinterpret_cast<const char *>(ptr), static_cast<std::memory_order>(m));
        ::memcpy(ret, &_ret, 1);
    } else
    if constexpr(sizeof(T) == 2) {
        short _ret = af_detail::load(reinterpret_cast<const short *>(ptr), static_cast<std::memory_order>(m));
        ::memcpy(ret, &_ret, 2);
    } else
    if constexpr(sizeof(T) == 4) {
        int _ret = af_detail::load(reinterpret_cast<const int *>(ptr), static_cast<std::memory_order>(m));
        ::memcpy(ret, &_ret, 4);
    } else
    if constexpr(sizeof(T) == 8) {
        long long _ret = af_detail::load(reinterpret_cast<const long long *>(ptr), static_cast<std::memory_order>(m));
        ::memcpy(ret, &_ret, 8);
    } else
    if constexpr(sizeof(T) == 16) {
        af_detail::Int128 _ret = af_detail::load((af_detail::Int128 * const)(ptr), static_cast<std::memory_order>(m));
        ::memcpy(ret, &_ret, 16);
    }
}

template<typename T>
static void __atomic_exchange(T *ptr, T *desired, T *ret, int m) noexcept
{
    if constexpr(sizeof(T) == 1) {
        char _ret = af_detail::exchange(reinterpret_cast<char *>(ptr), reinterpret_cast<const char *>(desired), static_cast<std::memory_order>(m));
        ::memcpy(desired, &_ret, 1);
    } else
    if constexpr(sizeof(T) == 2) {
        short _ret = af_detail::exchange(reinterpret_cast<short *>(ptr), reinterpret_cast<const short *>(desired), static_cast<std::memory_order>(m));
        ::memcpy(desired, &_ret, 2);
    } else
    if constexpr(sizeof(T) == 4) {
        long _ret = af_detail::exchange(reinterpret_cast<long *>(ptr), reinterpret_cast<const long *>(desired), static_cast<std::memory_order>(m));
        ::memcpy(desired, &_ret, 4);
    } else
    if constexpr(sizeof(T) == 8) {
        long long _ret = af_detail::exchange(reinterpret_cast<long long *>(ptr), reinterpret_cast<const long long *>(desired), static_cast<std::memory_order>(m));
        ::memcpy(desired, &_ret, 8);
    } else
    if constexpr(sizeof(T) == 16) {
        af_detail::Int128 _ret = af_detail::exchange((af_detail::Int128 *)(ptr), (const af_detail::Int128 *)(desired), static_cast<std::memory_order>(m));
        ::memcpy(desired, &_ret, 16);
    }
}

template<typename T>
static bool __atomic_compare_exchange(T *ptr,
                                      T *expected,
                                      T *desired,
                                      [[maybe_unused]] bool weak, // MSVC's std::atomic just pipes 'weak' into 'strong'
                                      int success,
                                      int failure) noexcept
{
    if constexpr(sizeof(T) == 1) {
        return af_detail::compare_exchange_strong<T>(reinterpret_cast<char *>(ptr),
                                                     reinterpret_cast<char *>(expected),
                                                     reinterpret_cast<const char *>(desired),
                                                     _Combine_cas_memory_orders((std::memory_order)success,
                                                                                    (std::memory_order)failure));
    } else if constexpr(sizeof(T) == 2) {
        return af_detail::compare_exchange_strong<T>(reinterpret_cast<short *>(ptr),
                                                     reinterpret_cast<short *>(expected),
                                                     reinterpret_cast<const short *>(desired),
                                                     _Combine_cas_memory_orders((std::memory_order)success,
                                                                                    (std::memory_order)failure));
    } else if constexpr(sizeof(T) == 4) {
        return af_detail::compare_exchange_strong<T>(reinterpret_cast<long *>(ptr),
                                                     reinterpret_cast<long *>(expected),
                                                     reinterpret_cast<const long *>(desired),
                                                     _Combine_cas_memory_orders((std::memory_order)success,
                                                                                    (std::memory_order)failure));
    } else if constexpr(sizeof(T) == 8) {
        return af_detail::compare_exchange_strong<T>(reinterpret_cast<long long *>(ptr),
                                                     reinterpret_cast<long long *>(expected),
                                                     reinterpret_cast<const long long *>(desired),
                                                     _Combine_cas_memory_orders((std::memory_order)success,
                                                                                (std::memory_order)failure));
    } else if constexpr(sizeof(T) == 16) {
        return af_detail::compare_exchange_strong<T>((af_detail::Int128 *)(ptr),
                                                     (af_detail::Int128 *)(expected),
                                                     (const af_detail::Int128 *)(desired),
                                                     _Combine_cas_memory_orders((std::memory_order)success,
                                                                                (std::memory_order)failure));
    }
    return false;
}

#endif // #ifdef _MSC_VER

// In compare_exchange overloads where only one memory order is given, we have
// to decide on the other. These are what is laid out by the standard.
constexpr std::memory_order compare_exchange_duo(std::memory_order in) noexcept
{
    constexpr int mapping[6] = {
            /* std::memory_order_relaxed -> */ static_cast<int>(std::memory_order_relaxed),
            /* std::memory_order_consume -> */ static_cast<int>(std::memory_order_consume),
            /* std::memory_order_acquire -> */ static_cast<int>(std::memory_order_acquire),
            /* std::memory_order_release -> */ static_cast<int>(std::memory_order_relaxed),
            /* std::memory_order_acq_rel -> */ static_cast<int>(std::memory_order_acquire),
            /* std::memory_order_seq_cst -> */ static_cast<int>(std::memory_order_seq_cst)

    };
    return static_cast<std::memory_order>(mapping[static_cast<int>(in)]);
}

template <typename T>
class atomic_fp
{
    static_assert(std::is_floating_point<T>::value, "Only for floating point types");
    static constexpr std::size_t k_alignment = alignof(T);

    template <typename U>
    using NV = std::remove_volatile_t<U>;

public:
    using value_type = T;
    using difference_type = value_type;

    static constexpr bool is_always_lock_free = __atomic_always_lock_free(sizeof(T), 0);

    atomic_fp() noexcept = default;
    constexpr atomic_fp(T t) : m_fp(t) { }

    atomic_fp(const atomic_fp&) = delete;
    atomic_fp& operator=(const atomic_fp&) = delete;
    atomic_fp& operator=(const atomic_fp&) volatile = delete;

    T operator=(T t) noexcept
    {
        this->store(t);
        return t;
    }

    T operator=(T t) volatile noexcept
    {
        this->store(t);
        return t;
    }

    bool is_lock_free() const noexcept
    {
        return __atomic_is_lock_free(sizeof(T), 0);
    }

    bool is_lock_free() const volatile noexcept
    {
        return __atomic_is_lock_free(sizeof(T), 0);
    }

    void store(T t, std::memory_order m = std::memory_order_seq_cst) noexcept
    {
        do_store(std::addressof(m_fp), t, m);
    }

    void store(T t, std::memory_order m = std::memory_order_seq_cst) volatile noexcept
    {
        do_store(std::addressof(m_fp), t, m);
    }

    T load(std::memory_order m = std::memory_order_seq_cst) const noexcept
    {
        return do_load(std::addressof(m_fp), m);
    }

    T load(std::memory_order m = std::memory_order_seq_cst) volatile const noexcept
    {
        return do_load(std::addressof(m_fp), m);
    }

    operator T() const noexcept
    {
        return this->load();
    }

    operator T() volatile const noexcept
    {
        return this->load();
    }

    T exchange(T desired, std::memory_order m = std::memory_order_seq_cst) noexcept
    {
        return do_exchange(std::addressof(m_fp), desired, m);
    }

    T exchange(T desired, std::memory_order m = std::memory_order_seq_cst) volatile noexcept
    {
        return do_exchange(std::addressof(m_fp), desired, m);
    }

    bool compare_exchange_weak(T& expected, T desired, std::memory_order success, std::memory_order failure) noexcept
    {
        return do_compare_exchange_weak(std::addressof(m_fp), expected, desired, success, failure);
    }

    bool compare_exchange_weak(T& expected, T desired, std::memory_order success, std::memory_order failure) volatile noexcept
    {
        return do_compare_exchange_weak(std::addressof(m_fp), expected, desired, success, failure);
    }

    bool compare_exchange_strong(T& expected, T desired, std::memory_order success, std::memory_order failure) noexcept
    {
        return do_compare_exchange_strong(std::addressof(m_fp), expected, desired, success, failure);
    }

    bool compare_exchange_strong(T& expected, T desired, std::memory_order success, std::memory_order failure) volatile noexcept
    {
        return do_compare_exchange_strong(std::addressof(m_fp), expected, desired, success, failure);
    }

    bool compare_exchange_weak(T& expected, T desired, std::memory_order order = std::memory_order_seq_cst) noexcept
    {
        return do_compare_exchange_weak(std::addressof(m_fp), expected, desired, order, compare_exchange_duo(order));
    }

    bool compare_exchange_weak(T& expected, T desired, std::memory_order order = std::memory_order_seq_cst) volatile noexcept
    {
        return do_compare_exchange_weak(std::addressof(m_fp), expected, desired, order, compare_exchange_duo(order));
    }

    bool compare_exchange_strong(T& expected, T desired, std::memory_order order = std::memory_order_seq_cst) noexcept
    {
        return do_compare_exchange_strong(std::addressof(m_fp), expected, desired, order, compare_exchange_duo(order));
    }

    bool compare_exchange_strong(T& expected, T desired, std::memory_order order = std::memory_order_seq_cst) volatile noexcept
    {
        return do_compare_exchange_strong(std::addressof(m_fp), expected, desired, order, compare_exchange_duo(order));
    }

    T fetch_add(T i, std::memory_order m = std::memory_order_seq_cst) noexcept
    {
        return do_fetch_add(std::addressof(m_fp), i, m);
    }

    T fetch_add(T i, std::memory_order m = std::memory_order_seq_cst) volatile noexcept
    {
        return do_fetch_add(std::addressof(m_fp), i, m);
    }

    T fetch_sub(T i, std::memory_order m = std::memory_order_seq_cst) noexcept
    {
        return do_fetch_sub(std::addressof(m_fp), i, m);
    }

    T fetch_sub(T i, std::memory_order m = std::memory_order_seq_cst) volatile noexcept
    {
        return do_fetch_sub(std::addressof(m_fp), i, m);
    }

    T operator+=(T i) noexcept
    {
        return do_add_fetch(std::addressof(m_fp), i);
    }

    T operator+=(T i) volatile noexcept
    {
        return do_add_fetch(std::addressof(m_fp), i);
    }

    T operator-=(T i) noexcept
    {
        return do_sub_fetch(std::addressof(m_fp), i);
    }

    T operator-=(T i) volatile noexcept
    {
        return do_sub_fetch(std::addressof(m_fp), i);
    }

protected:
    // This is meant to be used as a base class in a non-virtual way. Disable
    // instantiating objects of this type.
    ~atomic_fp() noexcept = default;

private:
    template <typename U>
    static void do_store(U* ptr, NV<U> t, std::memory_order m) noexcept
    {
        __atomic_store(ptr, std::addressof(t), static_cast<int>(m));
    }

    template <typename U>
    static U do_load(const U* ptr, std::memory_order m) noexcept
    {
        alignas(U) unsigned char buf[sizeof(U)];
        auto* const dest = reinterpret_cast<NV<U>*>(buf);
        __atomic_load(ptr, dest, static_cast<int>(m));
        return *dest;
    }

    template <typename U>
    static NV<U> do_exchange(U* ptr, NV<U> desired, std::memory_order m) noexcept
    {
        alignas(U) unsigned char buf[sizeof(U)];
        auto* const dest = reinterpret_cast<NV<U>*>(buf);
        __atomic_exchage(ptr, std::addressof(desired), dest, static_cast<int>(m));
        return *dest;
    }

    template <typename U>
    static bool do_compare_exchange_weak(U* ptr,
                                         NV<U>& expected,
                                         NV<U>& desired,
                                         std::memory_order success,
                                         std::memory_order failure) noexcept
    {
        return __atomic_compare_exchange(ptr,
                                         std::addressof(expected),
                                         std::addressof(desired),
                                         true,
                                         static_cast<int>(success),
                                         static_cast<int>(failure));
    }

    template <typename U>
    static bool do_compare_exchange_strong(U* ptr,
                                           NV<U>& expected,
                                           NV<U>& desired,
                                           std::memory_order success,
                                           std::memory_order failure) noexcept
    {
        return __atomic_compare_exchange(ptr,
                                         std::addressof(expected),
                                         std::addressof(desired),
                                         false,
                                         static_cast<int>(success),
                                         static_cast<int>(failure));
    }

    template <typename U>
    static U do_fetch_add(U* ptr, NV<U> i, std::memory_order m = std::memory_order_seq_cst) noexcept
    {
        NV<U> old_val = do_load(ptr, std::memory_order_relaxed);
        NV<U> new_val = old_val + i;
        while (!do_compare_exchange_weak(ptr, old_val, new_val, m, std::memory_order_relaxed)) {
            new_val = old_val + i;
        }
        return old_val;
    }

    template <typename U>
    static U do_add_fetch(U* ptr, NV<U> i) noexcept
    {
        NV<U> old_val = do_load(ptr, std::memory_order_relaxed);
        NV<U> new_val = old_val + i;
        while (!do_compare_exchange_weak(ptr, old_val, new_val, std::memory_order_seq_cst, std::memory_order_relaxed)) {
            new_val = old_val + i;
        }
        return new_val;
    }

    template <typename U>
    static U do_fetch_sub(U* ptr, NV<U> i, std::memory_order m = std::memory_order_seq_cst) noexcept
    {
        NV<U> old_val = do_load(ptr, std::memory_order_relaxed);
        NV<U> new_val = old_val - i;
        while (!do_compare_exchange_weak(ptr, old_val, new_val, m, std::memory_order_relaxed)) {
            new_val = old_val - i;
        }
        return old_val;
    }

    template <typename U>
    static U do_sub_fetch(U* ptr, NV<U> i) noexcept
    {
        NV<U> old_val = do_load(ptr, std::memory_order_relaxed);
        NV<U> new_val = old_val - i;
        while (!do_compare_exchange_weak(ptr, old_val, new_val, std::memory_order_seq_cst, std::memory_order_relaxed)) {
            new_val = old_val - i;
        }
        return new_val;
    }

    alignas(k_alignment) T m_fp;
};

} // namespace af_detail

namespace std {

template <>
class atomic<float> : public af_detail::atomic_fp<float>
{
public:
    atomic() noexcept = default;
    atomic(float desired) noexcept : af_detail::atomic_fp<float>(desired) { }

    atomic& operator=(const atomic&) = delete;
    atomic& operator=(const atomic&) volatile = delete;

    using af_detail::atomic_fp<float>::operator=;
};

template <>
class atomic<double> : public af_detail::atomic_fp<double>
{
public:
    atomic() noexcept = default;
    atomic(double desired) noexcept : af_detail::atomic_fp<double>(desired) { }

    atomic& operator=(const atomic&) = delete;
    atomic& operator=(const atomic&) volatile = delete;

    using af_detail::atomic_fp<double>::operator=;
};

template <>
class atomic<long double> : public af_detail::atomic_fp<long double>
//
// This is a 128-bit long double atomic operation class.
// Under some of the OS versions, a 128-bit lock-free atomic operation is not supported. (is_lock_free()
// always returns false). In this case, this class will not work as expected in that environment. (It
// is still atomic but not lock-free).
// However, at this moment, we don't use atomic<long double> objects in MoonRay and so this may not be
// a concern. However, in the future if we need long double atomic lock-free operation under an environment
// that doesn't support it, we should consider using the Atomic128.h implementation.
//
{
public:
    atomic() noexcept = default;
    atomic(long double desired) noexcept : af_detail::atomic_fp<long double>(desired) { }

    atomic& operator=(const atomic&) = delete;
    atomic& operator=(const atomic&) volatile = delete;

    using af_detail::atomic_fp<long double>::operator=;
};

// C++ standard requirements.
static_assert(std::is_standard_layout<atomic<float>>::value);
static_assert(std::is_standard_layout<atomic<double>>::value);
static_assert(std::is_standard_layout<atomic<long double>>::value);
static_assert(std::is_trivially_destructible<atomic<float>>::value);
static_assert(std::is_trivially_destructible<atomic<double>>::value);
static_assert(std::is_trivially_destructible<atomic<long double>>::value);

} // namespace std
#endif // #if !defined(__cpp_lib_atomic_float)
