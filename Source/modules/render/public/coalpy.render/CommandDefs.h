#pragma once
#include <coalpy.core/GenericHandle.h>
#include <string>

namespace coalpy
{
namespace render
{

struct WorkHandle : GenericHandle<unsigned int> { };

enum class ScheduleErrorType
{
    Ok,
    NullListFound,
    OutOfBounds,
    ListNotFinalized,
    BadTableInfo,
    ReadCpuFlagNotFound,
    CommitResourceStateFail,
    InvalidResource,
    ResourceStateNotFound,
    MultipleDownloadsOnSameResource,
    CorruptedCommandListSentinel,
};

enum class WaitErrorType
{
    Ok,
    NotReady,
    Invalid,
};

enum class DownloadResult
{
    Ok,
    NotReady,
    Invalid
};

enum ScheduleFlags : int
{
    ScheduleFlags_None = 0,
    ScheduleFlags_GetWorkHandle = 1 << 0,
};

struct ScheduleStatus
{
    bool success() const { return type == ScheduleErrorType::Ok; }
    WorkHandle workHandle;
    ScheduleErrorType type = ScheduleErrorType::Ok;
    std::string message;
};

struct WaitStatus
{
    bool success() const { return type == WaitErrorType::Ok; }
    WaitErrorType type = WaitErrorType::Ok;
    std::string message;
};

struct DownloadStatus
{
    bool success() const { return result == DownloadResult::Ok; }

    DownloadResult result = DownloadResult::Ok;
    void* downloadPtr = nullptr;
    size_t downloadByteSize = 0;
    size_t rowPitch = 0;
    int width  = 0;
    int height = 0;
    int depth  = 0;
};

struct ResourceMemoryInfo
{
    bool isBuffer = false;
    size_t byteSize = 0u;
    int width = 0u;
    int height = 0u;
    int depth  = 0u;
    size_t rowPitch = 0u;
};



}
}
