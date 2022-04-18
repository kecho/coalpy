#pragma once

#ifdef _WIN32
#define ENABLE_DX12 1
#define ENABLE_VULKAN 0
#define ENABLE_SDL_VULKAN 0
#endif

#ifdef __linux__
#define ENABLE_DX12 0
#define ENABLE_VULKAN 1
#define ENABLE_SDL_VULKAN 1
#endif

#ifndef ENABLE_DX12
#define ENABLE_DX12 0
#endif

#ifndef ENABLE_VULKAN
#define ENABLE_VULKAN 0
#endif

#ifndef ENABLE_SDL_VULKAN
#define ENABLE_SDL_VULKAN
#endif


#define ENABLE_RENDER_RESOURCE_NAMES 1
#define DX_RET(x) __uuidof(x), (void**)&x
#define DX_OK(x) { HRESULT _r = x; CPY_ASSERT(_r == S_OK); }
#define VK_OK(x) { VkResult _r = x; CPY_ASSERT(_r == VK_SUCCESS); }

