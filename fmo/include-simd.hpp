#ifndef FMO_INCLUDE_SIMD_HPP
#define FMO_INCLUDE_SIMD_HPP

#if defined(_MSC_VER) && !defined(__INTEL_COMPILER)
#   if defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
#       define __SSE2__
#   endif
#endif

#if defined(__SSE2__)
#   include <emmintrin.h>
#   define FMO_HAVE_SSE2
#endif

#if defined(__AVX2__)
#   include <immintrin.h>
#   define FMO_HAVE_AVX2
#endif

#if defined(__ARM_NEON__) || defined(__ARM_NEON)
#   include <arm_neon.h>
#   define FMO_HAVE_NEON
#endif

#endif // FMO_INCLUDE_SIMD_HPP
