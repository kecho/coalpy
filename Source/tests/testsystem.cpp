#include "testsystem.h"
#include <coalpy.core/ClTokenizer.h>
#include <coalpy.core/BitMask.h>
#include <coalpy.files/Utils.h>

#if defined(_WIN32)
#define DEFAULT_PLATFORMS (TestPlatforms)(TestPlatformDx12 | TestPlatformVulkan)
#elif defined (__linux__)
#define DEFAULT_PLATFORMS TestPlatformVulkan
#else
#define DEFAULT_PLATFORM
#endif

namespace coalpy
{

bool parseTestPlatforms(const std::string& arg, TestPlatforms& platforms)
{
    std::vector<std::string> tokens = ClTokenizer::splitString(arg, ',');
    if (tokens.empty())
    {
        platforms = DEFAULT_PLATFORMS;
        return true;
    }

    platforms = {};
    for (auto& token : tokens)
    {
        if (token == "dx12")
            platforms = (TestPlatforms)(platforms | TestPlatformDx12);
        else if (token == "vulkan")
            platforms = (TestPlatforms)(platforms | TestPlatformVulkan);
        else
            return false;
    }
    return true;
}

render::DevicePlat nextPlatform(TestPlatforms& platforms)
{
    unsigned mask = ~((unsigned)platforms - 1u);
    unsigned bit = popCnt((platforms & mask) - 1u);
    platforms = (TestPlatforms)(platforms ^ (1u << bit));
    return (render::DevicePlat)bit;
}

static ApplicationContext g_ctx = {};

#if defined(_WIN32)
    #define SEP "\\"
#elif defined(__linux__)
    #define SEP "/"
#else
    #error "Platform not supported"
#endif

const ApplicationContext& ApplicationContext::get()
{
    return g_ctx;
}

void ApplicationContext::set(const ApplicationContext& ctx)
{
    g_ctx = ctx;
}

std::string ApplicationContext::rootDir() const
{
    std::string appName = argv[0];
    std::string rootDir;
    FileUtils::getDirName(appName, rootDir);
    return rootDir;
}

std::string ApplicationContext::resourceRootDir() const
{
    std::string cachedRootDir = rootDir();
    std::string resourceDir = cachedRootDir + (cachedRootDir == "" ? "." SEP : SEP) + "coalpy" + SEP + "resources" + SEP; 
    return resourceDir;
}

}
