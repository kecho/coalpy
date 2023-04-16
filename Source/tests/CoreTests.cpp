#include "testsystem.h"
#include <coalpy.core/Assert.h>
#include <coalpy.core/ByteBuffer.h>
#include <coalpy.core/HashStream.h>

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

void testHashStream(TestContext& ctx)
{
    HashStream hsA;
    int a = 20;
    hsA << a;

    HashStream hsB;
    int b = 21;
    hsB << b;

    HashStream hsC;
    hsC << a << b;

    CPY_ASSERT(hsA.val() != 0);
    CPY_ASSERT(hsB.val() != 0);
    CPY_ASSERT(hsC.val() != 0);
    CPY_ASSERT(hsA.val() != hsB.val());
    CPY_ASSERT(hsB.val() != hsC.val());
    CPY_ASSERT(hsA.val() != hsC.val());
}

static TestCase* createCases(int& caseCounts)
{
    static TestCase sCases[] = {
        { "byteBuffer", testByteBuffer },
        { "hashstream", testHashStream }
    };

    caseCounts = (int)(sizeof(sCases) / sizeof(TestCase));
    return sCases;
}

void coreSuite(TestSuiteDesc& suite)
{
    suite.name = "core";
    suite.cases = createCases(suite.casesCount);
}

}
