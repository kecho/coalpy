#include "testsystem.h"
#include <coalpy.core/Assert.h>
#include <coalpy.tasks/ITaskSystem.h>
#include <vector>

namespace coalpy
{

class TaskSystemContext : public TestContext
{
public:
    ITaskSystem* ts = nullptr;
};

namespace
{

void testParallel0(TestContext& ctx)
{
    auto& testContext = (TaskSystemContext&)ctx;
    auto& ts = *testContext.ts;
    ts.start();
    std::vector<int> values(500, 0);

    int numTasks = 20;
    struct Range { int b; int e; };

    TaskDesc taskDesc("Job", [&values](TaskContext& ctx)
    {
        auto range = (Range*)ctx.data;
        for (int i = range->b; i < range->e; ++i)
        {
            values[i] = i * 2;
        }
    });

    std::vector<Range> ranges(numTasks);
    std::vector<Task> handles(numTasks);
    int sizePerJob = ((int)values.size())/numTasks;
    for (int i = 0; i < numTasks; ++i)
    {
        auto& r = ranges[i];
        r.b = i * sizePerJob;
        r.e = r.b + sizePerJob;
        handles[i] = ts.createTask(taskDesc, &r);
    }

    ts.execute(handles.data(), (int)handles.size());
    ts.signalStop();
    ts.join();
    ts.cleanFinishedTasks();

    for (int i = 0; i < (int)values.size(); ++i)
    {
        CPY_ASSERT(i * 2 == values[i]);
    }

}

void testTaskDeps(TestContext& ctx)
{
    auto& testContext = (TaskSystemContext&)ctx;
    auto& ts = *testContext.ts;
    ts.start();

    Task root = ts.createTask();

    int i = 0;
    TaskDesc ds0([&i](TaskContext& ctx){
        TaskUtil::sleep(400);
        i = 99;
    });

    Task child0 = ts.createTask(ds0);
    ts.depends(root, child0);
    ts.execute(root);

    ts.signalStop();
    ts.join();
    CPY_ASSERT(i == 99);
    ts.cleanFinishedTasks();
}

}

class TaskSystemTestSuite : public TestSuite
{
public:
    virtual const char* name() const { return "tasksystem"; }
    virtual const TestCase* getCases(int& caseCounts) const
    {
        static TestCase sCases[] = {
            { "testSimpleParallel", testParallel0 },
            { "testSimpleParallelRestart", testParallel0 },
            { "testDependencies", testTaskDeps }
        };

        caseCounts = (int)(sizeof(sCases) / sizeof(TestCase));
        return sCases;
    }

    virtual TestContext* createContext()
    {
        auto testContext = new TaskSystemContext();
        TaskSystemDesc desc;
        desc.threadPoolSize = 8;
        testContext->ts = ITaskSystem::create(desc);
        return testContext;
    }

    virtual void destroyContext(TestContext* context)
    {
        auto testContext = static_cast<TaskSystemContext*>(context);
        delete testContext;
    }
};

TestSuite* taskSystemSuite()
{
    return new TaskSystemTestSuite();
}

}
