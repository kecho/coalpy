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

private:
    void onFileListening();
    IShaderDb*   m_db;

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
    int m_fileWatchPollingRate;

    std::string m_rootDir;
};


}
