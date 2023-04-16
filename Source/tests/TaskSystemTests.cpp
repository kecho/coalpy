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

//utilities
#define ASSERT_NO_TASKS(s) {\
    ITaskSystem::Stats stats;\
    s.getStats(stats);\
    CPY_ASSERT_FMT(stats.numElements == 0, "Task system still has some tasks alive: %d", stats.numElements);\
    }
//

void testParallel0(TestContext& ctx)
{
    auto& testContext = (TaskSystemContext&)ctx;
    auto& ts = *testContext.ts;
    ASSERT_NO_TASKS(ts);
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
    ASSERT_NO_TASKS(ts);
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
    ts.cleanTaskTree(root);

    CPY_ASSERT_FMT(a == 4, "%d", a);
    CPY_ASSERT_FMT(b == 3, "%d", b);
    CPY_ASSERT_FMT(c == 2, "%d", c);
    CPY_ASSERT_FMT(d == 1, "%d", d);
    CPY_ASSERT_FMT(e == 90, "%d", e);

    ts.signalStop();
    ts.join();
}

void testTaskYield(TestContext& ctx)
{
    auto& testContext = (TaskSystemContext&)ctx;
    auto& ts = *testContext.ts;
    ASSERT_NO_TASKS(ts);
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
    ts.cleanTaskTree(root);

    CPY_ASSERT_FMT(q == 20, "%d", q);
    for (int t : targets)
    {
        CPY_ASSERT_FMT(t == 100, "%d", t);
    }

    ts.signalStop();
    ts.join();
}

}

static const TestCase* createCases(int& caseCounts)
{
    static TestCase sCases[] = {
        { "simpleParallel", testParallel0 },
        { "simpleParallelRestart", testParallel0 },
        { "dependencies", testTaskDeps },
        { "yield", testTaskYield }
    };

    caseCounts = (int)(sizeof(sCases) / sizeof(TestCase));
    return sCases;
}

static TestContext* createContext()
{
    auto testContext = new TaskSystemContext();
    TaskSystemDesc desc;
    desc.threadPoolSize = 8;
    testContext->ts = ITaskSystem::create(desc);
    return testContext;
}

static void destroyContext(TestContext* context)
{
    auto testContext = static_cast<TaskSystemContext*>(context);
    delete testContext->ts;
    delete testContext;
}

void taskSystemSuite(TestSuiteDesc& suite)
{
    suite.name = "tasksystem";
    suite.cases = createCases(suite.casesCount);
    suite.createContextFn = createContext;
    suite.destroyContextFn = destroyContext;
}

}
