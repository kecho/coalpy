#include "FileSystem.h"
#include <coalpy.tasks/ITaskSystem.h>
#include <coalpy.core/Assert.h>

namespace coalpy
{

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
        requestData->type = InternalFileSystem::RequestType::Read;
        requestData->filename = request.path;
        requestData->readCallback = request.doneCallback;
        requestData->opaqueHandle = {};
        requestData->fileStatus = FileStatus::Idle;
        requestData->task = m_ts.createTask(TaskDesc([this](TaskContext& ctx)
        {
            auto* requestData = (Request*)ctx.data;
            {
                requestData->fileStatus = FileStatus::Opening;
                FileReadResponse response;
                response.status = FileStatus::Opening;
                requestData->readCallback(response);
            }
            requestData->opaqueHandle = InternalFileSystem::openFile(requestData->filename.c_str(), InternalFileSystem::RequestType::Read);
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

            requestData->fileStatus = FileStatus::Reading;

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

                {
                    FileReadResponse response;
                    response.status = FileStatus::Reading;
                    response.buffer = readState.output;
                    response.size = readState.bytesRead;
                    requestData->readCallback(response);
                }

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
                requestData->fileStatus = FileStatus::ReadingSuccessEof;
                FileReadResponse response;
                response.status = FileStatus::ReadingSuccessEof;
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
        requestData->type = InternalFileSystem::RequestType::Write;
        requestData->filename = request.path;
        requestData->writeCallback = request.doneCallback;
        requestData->opaqueHandle = {};
        requestData->fileStatus = FileStatus::Idle;
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
            requestData->opaqueHandle = InternalFileSystem::openFile(requestData->filename.c_str(), InternalFileSystem::RequestType::Write);
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

            requestData->fileStatus = FileStatus::Writing;

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
                    requestData->fileStatus = FileStatus::WriteFail;
                    FileWriteResponse response;
                    response.status = FileStatus::WriteFail;
                    requestData->writeCallback(response);
                }
                return;
            }

            {
                requestData->fileStatus = FileStatus::WriteSuccess;
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
