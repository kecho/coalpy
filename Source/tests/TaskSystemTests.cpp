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

    int t = 0;
    auto setIntJob = TaskDesc([&t](TaskContext& ctx) {
        int& i = *(int*)ctx.data;
        i = ++t;
    });
    auto setIntJob2 = TaskDesc([&t](TaskContext& ctx) {
        int& i = *(int*)ctx.data;
        ++i;
    });

    int a = 0;
    int b = 0;
    int c = 0;
    int d = 0;
    int e = 89;

    Task t0 = ts.createTask(setIntJob,  &a);
    Task t1 = ts.createTask(setIntJob,  &b);
    Task t2 = ts.createTask(setIntJob,  &c);
    Task t3 = ts.createTask(setIntJob,  &d);
    Task t4 = ts.createTask(setIntJob2, &e);
    Task root = ts.createTask();

    ts.depends(t0, t1);
    ts.depends(t1, t2);
    ts.depends(t1, t3);
    ts.depends(t1, t4);
    ts.depends(t2, t3);
    ts.depends(root, t0);
    ts.depends(root, t3);
    ts.depends(root, t4);

    ts.execute(root);

    ts.wait(root);

    CPY_ASSERT_FMT(a == 4, "%d", a);
    CPY_ASSERT_FMT(b == 3, "%d", b);
    CPY_ASSERT_FMT(c == 2, "%d", c);
    CPY_ASSERT_FMT(d == 1, "%d", d);
    CPY_ASSERT_FMT(e == 90, "%d", e);

    ts.signalStop();
    ts.join();
    ts.cleanFinishedTasks();
}

void testTaskYield(TestContext& ctx)
{
    auto& testContext = (TaskSystemContext&)ctx;
    auto& ts = *testContext.ts;
    ts.start();

    auto writeToTarget = TaskDesc([](TaskContext& ctx)
    {
        auto& i = *((int*)ctx.data);
        i = 100;
    });

    const int totalJobs = 90;
    int targets[totalJobs];

    std::vector<Task> tasks;
    for (int& t : targets)
    {
        t = 0;
        tasks.push_back(ts.createTask(writeToTarget, &t));
    }

    int q = 0;
    tasks.push_back(ts.createTask(TaskDesc([&q](TaskContext& ctx)
    {
        TaskUtil::yieldUntil([](){ TaskUtil::sleepThread(100); });
        q = 20;
    })));

    Task root = ts.createTask();
    ts.depends(root, tasks.data(), (int)tasks.size());

    ts.execute(root);
    ts.wait(root);

    CPY_ASSERT_FMT(q == 20, "%d", q);
    for (int t : targets)
    {
        CPY_ASSERT_FMT(t == 100, "%d", t);
    }

    ts.signalStop();
    ts.join();
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
            { "testDependencies", testTaskDeps },
            { "testYield", testTaskYield }
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
