#include <iostream>
#include <coalpy.core/Assert.h>
#include <coalpy.core/ClParser.h>
#include <coalpy.core/ClTokenizer.h>
#include <string>
#include <set>
#include <atomic>
#include "testsystem.h"

//hack bring definition of assert into this unit for release builds
#ifndef _DEBUG
#include "../core/Assert.cpp"
#endif

using namespace coalpy;

typedef TestSuite*(*CreateSuiteFn)();

namespace coalpy
{

TestSuite* fileSystemSuite();

}

CreateSuiteFn g_suites[] = {
    fileSystemSuite
};


std::atomic<int> g_errors = 0;
void assertHandler(const char* condition, const char* fileStr, int line, const char* msg)
{
    printf("%s:%d %s %s \n", fileStr, line, condition, msg ? msg :"");
    ++g_errors;
}

struct Filters
{
    std::set<std::string> suites;
    std::set<std::string> cases;
} gFilters;

void runSuite(CreateSuiteFn createFn)
{
    g_errors = 0;
    TestSuite* suite = createFn();
    if (!gFilters.suites.empty() && gFilters.suites.find(suite->name()) == gFilters.suites.end())
        return;

    printf("[Suite: %s]\n", suite->name());

    TestContext* context = suite->createContext();
    int caseCount = 0;
    const TestCase* cases = suite->getCases(caseCount);
    for (int i = 0; i < caseCount; ++i)
    {
        const TestCase& caseData = cases[i];
        if (!gFilters.cases.empty() && gFilters.cases.find(caseData.name) == gFilters.cases.end())
            continue;

        int prevErr = g_errors;
        caseData.fn(*context);
        int finalErrors = g_errors;
        bool success = (prevErr == finalErrors);
        printf("Case %d: %s - %s\n", i, caseData.name, success ? "PASS" : "FAIL");
    }
    suite->destroyContext(context);
    g_errors = 0;
    delete suite;
}

struct ArgParameters
{
    bool help = false;
    bool printTests = false;
    const char* suitefilter = "";
    const char* testfilter = "";
};

bool prepareCli(ClParser& p, ArgParameters& params)
{
    ClParser::GroupId gid= p.createGroup("General", "General Params:");
    p.bind(gid, &params);
    CliSwitch(gid, "help", "h", "help", Bool, ArgParameters, help);
    CliSwitch(gid, "print available suites and tests", "p", "printtests", Bool, ArgParameters, printTests);
    CliSwitch(gid, "Comma separated suite filters", "s", "suites", String, ArgParameters, suitefilter);
    CliSwitch(gid, "Comma separated test case filters", "t", "tests", String, ArgParameters, testfilter);
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
            printf("\n", suite->name());
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

    AssertSystem::setAssertHandler(assertHandler);
    for (int i = 0; i < suiteCounts; ++i)
    {
        runSuite(g_suites[i]);
    }
    return 0;
}
