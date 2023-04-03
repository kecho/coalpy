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
#include <set>
#include <atomic>
#include "testsystem.h"

//hack bring definition of assert into this unit for release builds
#ifndef _DEBUG
#include "../modules/core/Assert.cpp"
#endif

using namespace coalpy;

typedef TestSuite*(*CreateSuiteFn)();

namespace coalpy
{

extern TestSuite* coreSuite();
extern TestSuite* fileSystemSuite();
extern TestSuite* taskSystemSuite();
extern TestSuite* shaderSuite();
extern TestSuite* renderSuite();

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

void runSuite(CreateSuiteFn createFn, SuiteStats& stats)
{
    g_errors = 0;
    SmartPtr<TestSuite> suite = createFn();
    if (!gFilters.suites.empty() && gFilters.suites.find(suite->name()) == gFilters.suites.end())
        return;

    if (gFilters.cases.empty())
        printf("[%s]\n", suite->name());

    TestContext* context = suite->createContext();
    int caseCount = 0;
    const TestCase* cases = suite->getCases(caseCount);
    Stopwatch sw;
    for (int i = 0; i < caseCount; ++i)
    {
        const TestCase& caseData = cases[i];
        if (!gFilters.cases.empty() && gFilters.cases.find(caseData.name) == gFilters.cases.end())
            continue;

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
    suite->destroyContext(context);
    g_totalErrors += g_errors;
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
    CliSwitch(gid, "Graphics api (dx12, vulkan or default)", "g", "gapi", String, ArgParameters, graphicsApi);
    CliSwitch(gid, "Run indefinitely iterations of the tests. Ideal to stress test things.", "e", "forever", Bool, ArgParameters, forever);
    return true;
}

int main(int argc, char* argv[])
{
    ApplicationContext appCtx;
    appCtx.argc = argc;
    appCtx.argv = argv;
    
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

    if (!strcmp(params.graphicsApi, "dx12"))
        platform = render::DevicePlat::Dx12;
    else if (!strcmp(params.graphicsApi, "vulkan"))
        platform = render::DevicePlat::Vulkan;
    else if (strcmp(params.graphicsApi,"default") && strcmp(params.graphicsApi,""))
    {
        std::cerr << "Undefined graphics api requested." << std::endl;
        return -1;
    }

    appCtx.graphicsApi = platform;

    ApplicationContext::set(appCtx);

    int suiteCounts = (int)(sizeof(g_suites)/sizeof(g_suites[0]));
    if (params.printTests)
    {
        
        for (int i = 0; i < suiteCounts; ++i)
        {
            TestSuite* suite = g_suites[i]();
            printf("%s:\n", suite->name());
            int caseCount = 0;
            const TestCase* cases = suite->getCases(caseCount);
            for (int j = 0; j < caseCount; ++j)
                printf("   %s\n", cases[j].name);
            printf("\n");
            delete suite;
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

    bool keepRunning = true;
    while (keepRunning)
    {
        AssertSystem::setAssertHandler(assertHandler);
        SuiteStats stats;
        for (int i = 0; i < suiteCounts; ++i)
        {
            runSuite(g_suites[i], stats);
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
