#pragma once
#include <coalpy.core/GenericHandle.h>
#include <coalpy.core/ByteBuffer.h>
#include <vector>
#include <string>
#include <functional>

namespace coalpy
{

class ITaskSystem;

struct FileSystemDesc
{
    ITaskSystem* taskSystem = nullptr;
};

enum class IoError
{
    FailedCreating,
    FailedOpening,
    FailedWriting,
    FailedReading,
    FailedCreatingDir,
    None
};

inline const char* IoError2String(IoError err)
{
    switch (err)
    {
    case IoError::FailedCreating:
        return "IoError::FailedCreating";
    case IoError::FailedOpening:
        return "IoError::FailedOpening";
    case IoError::FailedWriting:
        return "IoError::FailedWriting";
    case IoError::FailedReading:
        return "IoError::FailedReading";
    case IoError::FailedCreatingDir:
        return "IoError::FailedCreatingDir";
    default:
    case IoError::None:
        return "IoError::None";
    }
}

enum class FileStatus
{
    Idle,
    Opening,
    Reading,
    Writing,
    Fail,
    Success
};

struct FileReadResponse
{
    IoError error = IoError::None;
    FileStatus status = FileStatus::Idle;
    std::string filePath;
    const char* buffer = nullptr;
    int size = 0;
};

struct FileWriteResponse
{
    IoError error = IoError::None;
    FileStatus status = FileStatus::Idle;
};

using AsyncFileHandle = GenericHandle<unsigned int>;
using FileReadDoneCallback = std::function<void(FileReadResponse& response)>;
using FileWriteDoneCallback = std::function<void(FileWriteResponse& response)>;

enum class FileRequestFlags : int
{
    AutoStart = 1 << 0
};

struct FileReadRequest
{
    std::string path;
    std::vector<std::string> additionalRoots;
    FileReadDoneCallback doneCallback;
    int flags;

    FileReadRequest() {}

    FileReadRequest(std::string path, FileReadDoneCallback doneCallback)
    : flags(0), path(path), doneCallback(doneCallback) {}

    FileReadRequest(std::string path, FileReadDoneCallback doneCallback, int flags)
    : flags(flags), path(path), doneCallback(doneCallback) {}
};

struct FileWriteRequest
{
    std::string path;
    FileWriteDoneCallback doneCallback;
    const char* buffer;
    int size;
    int flags;
    
    FileWriteRequest() {}

    FileWriteRequest(std::string path, FileWriteDoneCallback doneCallback, const char* buffer, int size)
    : flags(0), path(path), doneCallback(doneCallback), buffer(buffer), size(size) {}

    FileWriteRequest(std::string path, FileWriteDoneCallback doneCallback, const char* buffer, int size, int flags)
    : flags(flags), path(path), doneCallback(doneCallback), buffer(buffer), size(size) {}
};

struct FileAttributes
{
    bool exists;
    bool isDir;
    bool isDot;
};

}
