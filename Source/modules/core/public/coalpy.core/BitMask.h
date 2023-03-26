#pragma once
#include <stdint.h>

#if defined(_WIN32)
#include <intrin.h>
#elif defined(__linux__)
#include <x86intrin.h>
#endif

namespace coalpy
{

typedef uint64_t BitMask;

inline int popCnt(BitMask mask)
{
#if defined (_WIN32)
    return __popcnt64(mask);
#elif defined(__linux__)
    return _popcnt64(mask);
#endif
}

inline int lzCnt(BitMask mask)
{
    return (int)__lzcnt64(mask);
}

}
