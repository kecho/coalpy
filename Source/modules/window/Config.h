#pragma once

#ifdef _WIN32
#define ENABLE_WIN32_WINDOW 1
#define ENABLE_SDL_WINDOW 0
#endif

#ifdef __linux__
#define ENABLE_WIN32_WINDOW 0
#define ENABLE_SDL_WINDOW 1
#endif

#ifndef ENABLE_WIN32_WINDOW
#define ENABLE_WIN32_WINDOW 0
#endif

#ifndef ENABLE_SDL_WINDOW
#define ENABLE_SDL_WINDOW 0
#endif

#define WND_OK(x) { BOOL _r = x; CPY_ASSERT(_r) }
