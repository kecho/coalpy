#include "ShaderService.h" 
#include <coalpy.files/Utils.h>
#include <coalpy.core/Assert.h>
#include <coalpy.render/IShaderDb.h>
#include <string>

#ifdef _WIN32 
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace coalpy
{

ShaderService::ShaderService(const ShaderServiceDesc& desc)
: m_fs(desc.fs)
, m_ts(desc.ts)
, m_db(desc.db)
, m_fileWatchPollingRate(desc.fileWatchPollingRate)
{
    std::string dirName = desc.watchDirectory;
    FileUtils::fixStringPath(dirName, m_rootDir);
}

void ShaderService::start()
{
    m_fileThread = std::make_unique<std::thread>(
        [this]()
    {
        onFileListening();
    });

    Message msg;
    msg.type = MessageType::ListenToDirectories;
    m_fileThreadQueue.push(msg);
}

void ShaderService::stop()
{
    Message msg;
    msg.type = MessageType::Exit;
    m_fileThreadQueue.push(msg);
    m_fileThread->join();
}

void ShaderService::onFileListening()
{
    bool active = true;
#ifdef _WIN32 
    HANDLE h = INVALID_HANDLE_VALUE;
#else
    #error "Platform not supported"
#endif
    while (active)
    {
        Message msg;
        m_fileThreadQueue.waitPop(msg);
        switch (msg.type)
        {
        case MessageType::ListenToDirectories:
#ifdef _WIN32 
            {
                if (h == INVALID_HANDLE_VALUE)
                {
                    h = FindFirstChangeNotificationA(m_rootDir.c_str(), TRUE, FILE_NOTIFY_CHANGE_LAST_WRITE);
                    CPY_ASSERT(h != INVALID_HANDLE_VALUE);
                }
                else
                {
                    bool success = FindNextChangeNotification(h);
                    CPY_ASSERT(success);
                    if (!success)
                        break;
                }

                auto result = WaitForSingleObject(h, m_fileWatchPollingRate <= 0 ? INFINITE : (DWORD)m_fileWatchPollingRate);
                CPY_ASSERT(result != WAIT_FAILED);
                if (result != WAIT_OBJECT_0)
                    break;

                //again try and listen for more dir changes.
                m_fileThreadQueue.push(msg);
            }
#else
    #error "Platform not supported"
#endif
            break;
        case MessageType::Exit:
        default:
            active = false;
        }
    }
}


ShaderHandle ShaderService::compileShader(const ShaderDesc& desc)
{
    std::unique_lock lock(m_shadersMutex);
    ShaderHandle outHandle;
    auto& data = m_shaders.allocate(outHandle);
    data.type = desc.type;
    data.debugName = desc.debugName;
    data.filename = desc.path;
    return outHandle;
}

ShaderHandle ShaderService::compileInlineShader(const ShaderInlineDesc& desc)
{
    return ShaderHandle();
}

GpuPipelineHandle ShaderService::createComputePipeline(const ComputePipelineDesc& desc)
{
    return GpuPipelineHandle();
}

IShaderService* IShaderService::create(const ShaderServiceDesc& desc)
{
    return new ShaderService(desc);
}

}
