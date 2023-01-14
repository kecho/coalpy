#pragma once

#include <string>
#include <coalpy.render/IDevice.h>

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
    virtual ~TestSuite() {}
    virtual const char* name() const = 0;
    virtual const TestCase* getCases(int& caseCounts) const = 0;
    virtual TestContext* createContext () { return new TestContext(); }
    virtual void destroyContext(TestContext* context) { delete context; }

    void AddRef() { ++m_ref; }
    void Release() { --m_ref; if (m_ref == 0) delete this; }
private:
    int m_ref = 0;
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
