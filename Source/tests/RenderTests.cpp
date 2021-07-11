#include "testsystem.h"
#include <coalpy.core/Assert.h>
#include <coalpy.render/IShaderDb.h>
#include <coalpy.tasks/ITaskSystem.h>
#include <coalpy.files/IFileSystem.h>
#include <coalpy.render/IDevice.h>
#include <coalpy.render/CommandList.h>
#include <coalpy.core/Stopwatch.h>
#include <iostream>

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
            auto resourceDir = ApplicationContext::get().resourceRootDir();

            {
                TaskSystemDesc desc;
                desc.threadPoolSize = 8;
                ctx->ts = ITaskSystem::create(desc);
            }

            {
                FileSystemDesc desc { ctx->ts };
                ctx->fs = IFileSystem::create(desc);
            }

            ctx->rootResourceDir = resourceDir;

            return ctx;
        }

        virtual void destroyContext(TestContext* context) override
        {
            auto testContext = static_cast<RenderTestContext*>(context);
            CPY_ASSERT(testContext->db == nullptr);
            CPY_ASSERT(testContext->device == nullptr);
            delete testContext->db;
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
            desc.onErrorFn = [](ShaderHandle handle, const char* shaderName, const char* shaderErrorStr)
            {
                std::cerr << shaderName << ":" << shaderErrorStr << std::endl;
            };
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
        const unsigned char* data = cmdList.data();
        MemOffset offset = {};
        {
            const auto* header = (const AbiCommandListHeader*)(data + offset);
            CPY_ASSERT(header->sentinel == (int)AbiCmdTypes::CommandListSentinel);
            CPY_ASSERT(header->commandListSize == cmdList.size());
            offset += sizeof(AbiCommandListHeader);
        }

        {
            const auto* uploadCommand = (const AbiUploadCmd*)(data + offset);
            CPY_ASSERT(uploadCommand->sentinel == (int)AbiCmdTypes::Upload);
            CPY_ASSERT(uploadCommand->destination == cbuffer);
            const char* str = uploadCommand->sources.data(data);
            CPY_ASSERT(!strcmp(str, testString));
            CPY_ASSERT(testStringSize == uploadCommand->sourceSize);
            offset += uploadCommand->cmdSize;
        }

        {
            const auto* computeCommand = (const AbiComputeCmd*)(data + offset);
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
            offset += computeCommand->cmdSize;

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

    void testRenderMemoryDownload(TestContext& ctx)
    {
        auto& renderTestCtx = (RenderTestContext&)ctx;
        renderTestCtx.begin();
        IDevice& device = *renderTestCtx.device;
        IShaderDb& db = *renderTestCtx.db;

        const char* writeNumberComputeSrc = R"(
            RWBuffer<uint> output : register(u0);

            [numthreads(64,1,1)]
            void csMain(uint3 dti : SV_DispatchThreadID)
            {
                output[dti.x] = dti.x + 1;
            }
        )";
        
        ShaderInlineDesc shaderDesc{ ShaderType::Compute, "setNumsShader", "csMain", writeNumberComputeSrc };
        ShaderHandle shader = db.requestCompile(shaderDesc);
        db.resolve(shader);
        CPY_ASSERT(db.isValid(shader));
        
        int totalElements = 128;
        BufferDesc buffDesc;
        buffDesc.memFlags = (MemFlags)MemFlag_GpuWrite;
        buffDesc.format = Format::R32_SINT;
        buffDesc.elementCount = totalElements;
        Buffer buff = device.createBuffer(buffDesc);

        buffDesc.memFlags = MemFlag_CpuRead;
        Buffer readbackBuff = device.createBuffer(buffDesc);
        
        ResourceTableDesc tableDesc;
        tableDesc.resources = &buff;
        tableDesc.resourcesCount = 1;
        OutResourceTable outTable = device.createOutResourceTable(tableDesc);

        CommandList commandList;
        {
            ComputeCommand cmd;
            cmd.setShader(shader);
            cmd.setOutResources(&outTable, 1);
            cmd.setDispatch("SetNumers", totalElements / 64, 1, 1);
            commandList.writeCommand(cmd);
        }

        {
            CopyCommand cmd;
            cmd.setResources(buff, readbackBuff);
            commandList.writeCommand(cmd);
        }

        {
            DownloadCommand downloadCmd;
            downloadCmd.setData(readbackBuff);
            commandList.writeCommand(downloadCmd);
        }

        commandList.finalize();
        CommandList* lists[] = { &commandList };
        
        auto result = device.schedule(lists, 1, ScheduleFlags_GetWorkHandle); 
        CPY_ASSERT_MSG(result.success(), result.message.c_str());
        auto waitStatus = device.waitOnCpu(result.workHandle);
        CPY_ASSERT(waitStatus.success());

        auto downloadStatus = device.getDownloadStatus(result.workHandle, readbackBuff);
        CPY_ASSERT(downloadStatus.success());
        CPY_ASSERT(downloadStatus.downloadPtr != nullptr);
        CPY_ASSERT(downloadStatus.downloadByteSize != sizeof(unsigned int) * totalElements);
        if (downloadStatus.downloadPtr != nullptr /*TODO: do the size && downloadStatus.downloadByteSize == sizeof(unsigned int) * totalElements*/)
        {
            auto* ptr = (unsigned int*)downloadStatus.downloadPtr;
            for (int i = 0; i < totalElements; ++i)
                CPY_ASSERT(ptr[i] == (i + 1));
        }

        device.release(result.workHandle);
        
        device.release(outTable);
        device.release(buff);
        device.release(readbackBuff);
        renderTestCtx.end();
    }
    
    void testSimpleComputePingPong(TestContext& ctx)
    {
        auto& renderTestCtx = (RenderTestContext&)ctx;
        renderTestCtx.begin();
        IDevice& device = *renderTestCtx.device;
        IShaderDb& db = *renderTestCtx.db;

        const char* pingShaderSrc = R"(
            RWBuffer<uint> output : register(u0);
            RWBuffer<uint> output1 : register(u1);

            [numthreads(64,1,1)]
            void csMain(uint3 dti : SV_DispatchThreadID)
            {
                output[dti.x] = dti.x + 1;
                output1[dti.x] = dti.x + 2;
            }
        )";

        const char* pongShaderSrc = R"(
            Buffer<uint> input : register(t0);
            Buffer<uint> input1 : register(t1);

            RWBuffer<uint> output : register(u0);
            RWBuffer<uint> output1 : register(u1);

            [numthreads(64,1,1)]
            void csMain(uint3 dti : SV_DispatchThreadID)
            {
                output[dti.x] = input[dti.x] + 10;
                output1[dti.x] = input1[dti.x] + 10;
            }
        )";
        
        ShaderInlineDesc shaderDesc0{ ShaderType::Compute, "pingShader", "csMain", pingShaderSrc };
        ShaderHandle pingShader = db.requestCompile(shaderDesc0);
        db.resolve(pingShader);
        CPY_ASSERT(db.isValid(pingShader));

        ShaderInlineDesc shaderDesc1{ ShaderType::Compute, "pongShader", "csMain", pongShaderSrc };
        ShaderHandle pongShader = db.requestCompile(shaderDesc1);
        db.resolve(pongShader);
        CPY_ASSERT(db.isValid(pongShader));
        
        int totalElements = 128;

        BufferDesc buffDesc;
        buffDesc.memFlags = (MemFlags)(MemFlag_GpuRead | MemFlag_GpuWrite);
        buffDesc.format = Format::R32_SINT;
        buffDesc.elementCount = totalElements;

        Buffer pingBuffs[2];
        pingBuffs[0] = device.createBuffer(buffDesc);
        pingBuffs[1] = device.createBuffer(buffDesc);

        Buffer pongBuffs[2];
        pongBuffs[0] = device.createBuffer(buffDesc);
        pongBuffs[1] = device.createBuffer(buffDesc);

        buffDesc.memFlags = MemFlag_CpuRead;
        Buffer readbackBuff0 = device.createBuffer(buffDesc);
        Buffer readbackBuff1 = device.createBuffer(buffDesc);
        
        ResourceTableDesc tableDesc;

        tableDesc.resources = pingBuffs;
        tableDesc.resourcesCount = 2;
        OutResourceTable pingOutTable = device.createOutResourceTable(tableDesc);

        tableDesc.resources = pingBuffs;
        tableDesc.resourcesCount = 2;
        InResourceTable pongInTable = device.createInResourceTable(tableDesc);

        tableDesc.resources = pongBuffs;
        tableDesc.resourcesCount = 2;
        OutResourceTable pongOutTable = device.createOutResourceTable(tableDesc);

        CommandList commandList;
        {
            ComputeCommand cmd;
            cmd.setShader(pingShader);
            cmd.setOutResources(&pingOutTable, 1);
            cmd.setDispatch("Ping", totalElements / 64, 1, 1);
            commandList.writeCommand(cmd);
        }

        {
            ComputeCommand cmd;
            cmd.setShader(pongShader);
            cmd.setInResources(&pongInTable, 1);
            cmd.setOutResources(&pongOutTable, 1);
            cmd.setDispatch("Pong", totalElements / 64, 1, 1);
            commandList.writeCommand(cmd);
        }

        {
            CopyCommand cmd;
            cmd.setResources(pongBuffs[0], readbackBuff0);
            commandList.writeCommand(cmd);
        }

        {
            CopyCommand cmd;
            cmd.setResources(pongBuffs[1], readbackBuff1);
            commandList.writeCommand(cmd);
        }

        {
            DownloadCommand downloadCmd;
            downloadCmd.setData(readbackBuff0);
            commandList.writeCommand(downloadCmd);
        }
        {
            DownloadCommand downloadCmd;
            downloadCmd.setData(readbackBuff1);
            commandList.writeCommand(downloadCmd);
        }

        commandList.finalize();
        CommandList* lists[] = { &commandList };
        
        auto result = device.schedule(lists, 1, ScheduleFlags_GetWorkHandle); 
        CPY_ASSERT_MSG(result.success(), result.message.c_str());
        auto waitStatus = device.waitOnCpu(result.workHandle);
        CPY_ASSERT(waitStatus.success());

        {
            auto downloadStatus = device.getDownloadStatus(result.workHandle, readbackBuff0);
            CPY_ASSERT(downloadStatus.success());
            CPY_ASSERT(downloadStatus.downloadPtr != nullptr);
            CPY_ASSERT(downloadStatus.downloadByteSize != sizeof(unsigned int) * totalElements);
            if (downloadStatus.downloadPtr != nullptr /*TODO: do the size && downloadStatus.downloadByteSize == sizeof(unsigned int) * totalElements*/)
            {
                auto* ptr = (unsigned int*)downloadStatus.downloadPtr;
                for (int i = 0; i < totalElements; ++i)
                    CPY_ASSERT(ptr[i] == (i + 1 + 10));
            }
        }

        {
            auto downloadStatus = device.getDownloadStatus(result.workHandle, readbackBuff1);
            CPY_ASSERT(downloadStatus.success());
            CPY_ASSERT(downloadStatus.downloadPtr != nullptr);
            CPY_ASSERT(downloadStatus.downloadByteSize != sizeof(unsigned int) * totalElements);
            if (downloadStatus.downloadPtr != nullptr /*TODO: do the size && downloadStatus.downloadByteSize == sizeof(unsigned int) * totalElements*/)
            {
                auto* ptr = (unsigned int*)downloadStatus.downloadPtr;
                for (int i = 0; i < totalElements; ++i)
                    CPY_ASSERT(ptr[i] == (i + 2 + 10));
            }
        }

        device.release(result.workHandle);
        
        device.release(pingBuffs[0]);
        device.release(pingBuffs[1]);
        device.release(pongBuffs[0]);
        device.release(pongBuffs[1]);
        device.release(readbackBuff0);
        device.release(readbackBuff1);
        device.release(pingOutTable);
        device.release(pongInTable);
        device.release(pongOutTable);
        renderTestCtx.end();
    }

    //registration of tests

    const TestCase* RenderTestSuite::getCases(int& caseCounts) const
    {
        static TestCase sCases[] = {
            { "createBuffer",  testCreateBuffer },
            { "createTexture", testCreateTexture },
            { "createTables",  testCreateTables },
            { "commandListAbi",  testCommandListAbi },
            { "renderMemoryDownload",  testRenderMemoryDownload },
            { "simpleComputePingPong",  testSimpleComputePingPong }
        };
    
        caseCounts = (int)(sizeof(sCases) / sizeof(TestCase));
        return sCases;
    }

    TestSuite* renderSuite()
    {
        return new RenderTestSuite();
    }
}
