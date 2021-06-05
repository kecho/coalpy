#pragma once

#ifndef CPY_ASSERT_ENABLED
#if _DEBUG
#define CPY_ASSERT_ENABLED 1
#else
#define CPY_ASSERT_ENABLED 0
#endif
#endif

#if CPY_ASSERT_ENABLED

#define CPY_ASSERT(cond)               { if (!(cond)) ::coalpy::AssertSystem::onAssert(#cond, __FILE__, __LINE__, nullptr); }
#define CPY_ASSERT_MSG(cond, msg)      { if (!(cond)) ::coalpy::AssertSystem::onAssert(#cond, __FILE__, __LINE__, "%s", msg); }
#define CPY_ASSERT_FMT(cond, fmt, ...) { if (!(cond)) ::coalpy::AssertSystem::onAssert(#cond, __FILE__, __LINE__, fmt, __VA_ARGS__); }
#define CPY_ERROR(cond)                { if (!(cond)) ::coalpy::AssertSystem::onError (#cond, __FILE__, __LINE__, nullptr); }
#define CPY_ERROR_MSG(cond, msg)       { if (!(cond)) ::coalpy::AssertSystem::onError (#cond, __FILE__, __LINE__, "%s", msg); }
#define CPY_ERROR_FMT(cond, fmt, ...)  { if (!(cond)) ::coalpy::AssertSystem::onError (#cond, __FILE__, __LINE__, fmt, __VA_ARGS__); }

namespace coalpy
{

struct AssertSystem
{
public:
    typedef void(*HandlerFn)(const char* condition, const char* fileStr, int line, const char* msg);

    static void setAssertHandler(HandlerFn fn);
    static void setErrorHandler(HandlerFn fn);
    static void onAssert(const char* condition, const char* file, unsigned int line, const char* fmt, ...);
    static void onError (const char* condition, const char* file, unsigned int line, const char* fmt, ...);
};

}

#else

#define CPY_ASSERT(cond)
#define CPY_ASSERT_MSG(cond, msg)
#define CPY_ASSERT_FMT(cond, fmt, ...)
#define CPY_ERROR(cond)
#define CPY_ERROR_MSG(cond, msg)
#define CPY_ERROR_FMT(cond, fmt, ...)

#endif
