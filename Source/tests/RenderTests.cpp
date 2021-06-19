#include "testsystem.h"
#include <coalpy.core/Assert.h>
#include <coalpy.render/IShaderDb.h>
#include <coalpy.tasks/ITaskSystem.h>
#include <coalpy.files/IFileSystem.h>
#include <coalpy.render/IDevice.h>

using namespace coalpy::render;

namespace coalpy
{
    class RenderTestContext : public TestContext
    {
    public:
        std::string rootResourceDir;
        ITaskSystem* ts = nullptr;
        IFileSystem* fs = nullptr;
        IShaderDb* db = nullptr;
        IDevice* device = nullptr;
        virtual ~RenderTestContext() {};
        void begin()
        {
            ts->start();
            createDevice();
        }

        void end()
        {
            destroyDevice();
            ts->signalStop();
            ts->join();
            ts->cleanFinishedTasks();
        }

        void createDevice();
        void destroyDevice();
    };

    class RenderTestSuite : public TestSuite
    {
    public:
        virtual const char* name() const override { return "render"; }
        virtual const TestCase* getCases(int& caseCounts) const override;
        virtual TestContext* createContext() override
        {
            auto ctx = new RenderTestContext();

            {
                TaskSystemDesc desc;
                desc.threadPoolSize = 8;
                ctx->ts = ITaskSystem::create(desc);
            }

            {
                FileSystemDesc desc { ctx->ts };
                ctx->fs = IFileSystem::create(desc);
            }

            ctx->rootResourceDir = ApplicationContext::get().resourceRootDir();

            return ctx;
        }

        virtual void destroyContext(TestContext* context) override
        {
            auto testContext = static_cast<RenderTestContext*>(context);
            CPY_ASSERT(testContext->db == nullptr);
            CPY_ASSERT(testContext->device == nullptr);
            delete testContext->fs;
            delete testContext->ts;
            delete testContext;
        }
    };

    void RenderTestContext::createDevice()
    {
        CPY_ASSERT(db == nullptr);
        CPY_ASSERT(device == nullptr);

        {
            ShaderDbDesc desc = { rootResourceDir.c_str(), fs, ts };
            db = IShaderDb::create(desc);
        }

        {
            DeviceConfig config;
            config.shaderDb = db;
            device = IDevice::create(config);
        }
    }

    void RenderTestContext::destroyDevice()
    {
        CPY_ASSERT(device != nullptr);
        delete device;
        device = nullptr;

        CPY_ASSERT(db != nullptr);
        delete db;
        db = nullptr;
    }

    // Begin unit tests

    void testCreateBuffer(TestContext& ctx)
    {
        auto& renderTestCtx = (RenderTestContext&)ctx;
        renderTestCtx.begin();
        IDevice& device = *renderTestCtx.device;

        BufferDesc desc;
        desc.name << "SimpleBuffer";
        desc.format = Format::RGBA_32_SINT;
        desc.elementsCount = 20;
        Buffer buff = device.createBuffer(desc);
        CPY_ASSERT(buff.valid());
        device.release(buff);
        renderTestCtx.end();
    }

    void testCreateTexture(TestContext& ctx)
    {
        auto& renderTestCtx = (RenderTestContext&)ctx;
        renderTestCtx.begin();
        IDevice& device = *renderTestCtx.device;

        TextureDesc desc;
        desc.name << "SimpleBuffer";
        desc.format = Format::RGBA_32_SINT;
        desc.width = 128u;
        desc.height = 128u;
        desc.mipLevels = 128u;
        Texture tex = device.createTexture(desc);
        CPY_ASSERT(tex.valid());
        device.release(tex);

        renderTestCtx.end();
    }

    //registration of tests

    const TestCase* RenderTestSuite::getCases(int& caseCounts) const
    {
        static TestCase sCases[] = {
            { "createBuffer",  testCreateBuffer },
            { "createTexture", testCreateTexture }
        };
    
        caseCounts = (int)(sizeof(sCases) / sizeof(TestCase));
        return sCases;
    }

    TestSuite* renderSuite()
    {
        return new RenderTestSuite();
    }
}
