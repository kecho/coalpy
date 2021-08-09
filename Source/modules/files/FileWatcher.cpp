#include <coalpy.core/Assert.h>
#include <coalpy.core/String.h>
#include <coalpy.tasks/ThreadQueue.h>
#include <iostream>
#include <string>
#include <shared_mutex>
#include "FileWatcher.h"

#ifdef _WIN32 
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#define WATCH_SERVICE_DEBUG_OUTPUT 0

namespace coalpy
{

enum class FileWatchMessageType
{
    ListenToDirectories,
    Exit
};

struct FileWatchMessage
{
    FileWatchMessageType type;
};

typedef void* WatchHandle;

struct FileWatchState
{
public:
    std::thread thread;
    std::shared_mutex fileWatchMutex;
    std::set<std::string> directoriesSet;
    std::vector<std::string> directories;
    std::vector<bool> waitResults;
    std::vector<WatchHandle> handles;
    std::set<IFileWatchListener*> listeners;
    ThreadQueue<FileWatchMessage> queue;

#ifdef _WIN32 
    std::vector<HANDLE> events;
#endif

};

namespace
{

#ifdef _WIN32 
bool findResults(
    const std::string& rootDir,
    HANDLE dirHandle,
    OVERLAPPED& overlapped,
    FILE_NOTIFY_INFORMATION* infos,
    int millisecondsToWait,
    std::set<std::string>& caughtFiles)
{
    DWORD bytesReturned = 0;

    auto hasOverlapped = GetOverlappedResult(dirHandle, &overlapped, &bytesReturned, FALSE);
    if (!hasOverlapped)
        return false;

    FILE_NOTIFY_INFORMATION* curr = bytesReturned ? infos : nullptr;
    while (curr != nullptr)
    {
        if (curr->Action == FILE_ACTION_MODIFIED)
        {
            std::wstring wfilename;
            wfilename.assign(curr->FileName, curr->FileNameLength / sizeof(wchar_t));
            std::string filename = ws2s(wfilename);
            caughtFiles.insert(rootDir + "/" + filename);
        }
        curr = curr->NextEntryOffset == 0 ? nullptr : (FILE_NOTIFY_INFORMATION*)((char*)curr + curr->NextEntryOffset);
    }
    
    return true;
}
#endif

bool waitListenForDirs(FileWatchState& state, int millisecondsToWait)
{
    std::set<std::string> caughtFiles;

#ifdef _WIN32 

    if (state.handles.empty())
        return true;

    std::shared_lock lock(state.fileWatchMutex);
    for (int i = 0; i < (int)state.handles.size(); ++i)
    {
        #if WATCH_SERVICE_DEBUG_OUTPUT
            std::cout << "polling filewatch: " << m_state.directories[i] << std::endl;
        #endif

        auto dirHandle = (HANDLE)state.handles[i];
        auto event = state.events[i];
        OVERLAPPED overlapped;
        overlapped.hEvent = event;
        FILE_NOTIFY_INFORMATION infos[1024] = {};

        {
            if (!state.waitResults[i])
            {
                DWORD bytesReturned = 0;
                bool result = ReadDirectoryChangesW(
                    dirHandle, (LPVOID)&infos, sizeof(infos), TRUE, FILE_NOTIFY_CHANGE_LAST_WRITE, &bytesReturned,
                    &overlapped, NULL);
                state.waitResults[i] = true;
                CPY_ASSERT_FMT(result, "Failed watching directory \"%s\"", state.directories[i].c_str());

                if (!result)
                    return false;
            }

            if (state.waitResults[i])
            {
                auto waitResult = WaitForSingleObject(overlapped.hEvent, i == 0 ? millisecondsToWait : 0);
                if (waitResult == WAIT_TIMEOUT)
                    continue;
            }

            if (!findResults(
                state.directories[i],
                dirHandle,
                overlapped,
                infos,
                i == 0 ? millisecondsToWait : 0,
                caughtFiles))
                    return false;

            state.waitResults[i] = false;
        }
    }
#endif

    if (!caughtFiles.empty())
    {
        for (auto* listener : state.listeners)
            listener->onFilesChanged(caughtFiles);
    }

    return true;
}

}

FileWatcher::FileWatcher(const FileWatchDesc& desc)
: m_desc(desc)
{
}

FileWatcher::~FileWatcher()
{
    CPY_ASSERT_MSG(m_state == nullptr, "Destroying file watcher without calling stop().");
}

void FileWatcher::start()
{
    CPY_ASSERT(m_state == nullptr);
    m_state = new FileWatchState;

    m_state->thread = std::thread(
        [this]()
    {
        onFileListening();
    });

    FileWatchMessage msg;
    msg.type = FileWatchMessageType::ListenToDirectories;
    m_state->queue.push(msg);
}

void FileWatcher::stop()
{
    if (!m_state)
        return;

    FileWatchMessage msg;
    msg.type = FileWatchMessageType::Exit;
    m_state->queue.push(msg);
    m_state->thread.join();

#ifdef _WIN32 
    for (auto h : m_state->handles)
        CloseHandle((HANDLE)h);

    for (auto e : m_state->events)
        CloseHandle((HANDLE)e);
#endif
    
    delete m_state;
    m_state = nullptr;
}

void FileWatcher::addDirectory(const char* directory)
{
    std::unique_lock lock(m_state->fileWatchMutex);
    std::string dirStr(directory);
    auto it = m_state->directoriesSet.insert(dirStr);
    if (!it.second)
        return;

#ifdef _WIN32 
    #if WATCH_SERVICE_DEBUG_OUTPUT
        std::cout << "opening " << dirStr << std::endl;
    #endif


    HANDLE dirHandle = CreateFileA(
        directory,
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, NULL);

    CPY_ASSERT_FMT(dirHandle != INVALID_HANDLE_VALUE, "Could not open directory \"%s\" for file watching service.", directory);
    if (dirHandle == INVALID_HANDLE_VALUE)
        return;

    m_state->directories.push_back(dirStr);
    m_state->handles.push_back((WatchHandle)dirHandle);
    m_state->waitResults.push_back(false);
    m_state->events.push_back(CreateEvent(NULL, TRUE, TRUE, NULL));
#endif
}

void FileWatcher::addListener(IFileWatchListener* listener)
{
    CPY_ASSERT(m_state);
    std::unique_lock lock(m_state->fileWatchMutex);
    m_state->listeners.insert(listener);
}

void FileWatcher::removeListener(IFileWatchListener* listener)
{
    if (m_state == nullptr)
        return;

    std::unique_lock lock(m_state->fileWatchMutex);
    auto it = m_state->listeners.find(listener);
    CPY_ASSERT(it != m_state->listeners.end());
    if (it == m_state->listeners.end())
        return;
    m_state->listeners.erase(it);
}

void FileWatcher::onFileListening()
{
    bool active = true;

    while (active)
    {
        FileWatchMessage msg;
        bool receivedMessage = m_state->queue.waitPopUntil(msg, m_desc.pollingRateMS);
        if (!receivedMessage)
        {
            FileWatchMessage msg;
            msg.type = FileWatchMessageType::ListenToDirectories;
            m_state->queue.push(msg);
            continue;
        }

        switch (msg.type)
        {
        case FileWatchMessageType::ListenToDirectories:
            {
                active = waitListenForDirs(*m_state, m_desc.pollingRateMS);
                FileWatchMessage msg;
                msg.type = FileWatchMessageType::ListenToDirectories;
                m_state->queue.push(msg);
            }
            break;
        case FileWatchMessageType::Exit:
        default:
            {
                active = false;
            }
        }
    }
}

IFileWatcher* IFileWatcher::create(const FileWatchDesc& desc)
{
    return new FileWatcher(desc);
}

}
