#include "FileSystem.h"
#include <coalpy.tasks/ITaskSystem.h>
#include <coalpy.core/Assert.h>

#ifdef _WIN32 
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace coalpy
{

#ifdef _WIN32
class FileSystemInternal
{
public:

    enum {
        bufferSize = 16 * 1024 //16kb buffer size
    };

    struct WindowsFile
    {
        HANDLE h;
        unsigned int fileSize;
        OVERLAPPED overlapped;
        char buffer[bufferSize];
    };

    inline bool valid(FileSystem::OpaqueFileHandle h) const
    {
        return h != nullptr;
    }

    FileSystem::OpaqueFileHandle openFile(const char* filename, FileSystem::RequestType request)
    {
        HANDLE h = CreateFileA(
            filename, //file name
            request == FileSystem::RequestType::Read ? GENERIC_READ : GENERIC_WRITE, //dwDesiredAccess
            0u, //dwShareMode
            NULL, //lpSecurityAttributes
            request == FileSystem::RequestType::Read ? OPEN_EXISTING : CREATE_ALWAYS,//dwCreationDisposition
            FILE_FLAG_OVERLAPPED, //dwFlagsAndAttributes
            NULL); //template attribute

        if (h == INVALID_HANDLE_VALUE)
            return nullptr;

        auto* wf = new WindowsFile;
        wf->h = h;
        wf->fileSize = GetFileSize(wf->h, NULL);
        wf->overlapped = {};
        wf->overlapped.hEvent = CreateEvent(
            NULL, //default security attribute
            TRUE, //manual reset event
            FALSE, //initial state = signaled
            NULL);//unnamed

        return (FileSystem::OpaqueFileHandle)wf;
    }

    bool readBytes(FileSystem::OpaqueFileHandle h, char*& outputBuffer, int& bytesRead, bool& isEof)
    {
        CPY_ASSERT(h != nullptr);
        isEof = false;
        if (h == nullptr)
            return false;

        auto* wf = (WindowsFile*)h;
        CPY_ASSERT(wf->h != INVALID_HANDLE_VALUE);

        DWORD dwordBytesRead;
        bool result = ReadFile(
            wf->h,
            wf->buffer,
            bufferSize,
            &dwordBytesRead,
            &wf->overlapped);

        if (!result)
        {
            auto dwError = GetLastError();
            switch (dwError)
            {
            case ERROR_HANDLE_EOF:
                {
                    isEof = true;
                    result = true;
                    break;
                }
            case ERROR_IO_PENDING:
                {
                    bool overlappedSuccess = GetOverlappedResult(wf->h,
                                                  &wf->overlapped,
                                                  &dwordBytesRead,
                                                  TRUE); 
                    if (!overlappedSuccess)
                    {
                        switch (dwError = GetLastError())
                        {
                        case ERROR_HANDLE_EOF:  
                            isEof = true;
                            result = true;
                            break;
                        case ERROR_IO_INCOMPLETE:
                            result = true;
                            break;
                        default:
                            result = false;
                            break;
                        }
                    }
                    else
                    {
                        result = true;
                        ResetEvent(wf->overlapped.hEvent);
                    }

                    break;
                }
            default:
                //file IO issue occured here
                result = false;
            }
        }

        if (result)
            wf->overlapped.Offset += dwordBytesRead;
        if (wf->overlapped.Offset >= wf->fileSize)
            isEof = true;

        bytesRead = (int)dwordBytesRead;
        outputBuffer = wf->buffer;
        return result;
    }

    void close(FileSystem::OpaqueFileHandle h)
    {
        CPY_ASSERT(h != nullptr);
        auto* wf = (WindowsFile*)h;
        CPY_ASSERT(wf->h != INVALID_HANDLE_VALUE);
        CloseHandle(wf->h);
        CloseHandle(wf->overlapped.hEvent);
        delete wf;
    }
};
#else
    #error "Platform not supported"
#endif

FileSystem::FileSystem(const FileSystemDesc& desc)
: m_desc(desc)
, m_ts(*desc.taskSystem)
, m_internalFs(*(new FileSystemInternal))
{
    
}

FileSystem::~FileSystem()
{
    delete &m_internalFs;
}

AsyncFileHandle FileSystem::read(const FileReadRequest& request)
{
    CPY_ASSERT_MSG(request.doneCallback, "File read request must provide a done callback.");

    AsyncFileHandle asyncHandle;
    Task task;
    
    {
        std::unique_lock lock(m_requestsMutex);
        Request*& requestData = m_requests.allocate(asyncHandle);
        requestData = new Request();
        requestData->type = RequestType::Read;
        requestData->filename = request.path;
        requestData->callback = request.doneCallback;
        requestData->opaqueHandle = {};
        requestData->fileStatus = FileStatus::Reading;
        requestData->task = m_ts.createTask(TaskDesc([this](TaskContext& ctx)
        {
            auto* requestData = (Request*)ctx.data;
            {
                requestData->fileStatus = FileStatus::Opening;
                FileReadResponse response;
                response.status = FileStatus::Opening;
                requestData->callback(response);
            }
            requestData->opaqueHandle = m_internalFs.openFile(requestData->filename.c_str(), RequestType::Read);
            if (!m_internalFs.valid(requestData->opaqueHandle))
            {
                {
                    requestData->fileStatus = FileStatus::OpenFail;
                    FileReadResponse response;
                    response.status = FileStatus::OpenFail;
                    requestData->callback(response);
                }
                return;
            }

            char* output = nullptr;
            int bytesRead = 0;
            bool isEof = false;
            bool successRead = false;
            while (!isEof)
            {
                TaskUtil::yieldUntil([&output, &bytesRead, &isEof, requestData, &successRead, this]() {
                    successRead = m_internalFs.readBytes(requestData->opaqueHandle, output, bytesRead, isEof);
                });

                if (!successRead)
                {
                    {
                        requestData->fileStatus = FileStatus::ReadingFail;
                        FileReadResponse response;
                        response.status = FileStatus::ReadingFail;
                        requestData->callback(response);
                    }
                    return;
                }
            }

            {
                requestData->fileStatus = FileStatus::ReadingSuccess;
                FileReadResponse response;
                response.status = FileStatus::ReadingSuccess;
                requestData->callback(response);
            }

        }), requestData);
        task = requestData->task;
    }

    m_ts.execute(task);
    return asyncHandle;
}

AsyncFileHandle FileSystem::write(const FileWriteRequest& request)
{
    return AsyncFileHandle();
}


void FileSystem::wait(AsyncFileHandle handle)
{
    Task task;
    {
        std::unique_lock lock(m_requestsMutex);
        Request* r = m_requests[handle];
        task = r->task;
    }

    m_ts.wait(task);
}

bool FileSystem::readStatus (AsyncFileHandle handle, FileReadResponse& response)
{
    return false;
}

bool FileSystem::writeStatus(AsyncFileHandle handle, FileWriteResponse& response)
{
    return false;
}

void FileSystem::closeHandle(AsyncFileHandle handle)
{
}

IFileSystem* IFileSystem::create(const FileSystemDesc& desc)
{
    return new FileSystem(desc);
}

}
