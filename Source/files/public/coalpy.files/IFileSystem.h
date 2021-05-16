#pragma once
#include <coalpy.core/GenericHandle.h>
#include <coalpy.core/ByteBuffer.h>
#include <functional>

namespace coalpy
{

class ByteBuffer;

enum class FileStatus
{
    OpenSucces,
    OpenFail,
    Opening,
    WriteSuccess,
    WriteFail,
    Writting
};

struct FileOpenResponse
{
    FileStatus status = FileStatus::Opening;
    ByteBuffer response;
};

struct FileWriteResponse
{
    FileStatus status = FileStatus::Writting;
};

using AsyncFileHandle = GenericHandle<unsigned int>;
using FileOpenDoneCallback = std::function<void(FileOpenResponse& response)>;
using FileWriteDoneCallback = std::function<void(FileWriteResponse& response)>;

struct FileOpenRequest
{
    const char* path = nullptr;
    FileOpenDoneCallback doneCallback = nullptr;
};

struct FileWriteRequest
{
    const char* path = nullptr;
    FileWriteDoneCallback doneCallback = nullptr;
};

class IFileSystem
{

virtual AsyncFileHandle open (const FileOpenRequest& request) = 0;
virtual AsyncFileHandle write(const FileWriteRequest& request) = 0;
virtual void wait(AsyncFileHandle handle) = 0;
virtual bool openStatus (AsyncFileHandle handle, FileOpenResponse& response) = 0;
virtual bool writeStatus(AsyncFileHandle handle, FileWriteResponse& response) = 0;
virtual void closeHandle(AsyncFileHandle handle) = 0;

};

}
