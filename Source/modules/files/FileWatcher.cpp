#include <coalpy.files/FileWatcher.h>
#include <coalpy.core/Assert.h>
#include <coalpy.core/String.h>
#include <coalpy.tasks/ThreadQueue.h>
#include <iostream>
#include <string>

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

struct FileWatchState
{
    std::thread thread;
    ThreadQueue<FileWatchMessage> queue;
};

namespace
{

#ifdef _WIN32 
bool findResults(
    FileWatchState& state,
    HANDLE dirHandle,
    OVERLAPPED& overlapped,
    FILE_NOTIFY_INFORMATION* infos,
    int pollingRateMs,
    std::set<std::string>& caughtFiles)
{
    bool foundResults = false;
    DWORD bytesReturned = 0;
    while (!foundResults)
    {
        auto waitResult = WaitForSingleObject(overlapped.hEvent, pollingRateMs);
        if (waitResult == WAIT_TIMEOUT)
        {
#if WATCH_SERVICE_DEBUG_OUTPUT
            std::cout << "timed out" << std::endl;
#endif
            FileWatchMessage newMsg;
            bool hasMessage = false;
            state.queue.acquireThread();
            hasMessage = state.queue.unsafePop(newMsg);
            state.queue.releaseThread();
            if (hasMessage && newMsg.type == FileWatchMessageType::Exit)
            {
                return false;
            }
            continue;
        }

        auto hasOverlapped = GetOverlappedResult(dirHandle, &overlapped, &bytesReturned, FALSE);
        if (!hasOverlapped)
            continue;

        foundResults = true;
        FILE_NOTIFY_INFORMATION* curr = bytesReturned ? infos : nullptr;
        while (curr != nullptr)
        {
            std::wstring wfilename;
            wfilename.assign(curr->FileName, curr->FileNameLength / sizeof(wchar_t));
            std::string filename = ws2s(wfilename);
            caughtFiles.insert(filename);
            curr = curr->NextEntryOffset == 0 ? nullptr : (FILE_NOTIFY_INFORMATION*)((char*)curr + curr->NextEntryOffset);
        }
    }
    return true;
}
#endif

void waitListenForDirs(
    FileWatchState& state,
    const char* rootDir,
    OnFileChangedFn onChangeFn,
    int pollingRateMs)
{
#ifdef _WIN32 
    #if WATCH_SERVICE_DEBUG_OUTPUT
        std::cout << "opening " << rootDir << std::endl;
    #endif

    HANDLE dirHandle = CreateFileA(
        rootDir,
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, NULL);

    CPY_ASSERT_FMT(dirHandle != INVALID_HANDLE_VALUE, "Could not open directory \"%s\" for file watching service.", rootDir);
    if (dirHandle == INVALID_HANDLE_VALUE)
        return;

    OVERLAPPED overlapped;
    overlapped.hEvent = CreateEvent(NULL, TRUE, TRUE, NULL);
    FILE_NOTIFY_INFORMATION infos[1024];

    bool listeningToDirectories = true;
    while (listeningToDirectories)
    {
        DWORD bytesReturned = 0;
        bool result = ReadDirectoryChangesW(
                dirHandle, (LPVOID)&infos, sizeof(infos), TRUE, FILE_NOTIFY_CHANGE_LAST_WRITE, &bytesReturned,
                &overlapped, NULL);

        if (!result)
        {
            CPY_ASSERT_FMT(false, "Failed watching directory \"%s\"", rootDir);
            listeningToDirectories = false;
            continue;
        }

        std::set<std::string> caughtFiles;
        listeningToDirectories = findResults(
            state,
            dirHandle,
            overlapped,
            infos,
            pollingRateMs,
            caughtFiles);
        if (!caughtFiles.empty())
            onChangeFn(caughtFiles);
    }
    
    CloseHandle(overlapped.hEvent);
    if (dirHandle != INVALID_HANDLE_VALUE)
        CloseHandle(dirHandle);

#endif
}

}

FileWatcher::~FileWatcher()
{
    CPY_ASSERT_MSG(m_state == nullptr, "Destroying file watcher without calling stop().");
}

void FileWatcher::start(
    const char* directory,
    OnFileChangedFn onFileChanged,
    int pollingRateMs)
{
    CPY_ASSERT(m_state == nullptr);
    m_rootDir = directory;
    m_onFileChangedFn = onFileChanged;
    m_pollingRateMs = pollingRateMs;
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
    delete m_state;
    m_state = nullptr;
}

void FileWatcher::onFileListening()
{
    bool active = true;

    while (active)
    {
        FileWatchMessage msg;
        m_state->queue.waitPop(msg);
        bool listeningToDirectories = false;
        switch (msg.type)
        {
        case FileWatchMessageType::ListenToDirectories:
            {
                waitListenForDirs(
                    *m_state,
                    m_rootDir.c_str(),
                    m_onFileChangedFn,
                    m_pollingRateMs);
                active = false;
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


}
