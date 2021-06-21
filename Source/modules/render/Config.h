#pragma once

#ifdef _WIN32

#define ENABLE_DX12 1

#else

#define ENABLE_DX12 0

#endif

#define ENABLE_RENDER_RESOURCE_NAMES 1
#define DX_RET(x) __uuidof(x), (void**)&x
#define DX_OK(x) { HRESULT _r = x; CPY_ASSERT(_r == S_OK) }

