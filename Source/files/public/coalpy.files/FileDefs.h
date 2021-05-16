#pragma once
#include <coalpy.core/GenericHandle.h>
#include <coalpy.core/ByteBuffer.h>
#include <string>
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
    Opening,
    Reading,
    OpenFail,
    ReadingFail,
    ReadingSuccess,
    WriteSuccess,
    WriteFail,
    Writting
};

struct FileReadResponse
{
    FileStatus status = FileStatus::Opening;
    ByteBuffer response;
};

struct FileWriteResponse
{
    FileStatus status = FileStatus::Writting;
};

using AsyncFileHandle = GenericHandle<unsigned int>;
using FileReadDoneCallback = std::function<void(FileReadResponse& response)>;
using FileWriteDoneCallback = std::function<void(FileWriteResponse& response)>;

struct FileReadRequest
{
    std::string path;
    FileReadDoneCallback doneCallback;
};

struct FileWriteRequest
{
    std::string path;
    FileWriteDoneCallback doneCallback;
};

}
