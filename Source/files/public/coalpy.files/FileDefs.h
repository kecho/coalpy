#pragma once
#include <coalpy.core/GenericHandle.h>
#include <coalpy.core/ByteBuffer.h>
#include <functional>

namespace coalpy
{

class ITaskSystem;

struct FileSystemDesc
{
    ITaskSystem* taskSystem = nullptr;
};

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

}
