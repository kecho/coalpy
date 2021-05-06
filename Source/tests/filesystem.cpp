#include "testsystem.h"
#include <coalpy.core/Assert.h>
#include <iostream>

namespace coalpy
{

void testFileCreate(TestContext& ctx)
{
}

class FileSystemTestSuite : public TestSuite
{
public:
    virtual const char* name() const { return "filesystem"; }
    virtual const TestCase* getCases(int& caseCounts) const
    {
        static TestCase sCases[] = {
            { "fileCreate", testFileCreate },
            { "fileCreate2", testFileCreate }
        };

        caseCounts = (int)(sizeof(sCases) / sizeof(TestCase));
        return sCases;
    }
};

TestSuite* fileSystemSuite()
{
    return new FileSystemTestSuite;
}

}
