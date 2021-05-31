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

#define WATCH_SERVICE_DEBUG_OUTPUT 0

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
#ifdef _WIN32 
    bool active = true;
#if WATCH_SERVICE_DEBUG_OUTPUT
    std::cout << "opening " << m_rootDir.c_str() << std::endl;
#endif
    HANDLE dirHandle = CreateFileA(
        m_rootDir.c_str(),
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, NULL);

    CPY_ASSERT_FMT(dirHandle != INVALID_HANDLE_VALUE, "Could not open directory \"%s\" for file watching service.", m_rootDir.c_str());

/*
    std::cout << "Changes detected" << bytesReturned << std::endl;
    FILE_NOTIFY_INFORMATION* curr = bytesReturned ? infos : nullptr;
    while (curr != nullptr)
    {
        std::wstring wfilename;
        wfilename.assign(curr->FileName, curr->FileNameLength / sizeof(wchar_t));
        std::string filename = ws2s(wfilename);
        std::cout << filename << std::endl;
        curr = curr->NextEntryOffset == 0 ? nullptr : (FILE_NOTIFY_INFORMATION*)((char*)curr + curr->NextEntryOffset);
    }

    bytesReturned = 0;
}
else
{
    std::cout << "Error querying changes" << std::endl;
}
*/

    OVERLAPPED overlapped;
    overlapped.hEvent = CreateEvent(NULL, TRUE, TRUE, NULL);
    FILE_NOTIFY_INFORMATION infos[1024];
    while (active)
    {
        Message msg;
        m_fileThreadQueue.waitPop(msg);
        bool listeningToDirectories = false;
        switch (msg.type)
        {
        case MessageType::ListenToDirectories:
            {
                listeningToDirectories = true;
                while (listeningToDirectories)
                {
                    DWORD bytesReturned;
                    bool result = ReadDirectoryChangesW(
                            dirHandle, (LPVOID)&infos, sizeof(infos), TRUE, FILE_NOTIFY_CHANGE_LAST_WRITE, &bytesReturned,
                            &overlapped, NULL);

                    if (!result)
                    {
                        CPY_ASSERT_FMT(false, "Failed watching directory \"%s\"", m_rootDir.c_str());
                        listeningToDirectories = false;
                        continue;
                    }

                    bool foundResults = false;
                    while (!foundResults)
                    {
                        auto waitResult = WaitForSingleObject(overlapped.hEvent, 1000);
                        if (waitResult == WAIT_TIMEOUT)
                        {
#if WATCH_SERVICE_DEBUG_OUTPUT
                            std::cout << "timed out" << std::endl;
#endif
                            Message newMsg;
                            bool hasMessage = false;
                            m_fileThreadQueue.acquireThread();
                            hasMessage = m_fileThreadQueue.unsafePop(newMsg);
                            m_fileThreadQueue.releaseThread();
                            if (hasMessage && newMsg.type == MessageType::Exit)
                            {
                                foundResults = true;
                                listeningToDirectories = false;
                                active = false;
                            }
                            continue;
                        }
                        auto hasOverlapped = GetOverlappedResult(dirHandle, &overlapped, &bytesReturned, FALSE);
                        if (hasOverlapped)
                        {
#if WATCH_SERVICE_DEBUG_OUTPUT
                            std::cout << "has overlapped waited!" << bytesReturned << std::endl;
#endif
                            foundResults = true;
                        }
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

    CloseHandle(overlapped.hEvent);
    if (dirHandle != INVALID_HANDLE_VALUE)
        CloseHandle(dirHandle);
#else
    #error "Platform not supported"
#endif
}

IShaderService* IShaderService::create(const ShaderServiceDesc& desc)
{
    return new ShaderService(desc);
}

}
