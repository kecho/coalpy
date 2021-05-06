#include <iostream>
#include <coalpy.core/Assert.h>
#include <string>
#include <set>
#include <atomic>
#include "testsystem.h"

//hack bring definition of assert into this unit for release builds
#ifndef _DEBUG
#include "../core/Assert.cpp"
#endif

using namespace coalpy;

namespace coalpy
{

TestSuite* fileSystemSuite();

}

std::atomic<int> g_errors = 0;
void assertHandler(const char* condition, const char* fileStr, int line, const char* msg)
{
    printf("%s:%d %s %s \n", fileStr, line, condition, msg ? msg :"");
    ++g_errors;
}


typedef TestSuite*(*CreateSuiteFn)();

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


int main(int argc, char* argv[])
{
    AssertSystem::setAssertHandler(assertHandler);
    runSuite(fileSystemSuite);
    return 0;
}
