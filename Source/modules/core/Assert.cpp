#include <coalpy.core/Assert.h>

#if CPY_ASSERT_ENABLED

#include <iostream>
#include <stdio.h>
#include <stdarg.h>
#ifdef _WIN32 
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <debugapi.h>
#else
#include <csignal>
#endif

namespace coalpy
{

namespace 
{

enum 
{
    ErrorStringBufferSize = 256
};

void defaultAssertFn(const char* condition, const char* fileStr, int line, const char* msg)
{
    std::cerr << "Assert [" << fileStr << ":" << line << "] " << std::endl;
    std::cerr << "cond :" << condition << std::endl;
    if (msg)
        std::cerr << "msg  :" << msg << std::endl;
}

void defaultErrorFn(const char* condition, const char* fileStr, int line, const char* msg)
{
    std::cerr << "ERROR [" << fileStr << ":" << line << "] " << std::endl;
    std::cerr << "cond :" << condition << std::endl;
    if (msg)
        std::cerr << "msg  :" << msg << std::endl;
}

AssertSystem::HandlerFn s_assertHandler = defaultAssertFn;
AssertSystem::HandlerFn s_errorHandler  = defaultErrorFn;

}

void AssertSystem::setAssertHandler(AssertSystem::HandlerFn fn)
{
    s_assertHandler = fn ? fn : defaultAssertFn;
}

void AssertSystem::setErrorHandler(AssertSystem::HandlerFn fn)
{
    s_assertHandler = fn ? fn : defaultErrorFn;
}

void AssertSystem::onAssert(const char* condition, const char* file, unsigned int line, const char* fmt, ...)
{
    if (!s_assertHandler)
        return;

    char buffer[(int)ErrorStringBufferSize];
    if (fmt)
    {
        va_list args;
        va_start(args, fmt);
        vsnprintf(buffer, ErrorStringBufferSize, fmt, args);
        va_end(args);
    }

    s_assertHandler(condition, file, line, fmt ? buffer : nullptr);

#ifdef _WIN32 
    __try
    {
        DebugBreak();
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {

    }
#else
    std::raise(SIGINT);
#endif

}

void AssertSystem::onError(const char* condition, const char* file, unsigned int line, const char* fmt, ...)
{
    if (!s_errorHandler)
        return;

    char buffer[(int)ErrorStringBufferSize];
    if (fmt)
    {
        va_list args;
        va_start(args, fmt);
        vsnprintf(buffer, ErrorStringBufferSize, fmt, args);
        va_end(args);
    }

    s_errorHandler(condition, file, line, fmt ? buffer : nullptr);

    static volatile int* crashMe = 0x0;
    *crashMe = 0x0;
}

}

#endif
