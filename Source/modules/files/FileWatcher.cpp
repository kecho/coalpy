#include <coalpy.core/Assert.h>
#include <coalpy.core/String.h>
#include <coalpy.tasks/ThreadQueue.h>
#include <iostream>
#include <string>
#include <sstream>
#include <chrono>
#include <thread>
#include <shared_mutex>
#include "FileWatcher.h"

#ifdef _WIN32 
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#elif defined(__linux__)
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <linux/limits.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <sys/inotify.h>
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

#ifdef _WIN32
typedef void* WatchHandle;

struct WinFileWatch
{
    bool waitResult;
    HANDLE event;
    OVERLAPPED overlapped;
    char payload[sizeof(FILE_NOTIFY_INFORMATION) + 1024];
};

#elif defined(__linux__)
typedef int WatchHandle;
#endif


struct FileWatchState
{
public:
    std::thread thread;
    std::shared_mutex fileWatchMutex;
    std::set<std::string> directoriesSet;
    std::vector<std::string> directories;
    std::vector<WatchHandle> handles;
    std::set<IFileWatchListener*> listeners;
    ThreadQueue<FileWatchMessage> queue;

#ifdef _WIN32 
    std::vector<WinFileWatch> watches;
#elif defined(__linux__)
    int inotifyInstance;
#endif

};

namespace
{

#ifdef _WIN32 
void findResults(
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
        return;

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
#if WATCH_SERVICE_DEBUG_OUTPUT
        else
        {
            std::cout << "Action not captured" << std::endl;
        }
#endif
        curr = curr->NextEntryOffset == 0 ? nullptr : (FILE_NOTIFY_INFORMATION*)((char*)curr + curr->NextEntryOffset);
    }
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
            std::cout << "polling filewatch: " << state.directories[i] << std::endl;
        #endif

        auto dirHandle = (HANDLE)state.handles[i];
        WinFileWatch& fileWatch = state.watches[i];
        auto event = fileWatch.event;
        OVERLAPPED& overlapped = fileWatch.overlapped;
        {
            if (!fileWatch.waitResult)
            {
                overlapped = {};
                overlapped.hEvent = event;
                DWORD bytesReturned = 0;
                bool result = ReadDirectoryChangesW(
                    dirHandle, (LPVOID)&fileWatch.payload, sizeof(fileWatch.payload), TRUE, FILE_NOTIFY_CHANGE_CREATION | FILE_NOTIFY_CHANGE_LAST_WRITE, &bytesReturned,
                    &overlapped, NULL);
                fileWatch.waitResult = true;
                CPY_ASSERT_FMT(result, "Failed watching directory \"%s\"", state.directories[i].c_str());

                if (!result)
                    return false;
            }

            if (fileWatch.waitResult)
            {
                auto waitResult = WaitForSingleObject(overlapped.hEvent, i == 0 ? millisecondsToWait : 0);
                if (waitResult == WAIT_TIMEOUT)
                    continue;
            }

            findResults(
                state.directories[i],
                dirHandle,
                overlapped,
                reinterpret_cast<FILE_NOTIFY_INFORMATION*>(fileWatch.payload),
                i == 0 ? millisecondsToWait : 0,
                caughtFiles);
            

            fileWatch.waitResult = false;
        }
    }
#elif defined(__linux__)
    const size_t fileName = 64;
    const size_t maxEvents = 512;
    const size_t buffLen = maxEvents * (sizeof(struct inotify_event) + fileName);

    char eventBuffer[buffLen];
    ssize_t bytesRead = ::read(state.inotifyInstance, eventBuffer, buffLen);
    ssize_t i = 0;

    while (i < bytesRead)
    {
        struct inotify_event *event = (struct inotify_event *) &eventBuffer[i];
        if(event->len)
        {
            if ( event->mask & IN_MODIFY )
            {
                //find the watch descriptor
                std::stringstream ss;
                for (int k = 0; k < (int)state.handles.size(); ++k)
                {
                    auto wd = state.handles[k];
                    if (wd != event->wd)
                        continue;
                    
                    const auto& dirName = state.directories[k];
                    ss << dirName;
                    if (!dirName.empty() && dirName[dirName.size() - 1] != '/')
                        ss << '/';
                    break;
                }
                ss << event->name;
                caughtFiles.insert(ss.str());
            }
        }
        i += sizeof(struct inotify_event) + event->len;
    }
#endif

    if (!caughtFiles.empty())
    {
        for (auto* listener : state.listeners)
            listener->onFilesChanged(caughtFiles);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(millisecondsToWait));

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

#ifdef __linux__
    m_state->inotifyInstance = ::inotify_init();
    CPY_ASSERT(m_state->inotifyInstance != -1);
    int hresult = fcntl(m_state->inotifyInstance, F_SETFL, O_NONBLOCK);
    CPY_ASSERT(hresult == 0);
#endif

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

    for (auto& w : m_state->watches)
        CloseHandle((HANDLE)w.event);
#elif defined(__linux__)
    for (auto h : m_state->handles)
        inotify_rm_watch(m_state->inotifyInstance, (int)h);
    ::close(m_state->inotifyInstance);
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

    #if WATCH_SERVICE_DEBUG_OUTPUT
        std::cout << "opening " << dirStr << std::endl;
    #endif

#ifdef _WIN32 

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

    m_state->watches.emplace_back();
    WinFileWatch& fileWatch = m_state->watches.back();
    fileWatch.waitResult = false;
    fileWatch.event = CreateEvent(NULL, TRUE, TRUE, NULL);
    fileWatch.overlapped = {};
    fileWatch.overlapped.hEvent = fileWatch.event;
#elif defined(__linux__)
    m_state->directories.push_back(dirStr);
    int wd = inotify_add_watch(
        m_state->inotifyInstance, directory, IN_MODIFY | IN_CREATE | IN_DELETE);
    m_state->handles.push_back((WatchHandle)wd);
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
