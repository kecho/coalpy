#pragma once

#if _DEBUG
#define C_ASSERT_ENABLED 1
#else
#define C_ASSERT_ENABLED 0
#endif

#if C_ASSERT_ENABLED

#define C_ASSERT(cond)               { if (cond) AssertSystem::assert(#cond, __FILE__, __LINE__, nullptr); }
#define C_ASSERT_MSG(cond, msg)      { if (cond) AssertSystem::assert(#cond, __FILE__, __LINE__, "%s", msg); }
#define C_ASSERT_FMT(cond, fmt, ...) { if (cond) AssertSystem::assert(#cond, __FILE__, __LINE__, fmt, __VA_ARGS__); }
#define C_ERROR(cond)                { if (cond) AssertSystem::error (#cond, __FILE__, __LINE__, nullptr); }
#define C_ERROR_MSG(cond, msg)       { if (cond) AssertSystem::error (#cond, __FILE__, __LINE__, "%s", msg); }
#define C_ERROR_FMT(cond, fmt, ...)  { if (cond) AssertSystem::error (#cond, __FILE__, __LINE__, fmt, __VA_ARGS__); }

namespace coalpy
{

struct AssertSystem
{
public:
    typedef void(*HandlerFn)(const char* condition, const char* fileStr, int line, const char* msg);

    static void setAssertHandler(HandlerFn fn);
    static void setErrorHandler(HandlerFn fn);
    static void assert(const char* condition, const char* file, unsigned int line, const char* fmt, ...);
    static void error (const char* condition, const char* file, unsigned int line, const char* fmt, ...);
};

}

#else

#define C_ASSERT(cond)
#define C_ASSERT_MSG(cond, msg)
#define C_ASSERT_FMT(cond, fmt, ...)
#define C_ERROR(cond)
#define C_ERROR_MSG(cond, msg)
#define C_ERROR_FMT(cond, fmt, ...)

#endif
