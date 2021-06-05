#pragma once

#ifdef _WIN32

#define ENABLE_WIN32_WINDOW 1

#else

#define ENABLE_WIN32_WINDOW 0

#endif

#define WND_OK(x) { BOOL _r = x; CPY_ASSERT(_r) }
