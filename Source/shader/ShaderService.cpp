#include "ShaderService.h" 
#include <coalpy.files/Utils.h>
#include <coalpy.core/Assert.h>
#include <coalpy.core/String.h>
#include <coalpy.render/IShaderDb.h>
#include <iostream>
#include <string>

#ifdef _WIN32 
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace coalpy
{

ShaderService::ShaderService(const ShaderServiceDesc& desc)
: m_db(desc.db)
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
#if 0
#ifdef _WIN32 
    bool active = true;
    std::cout << "opening " << m_rootDir.c_str() << std::endl;
    HANDLE dirHandle = CreateFileA(
        m_rootDir.c_str(),
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS /*| FILE_FLAG_OVERLAPPED*/, NULL);

    CPY_ASSERT_FMT(dirHandle != INVALID_HANDLE_VALUE, "Could not open directory \"%s\" for file watching service.", m_rootDir.c_str());

    //OVERLAPPED overlapped;
    FILE_NOTIFY_INFORMATION infos[1024];
    while (active)
    {
        Message msg;
        m_fileThreadQueue.waitPop(msg);
        switch (msg.type)
        {
        case MessageType::ListenToDirectories:
            {
                while (true)
                {
                    DWORD bytesReturned;
                    bool result = ReadDirectoryChangesW(
                        dirHandle, (LPVOID)&infos, sizeof(infos), TRUE, FILE_NOTIFY_CHANGE_LAST_WRITE, &bytesReturned, NULL, NULL);
                    if (result)
                    {
                        std::cout << "Changes detected" << bytesReturned << std::endl;
                        FILE_NOTIFY_INFORMATION* curr = infos;
                        while (curr != nullptr)
                        {
                            std::wstring wfilename;
                            wfilename.assign(curr->FileName, curr->FileNameLength / sizeof(wchar_t));
                            std::string filename = ws2s(wfilename);
                            std::cout << filename << std::endl;
                            curr = curr->NextEntryOffset == 0 ? nullptr : (FILE_NOTIFY_INFORMATION*)((char*)curr + curr->NextEntryOffset);
                        }
                    }
                    else
                    {
                        std::cout << "Error reading directory changes" << std::endl;
                    }
                }
            }
            break;
        case MessageType::Exit:
        default:
            {
                active = false;
            }
        }
    }

    if (dirHandle != INVALID_HANDLE_VALUE)
        CloseHandle(dirHandle);
#else
    #error "Platform not supported"
#endif
#endif
}

IShaderService* IShaderService::create(const ShaderServiceDesc& desc)
{
    return new ShaderService(desc);
}

}
