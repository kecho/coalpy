#include "testsystem.h"
#include <coalpy.core/Assert.h>
#include <coalpy.render/IShaderDb.h>
#include <coalpy.tasks/ITaskSystem.h>
#include <coalpy.files/IFileSystem.h>
#include <coalpy.render/IDevice.h>
#include <coalpy.render/CommandList.h>

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
        desc.name = "SimpleBuffer";
        desc.format = Format::RGBA_32_SINT;
        desc.elementCount = 20;
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
        desc.name = "SimpleBuffer";
        desc.format = Format::RGBA_32_SINT;
        desc.width = 128u;
        desc.height = 128u;
        Texture tex = device.createTexture(desc);
        CPY_ASSERT(tex.valid());
        device.release(tex);

        renderTestCtx.end();
    }

    void testCreateTables(TestContext& ctx)
    {
        auto& renderTestCtx = (RenderTestContext&)ctx;
        renderTestCtx.begin();
        IDevice& device = *renderTestCtx.device;

        

        TextureDesc texDesc;
        texDesc.name = "SimpleBuffer";
        texDesc.format = Format::RGBA_32_SINT;
        texDesc.width = 128u;
        texDesc.height = 128u;
        texDesc.memFlags = (MemFlags)(MemFlag_GpuWrite | MemFlag_GpuRead);

    
        BufferDesc buffDesc;
        buffDesc.name = "SimpleBuffer";
        buffDesc.format = Format::RGBA_32_SINT;
        buffDesc.elementCount = 20;
        buffDesc.memFlags = (MemFlags)(MemFlag_GpuWrite | MemFlag_GpuRead);

        const int resourceCount = 16;
        ResourceHandle handles[resourceCount];
        for (int i = 0; i < resourceCount; ++i)
        {
            handles[i] = ((i & 0x1) ? (ResourceHandle)device.createTexture(texDesc) : (ResourceHandle)device.createBuffer(buffDesc));
            CPY_ASSERT(handles[i].valid());
        }

        ResourceTableDesc tableDesc;
        tableDesc.resources = handles;
        tableDesc.resourcesCount = resourceCount;

        InResourceTable inTable = device.createInResourceTable(tableDesc);
        CPY_ASSERT(inTable.valid());

        OutResourceTable outTable = device.createOutResourceTable(tableDesc);
        CPY_ASSERT(outTable.valid());

        for (auto& h : handles)
            device.release(h);

        device.release(inTable);
        device.release(outTable);

        renderTestCtx.end();
    }

    void testCommandListAbi(TestContext& ctx)
    {
        auto& renderTestCtx = (RenderTestContext&)ctx;
        renderTestCtx.begin();
        IDevice& device = *renderTestCtx.device;

        TextureDesc texDesc;

        const int inputTexturesCount = 4;
        const int outputTexturesCount = 3;
        Texture inputTextures[inputTexturesCount];
        Texture outputTextures[outputTexturesCount];

        for (Texture& t : inputTextures)
            t = device.createTexture(texDesc);

        for (Texture& t : outputTextures)
            t = device.createTexture(texDesc);

        ResourceTableDesc tableDesc;

        tableDesc.resources = inputTextures;
        tableDesc.resourcesCount = inputTexturesCount;
        InResourceTable inputTable = device.createInResourceTable(tableDesc);

        tableDesc.resources = outputTextures;
        tableDesc.resourcesCount = outputTexturesCount;
        OutResourceTable outputTable = device.createOutResourceTable(tableDesc);

        BufferDesc buffDesc;
        Buffer cbuffer = device.createBuffer(buffDesc);

        CommandList cmdList;

        const char testString[] = "hello world";
        int testStringSize = sizeof(testString);

        {
            UploadCommand cmd;
            cmd.setData(testString, (int)testStringSize, cbuffer);
            cmdList.writeCommand(cmd);
        }

        const char* dispatchNameStr = "testDispatch";
        {
            ComputeCommand cmd;
            cmd.setConstants(&cbuffer, 1);
            cmd.setInResources(&inputTable, 1);
            cmd.setOutResources(&outputTable, 1);
            cmd.setDispatch("testDispatch", 8,8,1);
            cmdList.writeCommand(cmd);
        }

        cmdList.finalize();

        //Verify
        unsigned char* data = cmdList.data();
        MemOffset offset = {};
        {
            auto* header = (AbiCommandListHeader*)(data + offset);
            CPY_ASSERT(header->sentinel == (int)AbiCmdTypes::CommandListSentinel);
            CPY_ASSERT(header->commandListSize == cmdList.size());
            offset += sizeof(AbiCommandListHeader);
        }

        {
            auto* uploadCommand = (AbiUploadCmd*)(data + offset);
            CPY_ASSERT(uploadCommand->sentinel == (int)AbiCmdTypes::Upload);
            CPY_ASSERT(uploadCommand->destination == cbuffer);
            const char* str = uploadCommand->sources.data(data);
            CPY_ASSERT(!strcmp(str, testString));
            CPY_ASSERT(testStringSize == uploadCommand->sourceSize);
            offset += sizeof(AbiUploadCmd);
        }

        {
            auto* computeCommand = (AbiComputeCmd*)(data + offset);
            CPY_ASSERT(computeCommand->x == 8);
            CPY_ASSERT(computeCommand->y == 8);
            CPY_ASSERT(computeCommand->z == 1);
            CPY_ASSERT(computeCommand->constantCounts == 1);
            CPY_ASSERT(*computeCommand->constants.data(data) == cbuffer);
            CPY_ASSERT(computeCommand->inResourceTablesCounts == 1);
            CPY_ASSERT(*computeCommand->inResourceTables.data(data) == inputTable);
            CPY_ASSERT(computeCommand->outResourceTablesCounts == 1);
            CPY_ASSERT(*computeCommand->outResourceTables.data(data) == outputTable);
            CPY_ASSERT(!strcmp(computeCommand->debugName.data(data), dispatchNameStr));
            offset += sizeof(AbiComputeCmd);

        }

        for (Texture& t : inputTextures)
            device.release(t);
        for (Texture& t : outputTextures)
            device.release(t);
        device.release(cbuffer);
        device.release(inputTable);
        device.release(outputTable);

        renderTestCtx.end();
    }

    //registration of tests

    const TestCase* RenderTestSuite::getCases(int& caseCounts) const
    {
        static TestCase sCases[] = {
            { "createBuffer",  testCreateBuffer },
            { "createTexture", testCreateTexture },
            { "createTables",  testCreateTables },
            { "commandListAbi",  testCommandListAbi }
        };
    
        caseCounts = (int)(sizeof(sCases) / sizeof(TestCase));
        return sCases;
    }

    TestSuite* renderSuite()
    {
        return new RenderTestSuite();
    }
}
