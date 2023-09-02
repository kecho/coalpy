#include "testsystem.h"
#include <coalpy.core/Assert.h>
#include <coalpy.render/IShaderDb.h>
#include <coalpy.tasks/ITaskSystem.h>
#include <coalpy.files/IFileSystem.h>
#include <coalpy.render/IDevice.h>
#include <coalpy.render/CommandList.h>
#include <coalpy.core/Stopwatch.h>
#include <coalpy.render/../../Config.h>
#define INCLUDED_T_DEVICE_H 
#include <coalpy.render/../../TDevice.h>

#if ENABLE_DX12
#include <coalpy.render/../../Dx12/Dx12BufferPool.h>
#include <coalpy.render/../../Dx12/Dx12Device.h>
#endif

#if ENABLE_VULKAN
#include <coalpy.render/../../vulkan/VulkanReadbackBufferPool.h>
#include <coalpy.render/../../vulkan/VulkanDevice.h>
#endif

#include <string>
#include <set>
#include <iostream>
#include <cstring>

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

void RenderTestContext::createDevice()
{
    CPY_ASSERT(db == nullptr);
    CPY_ASSERT(device == nullptr);

    auto platform = ApplicationContext::get().graphicsApi;

    {
        ShaderDbDesc desc = { platform, rootResourceDir.c_str(), fs, ts };
        desc.onErrorFn = [](ShaderHandle handle, const char* shaderName, const char* shaderErrorStr)
        {
            std::cerr << shaderName << ":" << shaderErrorStr << std::endl;
        };
        db = IShaderDb::create(desc);
    }

    {
        DeviceConfig config;
        config.shaderDb = db;
        config.platform = platform;
        config.flags = DeviceFlags::EnableDebug;
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

#if  ENABLE_DX12
void dx12BufferPool(TestContext& ctx)
{
    auto& renderTestCtx = (RenderTestContext&)ctx;
    renderTestCtx.begin();
    IDevice& device = *renderTestCtx.device;
    Dx12Device& dx12Device = (Dx12Device&)device;
    Dx12BufferPool& bufferPool = dx12Device.readbackPool();
    Dx12CpuMemBlock block1 = bufferPool.allocate(256);
    bufferPool.free(block1);

    {
        Dx12CpuMemBlock block2 = bufferPool.allocate(1024 * 1024 * 10);
        Dx12CpuMemBlock block3 = bufferPool.allocate(215);
        Dx12CpuMemBlock block4 = bufferPool.allocate(33);
        Dx12CpuMemBlock block5 = bufferPool.allocate(15);

        bufferPool.free(block5);
        bufferPool.free(block2);
        bufferPool.free(block3);
        bufferPool.free(block4);
    }

    {
        Dx12CpuMemBlock block4 = bufferPool.allocate(3323);
        Dx12CpuMemBlock block2 = bufferPool.allocate(1024 * 1024 * 10);
        Dx12CpuMemBlock block3 = bufferPool.allocate(4123);
        Dx12CpuMemBlock block5 = bufferPool.allocate(1500);

        bufferPool.free(block5);
        bufferPool.free(block2);
        bufferPool.free(block3);
        bufferPool.free(block4);
    }

    renderTestCtx.end();
}
#endif

#if ENABLE_VULKAN
void vulkanBufferPool(TestContext& ctx)
{
    auto& renderTestCtx = (RenderTestContext&)ctx;
    renderTestCtx.begin();
    IDevice& device = *renderTestCtx.device;
    VulkanDevice& vkDevice = (VulkanDevice&)device;
    VulkanReadbackBufferPool& bufferPool = vkDevice.readbackPool();
    VulkanReadbackMemBlock block1 = bufferPool.allocate(256);
    bufferPool.free(block1);

    {
        VulkanReadbackMemBlock block2 = bufferPool.allocate(1024 * 1024 * 10);
        VulkanReadbackMemBlock block3 = bufferPool.allocate(215);
        VulkanReadbackMemBlock block4 = bufferPool.allocate(33);
        VulkanReadbackMemBlock block5 = bufferPool.allocate(15);

        bufferPool.free(block5);
        bufferPool.free(block2);
        bufferPool.free(block3);
        bufferPool.free(block4);
    }

    {
        VulkanReadbackMemBlock block4 = bufferPool.allocate(3323);
        VulkanReadbackMemBlock block2 = bufferPool.allocate(1024 * 1024 * 10);
        VulkanReadbackMemBlock block3 = bufferPool.allocate(4123);
        VulkanReadbackMemBlock block5 = bufferPool.allocate(1500);

        bufferPool.free(block5);
        bufferPool.free(block2);
        bufferPool.free(block3);
        bufferPool.free(block4);
    }

    renderTestCtx.end();
}
#endif

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
    buffDesc.format = Format::R32_UINT;
    buffDesc.elementCount = totalElements;
    Buffer buff = device.createBuffer(buffDesc);
    
    ResourceTableDesc tableDesc;
    tableDesc.resources = &buff;
    tableDesc.resourcesCount = 1;
    OutResourceTableResult outTableResult = device.createOutResourceTable(tableDesc);
    CPY_ASSERT_FMT(outTableResult.success(), "Error creating out table: %s", outTableResult.message.c_str());
    OutResourceTable outTable = outTableResult;

    CommandList commandList;
    {
        ComputeCommand cmd;
        cmd.setShader(shader);
        cmd.setOutResources(&outTable, 1);
        cmd.setDispatch("SetNumers", totalElements / 64, 1, 1);
        commandList.writeCommand(cmd);
    }

    {
        DownloadCommand downloadCmd;
        downloadCmd.setData(buff);
        commandList.writeCommand(downloadCmd);
    }

    commandList.finalize();
    CommandList* lists[] = { &commandList };
    
    auto result = device.schedule(lists, 1, ScheduleFlags_GetWorkHandle); 
    CPY_ASSERT_MSG(result.success(), result.message.c_str());
    auto waitStatus = device.waitOnCpu(result.workHandle, -1);
    CPY_ASSERT(waitStatus.success());

    auto downloadStatus = device.getDownloadStatus(result.workHandle, buff);
    CPY_ASSERT(downloadStatus.success());
    CPY_ASSERT(downloadStatus.downloadPtr != nullptr);
    CPY_ASSERT(downloadStatus.downloadByteSize == sizeof(unsigned int) * totalElements);
    if (downloadStatus.downloadPtr != nullptr && downloadStatus.downloadByteSize == sizeof(unsigned int) * totalElements)
    {
        auto* ptr = (unsigned int*)downloadStatus.downloadPtr;
        for (int i = 0; i < totalElements; ++i)
            CPY_ASSERT(ptr[i] == (i + 1));
    }

    device.release(result.workHandle);
    
    device.release(outTable);
    device.release(buff);
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
    buffDesc.format = Format::R32_UINT;
    buffDesc.elementCount = totalElements;

    Buffer pingBuffs[2];
    pingBuffs[0] = device.createBuffer(buffDesc);
    pingBuffs[1] = device.createBuffer(buffDesc);

    Buffer pongBuffs[2];
    pongBuffs[0] = device.createBuffer(buffDesc);
    pongBuffs[1] = device.createBuffer(buffDesc);
    
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
        DownloadCommand downloadCmd;
        downloadCmd.setData(pongBuffs[0]);
        commandList.writeCommand(downloadCmd);
    }

    {
        DownloadCommand downloadCmd;
        downloadCmd.setData(pongBuffs[1]);
        commandList.writeCommand(downloadCmd);
    }

    commandList.finalize();
    CommandList* lists[] = { &commandList };
    
    auto result = device.schedule(lists, 1, ScheduleFlags_GetWorkHandle); 
    CPY_ASSERT_MSG(result.success(), result.message.c_str());
    auto waitStatus = device.waitOnCpu(result.workHandle, -1);
    CPY_ASSERT(waitStatus.success());

    {
        auto downloadStatus = device.getDownloadStatus(result.workHandle, pongBuffs[0]);
        CPY_ASSERT(downloadStatus.success());
        CPY_ASSERT(downloadStatus.downloadPtr != nullptr);
        CPY_ASSERT(downloadStatus.downloadByteSize == sizeof(unsigned int) * totalElements);
        if (downloadStatus.downloadPtr != nullptr && downloadStatus.downloadByteSize == sizeof(unsigned int) * totalElements)
        {
            auto* ptr = (unsigned int*)downloadStatus.downloadPtr;
            for (int i = 0; i < totalElements; ++i)
                CPY_ASSERT(ptr[i] == (i + 1 + 10));
        }
    }

    {
        auto downloadStatus = device.getDownloadStatus(result.workHandle, pongBuffs[1]);
        CPY_ASSERT(downloadStatus.success());
        CPY_ASSERT(downloadStatus.downloadPtr != nullptr);
        CPY_ASSERT(downloadStatus.downloadByteSize == sizeof(unsigned int) * totalElements);
        if (downloadStatus.downloadPtr != nullptr && downloadStatus.downloadByteSize == sizeof(unsigned int) * totalElements)
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
    device.release(pingOutTable);
    device.release(pongInTable);
    device.release(pongOutTable);
    renderTestCtx.end();
}

void testCachedConstantBuffer(TestContext& ctx)
{
    auto& renderTestCtx = (RenderTestContext&)ctx;
    renderTestCtx.begin();
    IDevice& device = *renderTestCtx.device;
    IShaderDb& db = *renderTestCtx.db;

    const char* cbufferTestSrc = R"(
        cbuffer Constants : register(b0)
        {
            int4 a;
            int4 b;
        }

        RWBuffer<int4> output : register(u0);

        [numthreads(1,1,1)]
        void csMain(uint3 dti : SV_DispatchThreadID)
        {
            output[0] = a;
            output[1] = b;
        }
    )";

    ShaderInlineDesc shaderDesc{ ShaderType::Compute, "cbufferTestShader", "csMain", cbufferTestSrc };
    ShaderHandle shader = db.requestCompile(shaderDesc);
    db.resolve(shader);
    CPY_ASSERT(db.isValid(shader));

    int totalElements = 2;

    BufferDesc buffDesc;
    buffDesc.memFlags = (MemFlags)MemFlag_GpuRead;
    buffDesc.format = Format::RGBA_32_SINT;
    buffDesc.elementCount = totalElements;
    buffDesc.isConstantBuffer = true;
    Buffer constantBuffer = device.createBuffer(buffDesc);

    buffDesc.memFlags = (MemFlags)(MemFlag_GpuRead | MemFlag_GpuWrite);
    buffDesc.isConstantBuffer = false;
    Buffer resultBuffer = device.createBuffer(buffDesc);

    ResourceTableDesc tableDesc;

    tableDesc.resources = &resultBuffer;
    tableDesc.resourcesCount = 1;
    OutResourceTable outTable = device.createOutResourceTable(tableDesc);

    const int constantsData[4 * 2] = {
        -1, 0, 1, 2,
         3, 4, 5, 6
    };

    CommandList commandList;
    {
        UploadCommand cmd;
        cmd.setData((const char*)constantsData, (int)sizeof(constantsData), constantBuffer);
        commandList.writeCommand(cmd);
    }

    {
        ComputeCommand cmd;
        cmd.setShader(shader);
        cmd.setConstants(&constantBuffer, 1);
        cmd.setOutResources(&outTable, 1);
        cmd.setDispatch("testCbuffer", 1, 1, 1);
        commandList.writeCommand(cmd);
    }

    {
        DownloadCommand downloadCmd;
        downloadCmd.setData(resultBuffer);
        commandList.writeCommand(downloadCmd);
    }

    commandList.finalize();
    CommandList* lists[] = { &commandList };
    
    auto result = device.schedule(lists, 1, ScheduleFlags_GetWorkHandle); 
    CPY_ASSERT_MSG(result.success(), result.message.c_str());
    auto waitStatus = device.waitOnCpu(result.workHandle, -1);
    CPY_ASSERT(waitStatus.success());

    {
        auto downloadStatus = device.getDownloadStatus(result.workHandle, resultBuffer);
        CPY_ASSERT(downloadStatus.success());
        CPY_ASSERT(downloadStatus.downloadPtr != nullptr);
        CPY_ASSERT(downloadStatus.downloadByteSize == 4 * sizeof(unsigned int) * totalElements);
        if (downloadStatus.downloadPtr != nullptr && downloadStatus.downloadByteSize == 4 * sizeof(unsigned int) * totalElements)
        {
            auto* ptr = (int*)downloadStatus.downloadPtr;
            for (int i = 0; i < totalElements; ++i)
                CPY_ASSERT(ptr[i] == constantsData[i]);
        }
    }

    device.release(result.workHandle);
    device.release(constantBuffer);
    device.release(resultBuffer);
    device.release(outTable);
    renderTestCtx.end();
}

void testInlineConstantBuffer(TestContext& ctx)
{
    auto& renderTestCtx = (RenderTestContext&)ctx;
    renderTestCtx.begin();
    IDevice& device = *renderTestCtx.device;
    IShaderDb& db = *renderTestCtx.db;

    const char* cbufferTestSrc = R"(
        cbuffer Constants : register(b0)
        {
            int4 a;
            int4 b;
        }

        RWBuffer<int4> output : register(u0);

        [numthreads(1,1,1)]
        void csMain(uint3 dti : SV_DispatchThreadID)
        {
            output[0] = a;
            output[1] = b;
        }
    )";

    ShaderInlineDesc shaderDesc{ ShaderType::Compute, "cbufferTestShader", "csMain", cbufferTestSrc };
    ShaderHandle shader = db.requestCompile(shaderDesc);
    db.resolve(shader);
    CPY_ASSERT(db.isValid(shader));

    int totalElements = 2;

    BufferDesc buffDesc;
    buffDesc.memFlags = (MemFlags)MemFlag_GpuRead;
    buffDesc.format = Format::RGBA_32_SINT;
    buffDesc.elementCount = totalElements;
    buffDesc.memFlags = (MemFlags)(MemFlag_GpuRead | MemFlag_GpuWrite);
    Buffer resultBuffer = device.createBuffer(buffDesc);
    
    ResourceTableDesc tableDesc;

    tableDesc.resources = &resultBuffer;
    tableDesc.resourcesCount = 1;
    OutResourceTable outTable = device.createOutResourceTable(tableDesc);

    const int constantsData[4 * 2] = {
        -1, 0, 1, 2,
         3, 4, 5, 6
    };

    CommandList commandList;
    {
        ComputeCommand cmd;
        cmd.setShader(shader);
        cmd.setInlineConstant((const char*)constantsData, sizeof(constantsData));
        cmd.setOutResources(&outTable, 1);
        cmd.setDispatch("testCbuffer", 1, 1, 1);
        commandList.writeCommand(cmd);
    }

    {
        DownloadCommand downloadCmd;
        downloadCmd.setData(resultBuffer);
        commandList.writeCommand(downloadCmd);
    }

    commandList.finalize();
    CommandList* lists[] = { &commandList };
    
    auto result = device.schedule(lists, 1, ScheduleFlags_GetWorkHandle); 
    CPY_ASSERT_MSG(result.success(), result.message.c_str());
    auto waitStatus = device.waitOnCpu(result.workHandle, -1);
    CPY_ASSERT(waitStatus.success());

    {
        auto downloadStatus = device.getDownloadStatus(result.workHandle, resultBuffer);
        CPY_ASSERT(downloadStatus.success());
        CPY_ASSERT(downloadStatus.downloadPtr != nullptr);
        CPY_ASSERT(downloadStatus.downloadByteSize == 4 * sizeof(unsigned int) * totalElements);
        if (downloadStatus.downloadPtr != nullptr && downloadStatus.downloadByteSize == 4 * sizeof(unsigned int) * totalElements)
        {
            auto* ptr = (int*)downloadStatus.downloadPtr;
            for (int i = 0; i < totalElements; ++i)
                CPY_ASSERT(ptr[i] == constantsData[i]);
        }
    }

    device.release(result.workHandle);
    device.release(resultBuffer);
    device.release(outTable);
    renderTestCtx.end();
}

void testTextureSampler(TestContext& ctx)
{
    auto& renderTestCtx = (RenderTestContext&)ctx;
    renderTestCtx.begin();
    IDevice& device = *renderTestCtx.device;
    IShaderDb& db = *renderTestCtx.db;

    const char* shaderSrc = R"(
        Texture2D<float> g_input  : register(t0);
        RWBuffer<float>  g_output : register(u0);
        SamplerState    g_sampler : register(s0);

        [numthreads(1,1,1)]
        void csMain()
        {
            g_output[0] = g_input.SampleLevel(g_sampler, float2(0.5, 0.5), 0.0);
        }
    )";

    ShaderInlineDesc desc;
    desc.type = ShaderType::Compute;
    desc.name = "testShader";
    desc.mainFn = "csMain";
    desc.immCode = shaderSrc;
    ShaderHandle shader = db.requestCompile(desc);
    db.resolve(shader);
    CPY_ASSERT(db.isValid(shader));

    Texture source;
    {
        TextureDesc desc;
        desc.format = Format::R32_FLOAT;
        desc.width  = 2;
        desc.height = 2;
        source = device.createTexture(desc);
    }

    InResourceTable inputTable;
    {
        ResourceTableDesc desc;
        desc.resources = &source;
        desc.resourcesCount = 1;
        inputTable = device.createInResourceTable(desc);
    }

    Buffer dest;
    {
        BufferDesc desc;
        desc.format = Format::R32_FLOAT;
        desc.elementCount = 1;
        dest = device.createBuffer(desc);
    }

    OutResourceTable outputTable;
    {
        ResourceTableDesc desc;
        desc.resources = &dest;
        desc.resourcesCount = 1;
        outputTable = device.createOutResourceTable(desc);
    }

    Sampler sampler;
    {
        SamplerDesc desc;
        desc.type = FilterType::Linear;
        sampler = device.createSampler(desc);
    }

    SamplerTable samplerTable;
    {
        ResourceTableDesc desc;
        desc.resources = &sampler;
        desc.resourcesCount = 1;
        samplerTable = device.createSamplerTable(desc);
    }

    CommandList commandList;

    const float texData[] = { 0.0f, 0.0f, 1.0f, 1.0f };
    {
        UploadCommand cmd;
        cmd.setData((const char*)texData, sizeof(texData), source);
        commandList.writeCommand(cmd);
    }

    {
        ComputeCommand cmd;
        cmd.setShader(shader);
        cmd.setInResources(&inputTable, 1);
        cmd.setOutResources(&outputTable, 1);
        cmd.setSamplers(&samplerTable, 1);
        cmd.setDispatch("testDispatch", 1, 1, 1);
        commandList.writeCommand(cmd);
    }

    {
        DownloadCommand cmd;
        cmd.setData(dest);
        commandList.writeCommand(cmd);
    }

    commandList.finalize();

    CommandList* cmdListPtr = &commandList;
    ScheduleStatus status = device.schedule(&cmdListPtr, 1, ScheduleFlags_GetWorkHandle); 
    CPY_ASSERT_FMT(status.success(), "%s", status.message.c_str());

    WaitStatus waitStatus = device.waitOnCpu(status.workHandle, -1);
    CPY_ASSERT(waitStatus.success());

    DownloadStatus downloadStatus = device.getDownloadStatus(status.workHandle, dest);
    CPY_ASSERT_FMT(downloadStatus.success(), "%d", downloadStatus.result);

    CPY_ASSERT(downloadStatus.downloadPtr != nullptr);
    if (downloadStatus.downloadPtr)
    {
        float result = *((float*)downloadStatus.downloadPtr);
        CPY_ASSERT(std::abs(result - 0.5f) < 0.001f);
    }

    device.release(status.workHandle);
    
    device.release(source);
    device.release(inputTable);
    device.release(dest);
    device.release(outputTable);
    device.release(sampler);
    device.release(samplerTable);
    renderTestCtx.end();
}

void testUavBarrier(TestContext& ctx)
{
    auto& renderTestCtx = (RenderTestContext&)ctx;
    renderTestCtx.begin();
    IDevice& device = *renderTestCtx.device;
    IShaderDb& db = *renderTestCtx.db;

    const char* uavShaderTest = R"(
        RWBuffer<int> output : register(u0);

        cbuffer Constants : register(b0)
        {
            int4 counter;
        }

        [numthreads(1,1,1)]
        void csMain(uint3 dti : SV_DispatchThreadID)
        {
            output[0] = counter.x == 0 ? 1 : (output[0] + 1);
        }
    )";

    ShaderInlineDesc shaderDesc{ ShaderType::Compute, "uavShaderTest", "csMain", uavShaderTest };
    ShaderHandle shader = db.requestCompile(shaderDesc);
    db.resolve(shader);
    CPY_ASSERT_MSG(db.isValid(shader), "Invalid shader");

    BufferDesc buffDesc;
    buffDesc.memFlags = (MemFlags)MemFlag_GpuRead;
    buffDesc.format = Format::RGBA_32_SINT;
    buffDesc.elementCount = 1;
    buffDesc.memFlags = (MemFlags)(MemFlag_GpuRead | MemFlag_GpuWrite);
    Buffer numBuffer = device.createBuffer(buffDesc);
    
    ResourceTableDesc tableDesc;
    tableDesc.resources = &numBuffer;
    tableDesc.resourcesCount = 1;
    OutResourceTable outTable = device.createOutResourceTable(tableDesc);

    CommandList commandList;

    for (int i = 0; i < 4; ++i)
    {
        ComputeCommand cmd;
        cmd.setShader(shader);
        struct 
        {
            int counter[4];
        } constBuff;
        constBuff.counter[0] = i;
        cmd.setInlineConstant((const char*)&constBuff, sizeof(constBuff));
        cmd.setOutResources(&outTable, 1);
        cmd.setDispatch("uavTestShader", 1, 1, 1);
        commandList.writeCommand(cmd);
    }

    {
        DownloadCommand downloadCmd;
        downloadCmd.setData(numBuffer);
        commandList.writeCommand(downloadCmd);
    }

    commandList.finalize();
    CommandList* lists[] = { &commandList };
    
    auto result = device.schedule(lists, 1, ScheduleFlags_GetWorkHandle); 
    CPY_ASSERT_MSG(result.success(), result.message.c_str());
    auto waitStatus = device.waitOnCpu(result.workHandle, -1);
    CPY_ASSERT_MSG(waitStatus.success(), "Wait failed");

    {
        auto downloadStatus = device.getDownloadStatus(result.workHandle, numBuffer);
        CPY_ASSERT_MSG(downloadStatus.success(), "Invalid download");
        CPY_ASSERT_MSG(downloadStatus.downloadPtr != nullptr, "null download ptr");
        CPY_ASSERT_FMT(downloadStatus.downloadByteSize == 4 * sizeof(int), "Invalid size, expected %d but found %d", sizeof(int), downloadStatus.downloadByteSize);
        if (downloadStatus.downloadPtr != nullptr)
        {
            auto* ptr = (int*)downloadStatus.downloadPtr;
            CPY_ASSERT_FMT(*ptr == 4, "Expected 4, found %d", *ptr);
        }
    }

    device.release(result.workHandle);
    device.release(numBuffer);
    device.release(outTable);
    renderTestCtx.end();
}

void testUpload2dTexture(TestContext& ctx)
{
    auto& renderTestCtx = (RenderTestContext&)ctx;
    renderTestCtx.begin();
    IDevice& device = *renderTestCtx.device;

    const int texDimX = 73;
    const int texDimY = 21;
    TextureDesc texDesc;
    texDesc.format = Format::R32_SINT;
    texDesc.width = texDimX;
    texDesc.height = texDimY;

    Texture destTex = device.createTexture(texDesc);

    int data[texDimX * texDimY];
    for (int i = 0; i < texDimX * texDimY; ++i)
        data[i] = i - 10;

    CommandList cmdList;
    
    size_t sz = sizeof(int)*texDimX*texDimY;
    render::MemOffset offsetForUpload = cmdList.uploadInlineResource(destTex, sz); 

    {
        DownloadCommand downloadCmd;
        downloadCmd.setData(destTex);
        cmdList.writeCommand(downloadCmd);
    }

    cmdList.finalize();

    {
        int* texData = (int*)(cmdList.data() + offsetForUpload);
        memcpy(texData, data, sz);
    }

    CommandList* lists[] = { &cmdList };
    auto result = device.schedule(lists, 1, ScheduleFlags_GetWorkHandle);
    CPY_ASSERT(result.success());

    WaitStatus waitStatus = device.waitOnCpu(result.workHandle, -1);
    CPY_ASSERT(waitStatus.success());

    DownloadStatus downloadStatus = device.getDownloadStatus(result.workHandle, destTex);
    CPY_ASSERT(downloadStatus.success());

    int failCount = 0;
    for (int y = 0; y < texDimY; ++y)
    {
        for (int x = 0; x < texDimX; ++x)
        {
            int* row = (int*)((char*)downloadStatus.downloadPtr + downloadStatus.rowPitch * y);
            int i = texDimX * y + x;
            if (row[x] != (i - 10))
                ++failCount;
        }
    }

    CPY_ASSERT(failCount == 0);
    device.release(result.workHandle);
    device.release(destTex);
    renderTestCtx.end();
}

void testAppendConsumeBufferCreate(TestContext& ctx)
{
    auto& renderTestCtx = (RenderTestContext&)ctx;
    renderTestCtx.begin();
    IDevice& device = *renderTestCtx.device;

    BufferDesc bufferDesc;
    bufferDesc.type = BufferType::Structured;
    bufferDesc.isAppendConsume = true;
    bufferDesc.elementCount = 10;
    
    Buffer buffer1 = device.createBuffer(bufferDesc);
    CPY_ASSERT(buffer1.valid());

    bufferDesc.type = BufferType::Structured;
    bufferDesc.stride = 64;
    Buffer buffer2 = device.createBuffer(bufferDesc);
    CPY_ASSERT(buffer2.valid());

    bufferDesc.type = BufferType::Raw;
    Buffer buffer3 = device.createBuffer(bufferDesc);
    CPY_ASSERT(!buffer3.valid());

    device.release(buffer1);
    device.release(buffer2);

    renderTestCtx.end();
}

void testAppendConsumeBufferAppend(TestContext& ctx)
{
    auto& renderTestCtx = (RenderTestContext&)ctx;
    renderTestCtx.begin();
    IDevice& device = *renderTestCtx.device;
    IShaderDb& db = *renderTestCtx.db;

    const char* producerShaderSrc = R"(
        AppendStructuredBuffer<int> output : register(u0);

        [numthreads(10,1,1)]
        void csMain(uint3 dti : SV_DispatchThreadID)
        {
            output.Append(dti.x);
        }
    )";

    ShaderInlineDesc shaderDesc{ ShaderType::Compute, "acBufferShader", "csMain", producerShaderSrc };
    ShaderHandle shader = db.requestCompile(shaderDesc);
    db.resolve(shader);
    CPY_ASSERT(db.isValid(shader));

    BufferDesc bufferDesc;
    bufferDesc.type = BufferType::Structured;
    bufferDesc.format = Format::R32_SINT;
    bufferDesc.isAppendConsume = true;
    bufferDesc.elementCount = 10;
    
    Buffer acDummybuffer = device.createBuffer(bufferDesc);
    CPY_ASSERT(acDummybuffer.valid());

    Buffer acDummy2buffer = device.createBuffer(bufferDesc);
    CPY_ASSERT(acDummybuffer.valid());

    Buffer acbuffer = device.createBuffer(bufferDesc);
    CPY_ASSERT(acbuffer.valid());

    ResourceTableDesc tableDesc;
    tableDesc.resources = &acbuffer;
    tableDesc.resourcesCount = 1;
    OutResourceTable outTable = device.createOutResourceTable(tableDesc);

    BufferDesc counterBufferDesc;
    counterBufferDesc.format = Format::R32_SINT;
    counterBufferDesc.elementCount = 2;
    Buffer counterBuffer = device.createBuffer(counterBufferDesc);

    CommandList cmdList;
    {
        ClearAppendConsumeCounter clrCmd;
        clrCmd.setData(acbuffer);
        cmdList.writeCommand(clrCmd);
    }

    {
        ComputeCommand cmd;
        cmd.setShader(shader);
        cmd.setOutResources(&outTable, 1);
        cmd.setDispatch("appendConsumeTest", 1, 1, 1);
        cmdList.writeCommand(cmd);
    }

    {
        CopyAppendConsumeCounterCommand cmd;
        cmd.setData(acbuffer, counterBuffer, 4);
        cmdList.writeCommand(cmd);
    }

    {
        ClearAppendConsumeCounter clrCmd;
        clrCmd.setData(acbuffer, 6);
        cmdList.writeCommand(clrCmd);
    }

    {
        CopyAppendConsumeCounterCommand cmd;
        cmd.setData(acbuffer, counterBuffer, 0);
        cmdList.writeCommand(cmd);
    }

    {
        DownloadCommand downloadCmd;
        downloadCmd.setData(acbuffer);
        cmdList.writeCommand(downloadCmd);
    }

    {
        DownloadCommand downloadCmd;
        downloadCmd.setData(counterBuffer);
        cmdList.writeCommand(downloadCmd);
    }

    cmdList.finalize();

    
    CommandList* lists[] = { &cmdList };
    auto result = device.schedule(lists, 1, ScheduleFlags_GetWorkHandle);
    CPY_ASSERT(result.success());

    auto waitStatus = device.waitOnCpu(result.workHandle, -1);
    CPY_ASSERT(waitStatus.success());

    
    //test buffer contents
    {
        auto downloadStatus = device.getDownloadStatus(result.workHandle, acbuffer);
        CPY_ASSERT(downloadStatus.success());
        CPY_ASSERT(downloadStatus.downloadPtr != nullptr);
        CPY_ASSERT(downloadStatus.downloadByteSize == 10 * sizeof(int));
        std::vector<int> hasResults(10, 0);
        if (downloadStatus.downloadPtr != nullptr)
        {
            const int* results = (const int*)downloadStatus.downloadPtr;
            for (int i = 0; i < 10; ++i)
                ++hasResults[results[i]];

            for (int i = 0; i < 10; ++i)
                CPY_ASSERT(hasResults[i] == 1);
        }
        else
        {
            CPY_ASSERT(false);
        }
    }

    {
        auto downloadStatus = device.getDownloadStatus(result.workHandle, counterBuffer);
        CPY_ASSERT(downloadStatus.success());
        CPY_ASSERT(downloadStatus.downloadPtr != nullptr);
        CPY_ASSERT(downloadStatus.downloadByteSize == 2 * sizeof(int));
        if (downloadStatus.downloadPtr)
        {
            CPY_ASSERT(6 == *((int*)downloadStatus.downloadPtr));
            CPY_ASSERT(10 == *((int*)downloadStatus.downloadPtr + 1));
        }
    }

    device.release(result.workHandle);
    device.release(acbuffer);
    device.release(counterBuffer);
    renderTestCtx.end();
}

void testTextureArray(TestContext& ctx)
{
    auto& renderTestCtx = (RenderTestContext&)ctx;
    renderTestCtx.begin();
    IDevice& device = *renderTestCtx.device;
    IShaderDb& db = *renderTestCtx.db;

    const char* producerShaderSrc = R"(
        RWTexture2DArray<int4> output : register(u0);

        cbuffer Constants : register(b0)
        {
            int mipId;
            int maxWidth;
            int maxHeight;
            int padding0;
        }

        [numthreads(4,4,4)]
        void csMain(uint3 dti : SV_DispatchThreadID)
        {
            if (any(dti.xy >= int2(maxWidth, maxHeight)))
                return;

            output[dti] = int4(dti.xyz, mipId);
        }
    )";

    ShaderInlineDesc shaderDesc;
    shaderDesc.type = ShaderType::Compute;
    shaderDesc.name = "TextureArrayProducer";
    shaderDesc.mainFn = "csMain";
    shaderDesc.immCode = producerShaderSrc;
    ShaderHandle shaderHandle = db.requestCompile(shaderDesc);
    db.resolve(shaderHandle);
    CPY_ASSERT(db.isValid(shaderHandle));

    Texture texArray;
    const int mipSlices = 3;
    const int arraySlices = 4;
    {
        TextureDesc desc;
        desc.type = TextureType::k2dArray;
        desc.format = Format::RGBA_32_SINT;
        desc.width = 8;
        desc.height = 8;
        desc.depth = arraySlices;
        //desc.mipLevels = 8;
        //texArray = device.createTexture(desc);
        //CPY_ASSERT(!texArray.valid());

        desc.mipLevels = mipSlices;
        texArray = device.createTexture(desc);
        CPY_ASSERT(texArray.valid());
    }

    OutResourceTable mipTables[mipSlices];
    {
        ResourceTableDesc desc;
        desc.resources = &texArray;
        desc.resourcesCount = 1;
        for (int i = 0; i < mipSlices; ++i)
        {
            desc.uavTargetMips = &i;
            mipTables[i] = device.createOutResourceTable(desc);
            CPY_ASSERT(mipTables[i].valid())
        }
    }

    CommandList cmdList;
    for (int i = 0; i < mipSlices; ++i)
    {
        int w = 8 >> i;
        int h = 8 >> i;
        ComputeCommand cmd;
        cmd.setShader(shaderHandle);
        struct {
            int mipId;
            int maxWidth;
            int maxHeight;
            int padd0;
        } constants = { i, w, h, 0 };
        cmd.setInlineConstant((const char*)&constants, sizeof(constants));
        cmd.setOutResources(&mipTables[i], 1);
        cmd.setDispatch("texArray", 2, 2, arraySlices/4);
        cmdList.writeCommand(cmd);
    }

    for (int mipLevel = 0; mipLevel < mipSlices; ++mipLevel)
    {
        for (int arraySlice = 0; arraySlice < arraySlices; ++arraySlice)
        {
            DownloadCommand cmd;
            cmd.setData(texArray);
            cmd.setMipLevel(mipLevel);
            cmd.setArraySlice(arraySlice);
            cmdList.writeCommand(cmd);
        }
    }

    cmdList.finalize();

    CommandList* cmdLists;
    cmdLists = &cmdList;
    ScheduleStatus scheduleStatus = device.schedule(&cmdLists, 1, ScheduleFlags_GetWorkHandle);
    CPY_ASSERT_MSG(scheduleStatus.success(), scheduleStatus.message.c_str());

    auto waitStatus = device.waitOnCpu(scheduleStatus.workHandle, -1);
    CPY_ASSERT(waitStatus.success());

    for (int mipLevel = 0; mipLevel < mipSlices; ++mipLevel)
    {
        for (int arraySlice = 0; arraySlice < arraySlices; ++arraySlice)
        {
            DownloadStatus downloadStatus = device.getDownloadStatus(scheduleStatus.workHandle, texArray, mipLevel, arraySlice);
            CPY_ASSERT(downloadStatus.success());
            if (!downloadStatus.success())
                continue;
            char* ptr = (char*)downloadStatus.downloadPtr;
            int pixelPitch = sizeof(int) * 4;
            int w = 8 >> mipLevel;
            int h = 8 >> mipLevel;
            for (int x = 0; x < w; ++x)
            {
                for (int y = 0; y < h; ++y)
                {
                    int* pixelPtr = (int*)(ptr + (x * pixelPitch + y * downloadStatus.rowPitch)); 
                    CPY_ASSERT_FMT(pixelPtr[0] == x, "expected %d, got %d", x, pixelPtr[0]);
                    CPY_ASSERT_FMT(pixelPtr[1] == y, "expected %d, got %d", y, pixelPtr[1]);
                    CPY_ASSERT_FMT(pixelPtr[2] == arraySlice, "expected %d, got %d", arraySlice, pixelPtr[2]);
                    CPY_ASSERT_FMT(pixelPtr[3] == mipLevel, "expected %d, got %d", mipLevel, pixelPtr[3]);
                }
            }
        }
    }

    for (int i = 0; i < mipSlices; ++i)
    {
        device.release(mipTables[i]);
    }

    device.release(scheduleStatus.workHandle);
    device.release(texArray);
    renderTestCtx.end();
}


void testIndirectDispatch(TestContext& ctx)
{
    auto& renderTestCtx = (RenderTestContext&)ctx;
    renderTestCtx.begin();
    IDevice& device = *renderTestCtx.device;
    IShaderDb& db = *renderTestCtx.db;

    const char* setIndirectArgsSrc = R"(
        RWBuffer<int4> output : register(u0);
        [numthreads(1,1,1)]
        void csMain(uint3 dti : SV_DispatchThreadID)
        {
            output[0] = int4(2,3,4,0);
        }
    )";

    ShaderHandle setIndirectShader;
    {
        ShaderInlineDesc shaderDesc;
        shaderDesc.type = ShaderType::Compute;
        shaderDesc.name = "SetIndirectArgs";
        shaderDesc.mainFn = "csMain";
        shaderDesc.immCode = setIndirectArgsSrc;
        setIndirectShader = db.requestCompile(shaderDesc);
        db.resolve(setIndirectShader);
        CPY_ASSERT(db.isValid(setIndirectShader));
    }

    const char* executeIndirectSrc = R"(
        RWBuffer<int4> output : register(u0);
        [numthreads(1,1,1)]
        void csMain(uint3 gti : SV_GroupID)
        {
            int index = gti.x + gti.y * 2 + gti.z * (2 * 3);
            output[index] = int4(gti, 9.0);
        }
    )";

    ShaderHandle executeIndirectShader;
    {
        ShaderInlineDesc shaderDesc;
        shaderDesc.type = ShaderType::Compute;
        shaderDesc.name = "executeIndirectShader";
        shaderDesc.mainFn = "csMain";
        shaderDesc.immCode = executeIndirectSrc;
        executeIndirectShader = db.requestCompile(shaderDesc);
        db.resolve(executeIndirectShader);
        CPY_ASSERT(db.isValid(executeIndirectShader));
    }

    Buffer indirectArgsBuffer;
    {
        BufferDesc desc;
        desc.elementCount = 1;
        indirectArgsBuffer = device.createBuffer(desc);
        CPY_ASSERT(indirectArgsBuffer.valid());
    }

    Buffer outputBuffer;
    {
        BufferDesc desc;
        desc.elementCount =  2 * 3 * 4;
        outputBuffer = device.createBuffer(desc);
        CPY_ASSERT(outputBuffer.valid());
    }

    OutResourceTable indirectOutTable;
    {
        ResourceTableDesc desc;
        desc.resources = &indirectArgsBuffer;
        desc.resourcesCount = 1;
        indirectOutTable = device.createOutResourceTable(desc);
    }

    OutResourceTable outputTable;
    {
        ResourceTableDesc desc;
        desc.resources = &outputBuffer;
        desc.resourcesCount = 1;
        outputTable = device.createOutResourceTable(desc);
    }

    CommandList cmdList;
    {
        ComputeCommand cmd;
        cmd.setShader(setIndirectShader);
        cmd.setOutResources(&indirectOutTable, 1);
        cmd.setDispatch("setupArgs", 1, 1, 1);
        cmdList.writeCommand(cmd);
    }

    {
        ComputeCommand cmd;
        cmd.setShader(executeIndirectShader);
        cmd.setOutResources(&outputTable, 1);
        cmd.setIndirectDispatch("indirectDispatch", indirectArgsBuffer);
        cmdList.writeCommand(cmd);
    }

    {
        DownloadCommand cmd;
        cmd.setData(outputBuffer);
        cmdList.writeCommand(cmd);
    }
    
    cmdList.finalize();

    CommandList* cmdLists = &cmdList;
    ScheduleStatus scheduleStatus = device.schedule(&cmdLists, 1, ScheduleFlags_GetWorkHandle);
    CPY_ASSERT(scheduleStatus.success());

    WaitStatus waitStatus = device.waitOnCpu(scheduleStatus.workHandle, -1);
    CPY_ASSERT(waitStatus.success());

    DownloadStatus downloadResult = device.getDownloadStatus(scheduleStatus.workHandle, outputBuffer);
    CPY_ASSERT(downloadResult.success());

    const int* intptr = (int*)downloadResult.downloadPtr;
    for (int x = 0; x < 2; ++x)
        for (int y = 0; y < 3; ++y)
            for (int z = 0; z < 4; ++z)
            {
                int index = x + y * 2 + z * 2 * 3;
                const int* data = intptr + index * 4;
                CPY_ASSERT(data[0] == x);
                CPY_ASSERT(data[1] == y);
                CPY_ASSERT(data[2] == z);
                CPY_ASSERT(data[3] == 9);
            }

    device.release(scheduleStatus.workHandle);
    device.release(indirectArgsBuffer);
    device.release(outputBuffer);
    device.release(indirectOutTable);
    device.release(outputTable);
    renderTestCtx.end();
}

void testCopyBuffer(TestContext& ctx)
{
    auto& renderTestCtx = (RenderTestContext&)ctx;
    renderTestCtx.begin();
    IDevice& device = *renderTestCtx.device;

    const int bufferLen = 8;
    const int testBuffer[bufferLen] = { 2, 3, 4, 5, 7, 8, 90, 0xfffff };

    Buffer srcBuffer;
    Buffer dstBuffer;

    { 
        BufferDesc bufferDesc;
        bufferDesc.format = Format::R32_SINT;
        bufferDesc.elementCount = bufferLen;
        srcBuffer = device.createBuffer(bufferDesc);
        dstBuffer = device.createBuffer(bufferDesc);
    }

    CommandList cmdList;

    {
        UploadCommand cmd;
        cmd.setData((const char*)testBuffer, sizeof(int) * bufferLen, srcBuffer);
        cmdList.writeCommand(cmd);
    }

    //Simple copy
    {
        CopyCommand cmd;
        cmd.setResources(srcBuffer, dstBuffer);
        cmdList.writeCommand(cmd);
    }

    {
        DownloadCommand cmd;
        cmd.setData(dstBuffer);
        cmdList.writeCommand(cmd);
    }

    cmdList.finalize();

    {
        CommandList* cmdListPtr = &cmdList;
        ScheduleStatus scheduleStatus = device.schedule(&cmdListPtr, 1, ScheduleFlags_GetWorkHandle);
        CPY_ASSERT(scheduleStatus.success());

        WaitStatus waitStatus = device.waitOnCpu(scheduleStatus.workHandle, -1);
        CPY_ASSERT(waitStatus.success());

        //Verify simple copy
        DownloadStatus downloadStatus = device.getDownloadStatus(scheduleStatus.workHandle, dstBuffer);
        CPY_ASSERT(downloadStatus.success());

        auto resultList = (int*)downloadStatus.downloadPtr;
        for (int i = 0; i < bufferLen; ++i)
            CPY_ASSERT(testBuffer[i] == resultList[i]);
    
        device.release(scheduleStatus.workHandle);
    }

    cmdList.reset();
    const int copyLen = 4;
    const int srcOffset = 4;
    const int dstOffset = 1;

    //Complex copy
    {
        CopyCommand cmd;
        cmd.setBuffers(srcBuffer, dstBuffer, copyLen * sizeof(int), srcOffset * sizeof(int), dstOffset * sizeof(int));
        cmdList.writeCommand(cmd);
    }

    {
        DownloadCommand cmd;
        cmd.setData(dstBuffer);
        cmdList.writeCommand(cmd);
    }

    cmdList.finalize();

    {
        CommandList* cmdListPtr = &cmdList;
        ScheduleStatus scheduleStatus = device.schedule(&cmdListPtr, 1, ScheduleFlags_GetWorkHandle);
        CPY_ASSERT(scheduleStatus.success());

        WaitStatus waitStatus = device.waitOnCpu(scheduleStatus.workHandle, -1);
        CPY_ASSERT(waitStatus.success());

        //Verify simple copy
        DownloadStatus downloadStatus = device.getDownloadStatus(scheduleStatus.workHandle, dstBuffer);
        CPY_ASSERT(downloadStatus.success());

        auto resultList = (int*)downloadStatus.downloadPtr;
        for (int i = 0; i < copyLen; ++i)
            CPY_ASSERT(testBuffer[i + srcOffset] == resultList[i + dstOffset]);
    
        device.release(scheduleStatus.workHandle);
    }

    device.release(srcBuffer);
    device.release(dstBuffer);
    renderTestCtx.end();
}

void testCopyTexture(TestContext& ctx)
{
    auto& renderTestCtx = (RenderTestContext&)ctx;
    renderTestCtx.begin();
    IDevice& device = *renderTestCtx.device;

    const int txW = 4;
    const int txH = 4;
    const int totalPixels = txW * txH;
    const int pixelVals[totalPixels] = {
        1, 2, 3, 4,
        5, 6, 7, 8,
        9, 10, 11, 12,
        13, 14, 15, 16 };

    Texture srcTexture;
    Texture dstTexture;
    Texture dstSmallTexture;
    {
        TextureDesc desc;

        desc.type = TextureType::k2d;
        desc.format = Format::R32_SINT;
        desc.width = txW;
        desc.height = txH;
        desc.mipLevels = 1;
        srcTexture = device.createTexture(desc);

        desc.type = TextureType::k2d;
        desc.width = txW;
        desc.height = txH;
        desc.mipLevels = 1;
        dstTexture = device.createTexture(desc);

        desc.type = TextureType::k2d;
        desc.width >>= 1;
        desc.height >>= 1;
        desc.mipLevels = 1;
        dstSmallTexture = device.createTexture(desc);
    }

    CommandList cmdList;
    {
        UploadCommand cmd;
        cmd.setData((const char*)pixelVals, totalPixels * sizeof(int), srcTexture);
        cmdList.writeCommand(cmd);
    }

    {
        CopyCommand cmd;
        cmd.setResources(srcTexture, dstTexture);
        cmdList.writeCommand(cmd);
    }

    {
        DownloadCommand cmd;
        cmd.setData(dstTexture);
        cmdList.writeCommand(cmd);
    }

    cmdList.finalize();

    //Check simple copy
    {
        CommandList* cmdListPtr = &cmdList;
        ScheduleStatus scheduleStatus = device.schedule(&cmdListPtr, 1, ScheduleFlags_GetWorkHandle);
        CPY_ASSERT_MSG(scheduleStatus.success(), scheduleStatus.message.c_str());

        WaitStatus waitStatus = device.waitOnCpu(scheduleStatus.workHandle, -1);
        CPY_ASSERT(waitStatus.success());

        //Verify simple copy
        DownloadStatus downloadStatus = device.getDownloadStatus(scheduleStatus.workHandle, dstTexture);
        CPY_ASSERT(downloadStatus.success());

        const char* resultTexels = (const char*)downloadStatus.downloadPtr;
        for (int y = 0; y < txH; ++y)
        {
            const int* row = (const int*)(resultTexels + (downloadStatus.rowPitch * y));
            for (int x = 0; x < txW; ++x)
            {
                CPY_ASSERT(pixelVals[y * txW + x] == row[x]);
            }
        }

        device.release(scheduleStatus.workHandle);
    }

    ///////////////////

    cmdList.reset();

    const int xSrcOffset = 1;
    const int ySrcOffset = 2;
    const int smallCopyWidth = 2;
    const int smallCopyHeight = 2;

    {
        CopyCommand cmd;
        cmd.setTextures(srcTexture, dstSmallTexture, smallCopyWidth, smallCopyHeight, 1,
            xSrcOffset, ySrcOffset, 0,
            0, 0, 0);
        cmdList.writeCommand(cmd);
    }

    {
        DownloadCommand cmd;
        cmd.setData(dstSmallTexture);
        cmdList.writeCommand(cmd);
    }
    
    cmdList.finalize();

    //Check more complex copy
    {
        CommandList* cmdListPtr = &cmdList;
        ScheduleStatus scheduleStatus = device.schedule(&cmdListPtr, 1, ScheduleFlags_GetWorkHandle);
        CPY_ASSERT_MSG(scheduleStatus.success(), scheduleStatus.message.c_str());

        WaitStatus waitStatus = device.waitOnCpu(scheduleStatus.workHandle, -1);
        CPY_ASSERT(waitStatus.success());

        DownloadStatus downloadStatus = device.getDownloadStatus(scheduleStatus.workHandle, dstSmallTexture);
        CPY_ASSERT(downloadStatus.success());

        const char* resultTexels = (const char*)downloadStatus.downloadPtr;
        for (int y = 0; y < smallCopyWidth; ++y)
        {
            const int* row = (const int*)(resultTexels + (downloadStatus.rowPitch * y));
            for (int x = 0; x < smallCopyHeight; ++x)
            {
                CPY_ASSERT(pixelVals[(y + ySrcOffset) * txW + (x + xSrcOffset)] == row[x]);
            }
        }

        device.release(scheduleStatus.workHandle);
    }

    device.release(srcTexture);
    device.release(dstTexture);
    device.release(dstSmallTexture);
    renderTestCtx.end();
}

void testCopyTextureArrayAndMips(TestContext& ctx)
{
    auto& renderTestCtx = (RenderTestContext&)ctx;
    renderTestCtx.begin();
    IDevice& device = *renderTestCtx.device;

    const int txW = 4;
    const int txH = 4;
    const int totalPixels = txW * txH;
    const int pixelVals[totalPixels] = {
        1, 2, 3, 4,
        5, 6, 7, 8,
        9, 10, 11, 12,
        13, 14, 15, 16 };

    Texture srcTexture;
    Texture dstTexture;
    {
        TextureDesc desc;

        desc.type = TextureType::k2d;
        desc.format = Format::R32_SINT;
        desc.width = txW;
        desc.height = txH;
        desc.mipLevels = 2;
        srcTexture = device.createTexture(desc);

        desc.type = TextureType::k2dArray;
        desc.width >>= 1;
        desc.height >>= 1;
        desc.depth = 3;
        desc.mipLevels = 1;
        dstTexture = device.createTexture(desc);
    }

    CommandList cmdList;
    {
        UploadCommand cmd;
        cmd.setData((const char*)pixelVals, totalPixels * sizeof(int), srcTexture);
        cmd.setTextureDestInfo(
            2, 2, 1, //size
            0, 0, 0, //dest
            1); //mip level
        cmdList.writeCommand(cmd);
    }

    {
        CopyCommand cmd;
        cmd.setTextures(srcTexture, dstTexture, txW >> 1, txH >> 1,  1,
            0, 0, 0, 
            0, 0, 2, //dst array 2
            1, 0); //src mip level is 1
        cmdList.writeCommand(cmd);
    }

    {
        DownloadCommand cmd;
        cmd.setData(dstTexture);
        cmd.setArraySlice(2);
        cmdList.writeCommand(cmd);
    }

    cmdList.finalize();

    //Check
    {
        CommandList* cmdListPtr = &cmdList;
        ScheduleStatus scheduleStatus = device.schedule(&cmdListPtr, 1, ScheduleFlags_GetWorkHandle);
        CPY_ASSERT_MSG(scheduleStatus.success(), scheduleStatus.message.c_str());

        WaitStatus waitStatus = device.waitOnCpu(scheduleStatus.workHandle, -1);
        CPY_ASSERT(waitStatus.success());

        //Verify simple copy
        DownloadStatus downloadStatus = device.getDownloadStatus(scheduleStatus.workHandle, dstTexture, 0, 2);
        CPY_ASSERT(downloadStatus.success());

        const char* resultTexels = (const char*)downloadStatus.downloadPtr;
        CPY_ASSERT(resultTexels);
        if (resultTexels)
        {
            for (int y = 0; y < (txH >> 1); ++y)
            {
                const int* row = (const int*)(resultTexels + (downloadStatus.rowPitch * y));
                for (int x = 0; x < (txW >> 1); ++x)
                {
                    CPY_ASSERT(pixelVals[y * (txW >> 1) + x] == row[x]);
                }
            }
        }

        device.release(scheduleStatus.workHandle);
    }

    ///////////////////

    device.release(srcTexture);
    device.release(dstTexture);
    renderTestCtx.end();
}

void testCollectGpuMarkers(TestContext& ctx)
{
    auto& renderTestCtx = (RenderTestContext&)ctx;
    renderTestCtx.begin();
    IDevice& device = *renderTestCtx.device;

    CommandList cmdList;

    cmdList.beginMarker("Parent0");
        cmdList.beginMarker("Child0");
            cmdList.beginMarker("Child1");
            cmdList.endMarker();
        cmdList.endMarker();
        cmdList.beginMarker("Child2");
        cmdList.endMarker();
    cmdList.endMarker();

    cmdList.finalize();
    
    device.beginCollectMarkers();
    {
        CommandList* cmdListPtr = &cmdList;
        device.schedule(&cmdListPtr, 1);
    }
    MarkerResults results = device.endCollectMarkers();
    CPY_ASSERT(results.timestampBuffer.success());
    CPY_ASSERT(results.markerCount == 4);
    CPY_ASSERT(results.markers != nullptr);

    //download and test data
    {
        CommandList downloadList;        
        DownloadCommand downloadCmd;
        downloadCmd.setData(results.timestampBuffer);
        downloadList.writeCommand(downloadCmd);
        downloadList.finalize();
        {
            CommandList* cmdListPtr = &downloadList;
            ScheduleStatus scheduleStatus = device.schedule(&cmdListPtr, 1, ScheduleFlags_GetWorkHandle);
            CPY_ASSERT_MSG(scheduleStatus.success(), scheduleStatus.message.c_str());

            WaitStatus waitStatus = device.waitOnCpu(scheduleStatus.workHandle, -1);
            CPY_ASSERT(waitStatus.success());

            //Verify simple copy
            DownloadStatus downloadStatus = device.getDownloadStatus(scheduleStatus.workHandle, results.timestampBuffer);
            CPY_ASSERT(downloadStatus.success());

            uint64_t* timestamps = (uint64_t*)downloadStatus.downloadPtr;
            for (int i = 0; i < results.markerCount; ++i)
            {
                const MarkerTimestamp& markerData = results.markers[i];
                auto b = timestamps[markerData.beginTimestampIndex];
                auto e = timestamps[markerData.endTimestampIndex];
                //std::cout << "b: " << b << " e: " << e << " d: " << (e - b) << " :: " << ((float)(e - b) /(float)results.timestampFrequency) << std::endl;
                CPY_ASSERT(e >= b);
                CPY_ASSERT(e != 0);
                CPY_ASSERT(b != 0);
            }
        }
    }

    renderTestCtx.end();
}

static const TestCase* createCases(int& caseCounts)
{
    static const TestCase sCases[] = {
#if ENABLE_DX12
        { "dx12BufferPool",  dx12BufferPool },
#endif
        { "vulkanBufferPool", vulkanBufferPool },
        { "createBuffer",  testCreateBuffer },
        { "createTexture", testCreateTexture },
        { "createTables",  testCreateTables },
        { "commandListAbi",  testCommandListAbi },
        { "renderMemoryDownload",  testRenderMemoryDownload },
        { "simpleComputePingPong",  testSimpleComputePingPong },
        { "cachedConstantBuffer",  testCachedConstantBuffer },
        { "inlineConstantBuffer",  testInlineConstantBuffer },
        { "textureSamplers",  testTextureSampler },
        { "uavBarrier",  testUavBarrier },
        { "upload2dTexture",  testUpload2dTexture },
        { "appendConsumeBufferCreate",  testAppendConsumeBufferCreate },
        { "appendConsumeBufferAppend",  testAppendConsumeBufferAppend },
        { "textureArray",  testTextureArray },
        { "indirectDispatch",  testIndirectDispatch },
        { "copyBuffer",  testCopyBuffer },
        { "copyTexture",  testCopyTexture },
        { "copyTextureArrayAndMips",  testCopyTextureArrayAndMips },
        { "collectGpuMarkers",  testCollectGpuMarkers },
    };

    caseCounts = sizeof(sCases)/sizeof(sCases[0]);
    return sCases;
}

static const TestCaseFilter* createCasesFilters(int& caseCounts)
{
    static const TestCaseFilter sFilters[] =
    {
#if  ENABLE_DX12
        { "dx12BufferPool", TestPlatformDx12 },
#endif
#if  ENABLE_VULKAN
        { "vulkanBufferPool", TestPlatformVulkan },
#endif
    };

    caseCounts = sizeof(sFilters)/sizeof(sFilters[0]);
    return sFilters;
}

static TestContext* createContext()
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

static void destroyContext(TestContext* context)
{
    auto testContext = static_cast<RenderTestContext*>(context);
    CPY_ASSERT(testContext->db == nullptr);
    CPY_ASSERT(testContext->device == nullptr);
    delete testContext->db;
    delete testContext->fs;
    delete testContext->ts;
    delete testContext;
}

void renderSuite(TestSuiteDesc& suite)
{
    suite.name = "render";
    suite.cases = createCases(suite.casesCount);
    suite.filters = createCasesFilters(suite.filterCounts);
    suite.createContextFn = createContext;
    suite.destroyContextFn = destroyContext;
    unsigned supportedPlatforms = 0;
#if ENABLE_DX12
    supportedPlatforms |= TestPlatformDx12;
#endif
#if ENABLE_VULKAN
    supportedPlatforms |= TestPlatformVulkan;
#endif
    suite.supportedRenderPlatforms = (TestPlatforms)supportedPlatforms;
}

}
