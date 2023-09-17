#pragma once

#include <string>
#include <coalpy.render/IDevice.h>

namespace coalpy
{

enum TestPlatforms : unsigned
{
    TestPlatformDx12 = 1 << (unsigned)render::DevicePlat::Dx12,
    TestPlatformVulkan = 1 << (unsigned)render::DevicePlat::Vulkan,
    TestPlatformMetal = 1 << (unsigned)render::DevicePlat::Metal
};

bool parseTestPlatforms(const std::string& arg, TestPlatforms& outPlatforms);
render::DevicePlat nextPlatform(TestPlatforms& platforms);

class TestContext
{
public:
    TestContext() {}
    virtual ~TestContext() {}
};

typedef void(*TestFn)(TestContext& ctx);

struct TestCase
{
    const char* name;
    TestFn fn;
};

typedef TestContext* (*CreateContextFn)();
typedef void (*DestroyContextFn)(TestContext*);

struct TestCaseFilter
{
    const char* caseName;
    TestPlatforms supportedPlatforms;
};

struct TestSuiteDesc
{
    const char* name = {};
    const TestCase* cases = {};
    int casesCount = 0;
    const TestCaseFilter* filters = {};
    int filterCounts = 0;
    CreateContextFn createContextFn = {};
    DestroyContextFn destroyContextFn = {};
    TestPlatforms supportedRenderPlatforms = {};
};

struct ApplicationContext
{
    int argc;
    char** argv;
    render::DevicePlat graphicsApi;
    
    static const ApplicationContext& get();
    static void set(const ApplicationContext& ctx);

    std::string rootDir() const;
    std::string resourceRootDir() const;
};

}
