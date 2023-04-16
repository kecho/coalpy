#include <iostream>
#include <coalpy.core/Assert.h>
#include <coalpy.core/ClParser.h>
#include <coalpy.core/ClTokenizer.h>
#include <coalpy.core/Stopwatch.h>
#include <coalpy.core/SmartPtr.h>
#include <coalpy.render/IDevice.h>
#include <coalpy.render/../../Config.h>
#include <string>
#include <string.h>
#include <unordered_map>
#include <set>
#include <atomic>
#include "testsystem.h"

//hack bring definition of assert into this unit for release builds
#ifndef _DEBUG
#include "../modules/core/Assert.cpp"
#endif

using namespace coalpy;

typedef void (*CreateSuiteFn)(TestSuiteDesc&);

namespace coalpy
{

extern void coreSuite(TestSuiteDesc& suite);
extern void fileSystemSuite(TestSuiteDesc& suite);
extern void taskSystemSuite(TestSuiteDesc& suite);
extern void shaderSuite(TestSuiteDesc& suite);
extern void renderSuite(TestSuiteDesc& suite);

}

CreateSuiteFn g_suites[] = {
    coreSuite,
    taskSystemSuite,
    fileSystemSuite,
    shaderSuite,
    renderSuite
};

bool g_enableErrorOutput = true;
int g_totalErrors = 0;
std::atomic<int> g_errors = 0;
void assertHandler(const char* condition, const char* fileStr, int line, const char* msg)
{
    if (g_enableErrorOutput)
        printf("%s:%d %s %s \n", fileStr, line, condition, msg ? msg :"");
    ++g_errors;
}

struct Filters
{
    std::set<std::string> suites;
    std::set<std::string> cases;
} gFilters;

struct SuiteStats
{
    int casesRan = 0;
};

void runSuiteOnPlatform(const TestSuiteDesc& suite, const char* currPlatform, uint32_t platformBit, SuiteStats& stats)
{
    std::unordered_map<std::string, TestPlatforms> platformFilters;
    for (int i = 0; i < suite.filterCounts; ++i)
        platformFilters[std::string(suite.filters[i].caseName)] = suite.filters[i].supportedPlatforms;

    std::vector<const TestCase*> cases;
    for (int i = 0; i < suite.casesCount; ++i)
    {
        const TestCase& caseData = suite.cases[i];
        if (!gFilters.cases.empty() && gFilters.cases.find(caseData.name) == gFilters.cases.end())
            continue;

        auto filterFound = platformFilters.find(caseData.name);
        if (filterFound != platformFilters.end() && (filterFound->second & platformBit) == 0)
            continue;
        cases.push_back(&caseData);
    }

    if (!cases.empty())
        if (currPlatform == nullptr)
            printf("[%s]\n", suite.name);
        else
            printf("[%s][%s]\n", suite.name, currPlatform);

    static TestContext sEmptyContext = {};
    TestContext* context = suite.createContextFn ? suite.createContextFn() : &sEmptyContext;
    int caseCount = 0;
    Stopwatch sw;
    for (int i = 0; i < cases.size(); ++i)
    {
        const TestCase& caseData = *cases[i];
        ++stats.casesRan;
        sw.start();
        int prevErr = g_errors;
        caseData.fn(*context);
        int finalErrors = g_errors;
        bool success = (prevErr == finalErrors);
        float runningTime = (float)sw.timeMicroSeconds() / 1000.0f;
        printf("%d: %s (%.3fms) - %s\n", i, caseData.name, runningTime, success ? "PASS" : "FAIL");
    }
    if (gFilters.cases.empty())
        printf("\n");
    if (suite.destroyContextFn && context != &sEmptyContext)
        suite.destroyContextFn(context);
    g_totalErrors += g_errors;
}

void runSuite(CreateSuiteFn createFn, SuiteStats& stats, const ApplicationContext& ctx, TestPlatforms platforms)
{
    g_errors = 0;
    TestSuiteDesc suite ;
    createFn(suite);
    if (!gFilters.suites.empty() && gFilters.suites.find(suite.name) == gFilters.suites.end())
        return;

    TestPlatforms actualPlatforms = (TestPlatforms)(suite.supportedRenderPlatforms & platforms);
    if (actualPlatforms == 0) //no multi platforms
    {
        ApplicationContext::set(ctx);
        runSuiteOnPlatform(suite, nullptr, 0u, stats);
    }
    else
    {
        while (actualPlatforms)
        {
            render::DevicePlat plat = nextPlatform(actualPlatforms);
            ApplicationContext platformContext = ctx;
            platformContext.graphicsApi = plat;
            ApplicationContext::set(platformContext);
            runSuiteOnPlatform(suite, render::getDevicePlatName(plat), 1u << (int)plat, stats);
        }
    }
    g_errors = 0;
}

struct ArgParameters
{
    bool help = false;
    bool printTests = false;
    bool forever = false;
    bool quietMode = false;
    const char* suitefilter = "";
    const char* testfilter = "";
    const char* graphicsApi = "";
};

bool prepareCli(ClParser& p, ArgParameters& params)
{
    ClParser::GroupId gid= p.createGroup("General", "General Params:");
    p.bind(gid, &params);
    CliSwitch(gid, "help", "h", "help", Bool, ArgParameters, help);
    CliSwitch(gid, "print available suites and tests", "p", "printtests", Bool, ArgParameters, printTests);
    CliSwitch(gid, "quiet mode, only report test outcome. Doesnt print error details", "q", "quiet", Bool, ArgParameters, quietMode);
    CliSwitch(gid, "Comma separated suite filters", "s", "suites", String, ArgParameters, suitefilter);
    CliSwitch(gid, "Comma separated test case filters", "t", "tests", String, ArgParameters, testfilter);
    CliSwitch(gid, "Comma separated graphics apis (dx12, vulkan or default)", "g", "gapi", String, ArgParameters, graphicsApi);
    CliSwitch(gid, "Run indefinitely iterations of the tests. Ideal to stress test things.", "e", "forever", Bool, ArgParameters, forever);
    return true;
}

int main(int argc, char* argv[])
{
    ArgParameters params;
    ClParser p;
    if (!prepareCli(p, params))
    {
        std::cerr << "Error setting up cli parser\n";
        return -1;
    }

    if (!p.parse(argc, argv))
    {
        return -1;
    }

    if (params.help)
    {
        p.prettyPrintHelp();
        return 0;
    }

    g_enableErrorOutput = !params.quietMode;

    #if defined(_WIN32) && ENABLE_DX12
    auto platform = render::DevicePlat::Dx12;
    #elif defined(__linux__) && ENABLE_VULKAN
    auto platform = render::DevicePlat::Vulkan;
    #endif

    TestPlatforms platforms = {};
    if (!parseTestPlatforms(params.graphicsApi, platforms))
    {
        std::cerr << "Valid platforms must be 'dx12', 'vulkan' comma separated" << std::endl;
        return -1;
    }

    int suiteCounts = (int)(sizeof(g_suites)/sizeof(g_suites[0]));
    if (params.printTests)
    {
        
        for (int i = 0; i < suiteCounts; ++i)
        {
            TestSuiteDesc suite;
            g_suites[i](suite);
            printf("%s:\n", suite.name);
            int caseCount = 0;
            for (int j = 0; j < suite.casesCount; ++j)
                printf("   %s\n", suite.cases[j].name);
            printf("\n");
        }
        return 0;
    }

    {
        std::string suiteFilterStr = params.suitefilter;
        auto strList = ClTokenizer::splitString(suiteFilterStr, ',');
        for (auto s : strList)
            gFilters.suites.insert(s);
    }

    {
        std::string caseFilterStr = params.testfilter;
        auto strList = ClTokenizer::splitString(caseFilterStr, ',');
        for (auto s : strList)
            gFilters.cases.insert(s);
    }

    ApplicationContext appCtx;
    appCtx.argc = argc;
    appCtx.argv = argv;
    
    bool keepRunning = true;
    while (keepRunning)
    {
        AssertSystem::setAssertHandler(assertHandler);
        SuiteStats stats;
        for (int i = 0; i < suiteCounts; ++i)
        {
            runSuite(g_suites[i], stats, appCtx, platforms);
        }

        if ((!gFilters.cases.empty() || !gFilters.suites.empty()) && stats.casesRan == 0)
        {
            std::cerr << "No tests ran. Check test filters (-s and -t)." << std::endl;
            ++g_totalErrors;
        }

        std::cout << (g_totalErrors == 0 ? "SUCCESS" : "FAIL") << std::endl;
        
        keepRunning = params.forever;
    }
    if (g_totalErrors > 0)
        std::cerr << "Error detected in run." << std::endl;
    return (g_totalErrors == 0) ? 0 : 1;
}
