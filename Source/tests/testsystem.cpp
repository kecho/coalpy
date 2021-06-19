#include "testsystem.h"
#include <coalpy.files/Utils.h>

namespace coalpy
{

static ApplicationContext g_ctx = {};

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
    std::string resourceDir = cachedRootDir + (cachedRootDir == "" ? ".\\" : "\\") + "coalpy\\resources\\"; 
    return resourceDir;
}

}
