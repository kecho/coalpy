#include "FileSystem.h"
#include <coalpy.tasks/ITaskSystem.h>
#include <coalpy.core/Assert.h>

#ifdef _WIN32 
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <coalpy.core/ClTokenizer.h>
#endif

namespace coalpy
{

struct PathInfo
{
    std::vector<std::string> directoryList;
    std::string path;
    std::string filename;
};

#ifdef _WIN32
namespace InternalFileSystem
{
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

    bool valid(FileSystem::OpaqueFileHandle h)
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
            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, //dwFlagsAndAttributes
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
                            result = false;
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

    bool writeBytes(FileSystem::OpaqueFileHandle h, const char* buffer, int bufferSize)
    {
        CPY_ASSERT(h != nullptr);
        if (h == nullptr)
            return false;

        auto* wf = (WindowsFile*)h;
        CPY_ASSERT(wf->h != INVALID_HANDLE_VALUE);

        DWORD dwordBytesWritten;
        bool result = WriteFile(
            wf->h,
            buffer,
            (DWORD)bufferSize,
            &dwordBytesWritten,
            &wf->overlapped);

        if (!result)
        {
            auto dwError = GetLastError();
            switch (dwError)
            {
            case ERROR_IO_PENDING:
                {
                    bool overlappedSuccess = GetOverlappedResult(wf->h,
                                                  &wf->overlapped,
                                                  &dwordBytesWritten,
                                                  TRUE); 
                    if (!overlappedSuccess)
                    {
                        result = false;
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

        if (bufferSize != dwordBytesWritten)
            return false;

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

    void fixStringPath(std::string& str)
    {
        for (auto& c : str)
        {
            if (c == '/')
                c = '\\';
        }
    }

    void getPathInfo(const std::string& filePath, PathInfo& pathInfo)
    {
        pathInfo = {};
        auto dirCandidates = ClTokenizer::splitString(filePath, '\\');
        for (auto d : dirCandidates)
            if (d != "")
                pathInfo.directoryList.push_back(d);

        if (pathInfo.directoryList.empty())
            return;

        pathInfo.filename = pathInfo.directoryList.back();
        pathInfo.directoryList.pop_back();
    
        std::stringstream ss;
        for (auto& d : pathInfo.directoryList)
        {
            ss << d  << "\\";
        }
        pathInfo.path = std::move(ss.str());
    }

    bool createDirectory(const char* str)
    {
        return CreateDirectoryA(str, nullptr);
    }

    bool deleteDirectory(const char* str)
    {
        return RemoveDirectoryA(str);
    }

    bool deleteFile(const char* str)
    {
        return DeleteFile(str);
    }

    void getAttributes(const std::string& dirName_in, bool& exists, bool& isDir)
    {
        exists = false;
        isDir = false;
        DWORD ftyp = GetFileAttributesA(dirName_in.c_str());
        if (ftyp == INVALID_FILE_ATTRIBUTES)
        {
            exists = false;
            isDir = false;
            return;
        }
        
        exists = true;
        if (ftyp & FILE_ATTRIBUTE_DIRECTORY)
        {
            isDir = true;
            return;
        }
        
        return;
    }

    bool carvePath(const std::string& path, bool lastIsFile = true)
    {
        bool exists, isDir;
        getAttributes(path, exists, isDir);
        if (exists)
            return lastIsFile ? !isDir : isDir;

        //ok so the path doesnt really exists, lets carve it.
        PathInfo pathInfo;
        getPathInfo(path, pathInfo);
        if (pathInfo.filename == "")
            return false;

        if (!lastIsFile)
            pathInfo.directoryList.push_back(pathInfo.filename);

        std::stringstream ss;
        for (auto& d : pathInfo.directoryList)
        {
            ss << d << "\\";
            auto currentPath = ss.str();
            getAttributes(currentPath, exists, isDir);
            if (exists && !isDir)
                return false;

            if (!exists && !createDirectory(currentPath.c_str()))
                return false;
        }

        return true;
    }
};
#else
    #error "Platform not supported"
#endif

FileSystem::FileSystem(const FileSystemDesc& desc)
: m_desc(desc)
, m_ts(*desc.taskSystem)
{
    
}

FileSystem::~FileSystem()
{
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
        requestData->readCallback = request.doneCallback;
        requestData->opaqueHandle = {};
        requestData->fileStatus = FileStatus::Reading;
        requestData->task = m_ts.createTask(TaskDesc([this](TaskContext& ctx)
        {
            auto* requestData = (Request*)ctx.data;
            {
                requestData->fileStatus = FileStatus::Opening;
                FileReadResponse response;
                response.status = FileStatus::Opening;
                requestData->readCallback(response);
            }
            requestData->opaqueHandle = InternalFileSystem::openFile(requestData->filename.c_str(), RequestType::Read);
            if (!InternalFileSystem::valid(requestData->opaqueHandle))
            {
                {
                    requestData->fileStatus = FileStatus::OpenFail;
                    FileReadResponse response;
                    response.status = FileStatus::OpenFail;
                    requestData->readCallback(response);
                }
                return;
            }

            struct ReadState {
                char* output = nullptr;
                int bytesRead = 0;
                bool isEof = false;
                bool successRead = false;
            } readState;
            while (!readState.isEof)
            {
                TaskUtil::yieldUntil([&readState, requestData]() {
                    readState.successRead = InternalFileSystem::readBytes(
                        requestData->opaqueHandle, readState.output, readState.bytesRead, readState.isEof);
                });

                if (!readState.successRead)
                {
                    {
                        requestData->fileStatus = FileStatus::ReadingFail;
                        FileReadResponse response;
                        response.status = FileStatus::ReadingFail;
                        requestData->readCallback(response);
                    }
                    return;
                }
            }

            {
                requestData->fileStatus = FileStatus::ReadingSuccess;
                FileReadResponse response;
                response.status = FileStatus::ReadingSuccess;
                requestData->readCallback(response);
            }

        }), requestData);
        task = requestData->task;
    }

    m_ts.execute(task);
    return asyncHandle;
}

AsyncFileHandle FileSystem::write(const FileWriteRequest& request)
{
    CPY_ASSERT_MSG(request.doneCallback, "File read request must provide a done callback.");

    AsyncFileHandle asyncHandle;
    Task task;
    
    {
        std::unique_lock lock(m_requestsMutex);
        Request*& requestData = m_requests.allocate(asyncHandle);
        requestData = new Request();
        requestData->type = RequestType::Write;
        requestData->filename = request.path;
        requestData->writeCallback = request.doneCallback;
        requestData->opaqueHandle = {};
        requestData->fileStatus = FileStatus::Writting;
        requestData->writeBuffer = request.buffer;
        requestData->writeSize = request.size;
        requestData->task = m_ts.createTask(TaskDesc([this](TaskContext& ctx)
        {
            auto* requestData = (Request*)ctx.data;
            {
                requestData->fileStatus = FileStatus::Opening;
                FileWriteResponse response;
                response.status = FileStatus::Opening;
                requestData->writeCallback(response);
            }
            requestData->opaqueHandle = InternalFileSystem::openFile(requestData->filename.c_str(), RequestType::Write);
            if (!InternalFileSystem::valid(requestData->opaqueHandle))
            {
                {
                    requestData->fileStatus = FileStatus::OpenFail;
                    FileWriteResponse response;
                    response.status = FileStatus::OpenFail;
                    requestData->writeCallback(response);
                }
                return;
            }

            struct WriteState {
                const char* buffer;
                int bufferSize;
                bool successWrite;
            } writeState = { requestData->writeBuffer, requestData->writeSize, false };

            TaskUtil::yieldUntil([&writeState, requestData]() {
                writeState.successWrite = InternalFileSystem::writeBytes(
                    requestData->opaqueHandle, writeState.buffer, writeState.bufferSize);
            });

            if (!writeState.successWrite)
            {
                {
                    requestData->fileStatus = FileStatus::ReadingFail;
                    FileWriteResponse response;
                    response.status = FileStatus::WriteFail;
                    requestData->writeCallback(response);
                }
                return;
            }

            {
                requestData->fileStatus = FileStatus::ReadingSuccess;
                FileWriteResponse response;
                response.status = FileStatus::WriteSuccess;
                requestData->writeCallback(response);
            }

        }), requestData);
        task = requestData->task;
    }

    m_ts.execute(task);
    return asyncHandle;
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
    Request* requestData = nullptr;
    {
        std::unique_lock lock(m_requestsMutex);
        if (!m_requests.contains(handle))
            return;

        requestData = m_requests[handle];
        if (requestData == nullptr)
            return;
    }

    m_ts.wait(requestData->task);
    m_ts.cleanTaskTree(requestData->task);

    if (!InternalFileSystem::valid(requestData->opaqueHandle))
        return;

    InternalFileSystem::close(requestData->opaqueHandle);
    delete requestData;
    
    {
        std::unique_lock lock(m_requestsMutex);
        m_requests.free(handle);
    }
}

bool FileSystem::carveDirectoryPath(const char* directoryName)
{
    std::string dir = directoryName;
    InternalFileSystem::fixStringPath(dir);
    return InternalFileSystem::carvePath(dir, false);
}

bool FileSystem::enumerateFiles(std::vector<std::string>& dirList)
{
    return false;
}

bool FileSystem::deleteDirectory(const char* directoryName)
{
    return InternalFileSystem::deleteDirectory(directoryName);
}

bool FileSystem::deleteFile(const char* fileName)
{
    return InternalFileSystem::deleteFile(fileName);
}

IFileSystem* IFileSystem::create(const FileSystemDesc& desc)
{
    return new FileSystem(desc);
}

}
