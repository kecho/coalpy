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
    Idle,
    Opening,
    Reading,
    OpenFail,
    FailedCreatingDir,
    ReadingFail,
    ReadingSuccessEof,
    WriteSuccess,
    WriteFail,
    Writing
};

struct FileReadResponse
{
    FileStatus status = FileStatus::Idle;
    const char* buffer = nullptr;
    int size = 0;
};

struct FileWriteResponse
{
    FileStatus status = FileStatus::Idle;
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
    FileWriteDoneCallback doneCallback = nullptr;
    const char* buffer = nullptr;
    int size = 0;
};

struct FileAttributes
{
    bool exists;
    bool isDir;
    bool isDot;
};

}
