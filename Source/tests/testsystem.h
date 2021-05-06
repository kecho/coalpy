#pragma once

namespace coalpy
{

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

class TestSuite
{
public:
    virtual const char* name() const = 0;
    virtual const TestCase* getCases(int& caseCounts) const = 0;
    virtual TestContext* createContext () { return new TestContext(); }
    virtual void destroyContext(TestContext* context) { delete context; }
};

}
