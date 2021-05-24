#pragma once

#include <coalpy.shader/IShaderService.h>
#include <coalpy.core/HandleContainer.h>
#include <coalpy.tasks/ThreadQueue.h>
#include <shared_mutex>
#include <string>
#include <memory>
#include <thread>

namespace coalpy
{

class IFileSystem;
class ITaskSystem;

class ShaderService : public IShaderService
{
public:
    ShaderService(const ShaderServiceDesc& desc);
    virtual void start() override;
    virtual void stop()  override;
    virtual ShaderHandle compileShader(const ShaderDesc& desc) override;
    virtual ShaderHandle compileInlineShader(const ShaderInlineDesc& desc) override;
    virtual GpuPipelineHandle createComputePipeline(const ComputePipelineDesc& desc) override;

private:
    void onFileListening();
    IFileSystem* m_fs;
    ITaskSystem* m_ts;

    enum class MessageType
    {
        ListenToDirectories,
        Exit
    };

    struct Message
    {
        MessageType type;
    };

    std::unique_ptr<std::thread> m_fileThread;
    ThreadQueue<Message> m_fileThreadQueue;

    struct ShaderData
    {
        ShaderType type;
        std::string debugName;
        std::string filename;
    };

    std::string m_rootDir;
    std::shared_mutex m_shadersMutex;
    HandleContainer<ShaderHandle, ShaderData> m_shaders;
};


}
