#include "testsystem.h"
#include <coalpy.core/Assert.h>
#include <coalpy.core/ByteBuffer.h>

namespace coalpy
{


void testByteBuffer(TestContext& ctx)
{
    ByteBuffer buffer;
    int intArray[] = { 1, 2, 3, 4 };
    buffer.append((const u8*)intArray, sizeof(int)*4); 

    CPY_ASSERT(buffer.size() == sizeof(int)*4);
    for (int grow = 0; grow < 10; ++grow)
    {
        buffer.reserve(grow * sizeof(int) * 4);
        const int* cpy = (const int*)buffer.data();
        for (int i = 0; i < 4; ++i)
        {
            CPY_ASSERT(intArray[i] == cpy[i]);
        }
    }
}


class CoreTestSuite : public TestSuite
{
public:
    virtual const char* name() const { return "core"; }
    virtual const TestCase* getCases(int& caseCounts) const
    {
        static TestCase sCases[] = {
            { "byteBuffer", testByteBuffer }
        };

        caseCounts = (int)(sizeof(sCases) / sizeof(TestCase));
        return sCases;
    }
};


TestSuite* coreSuite()
{
    return new CoreTestSuite;
}

}
