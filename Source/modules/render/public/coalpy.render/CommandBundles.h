#pragma once
#include <coalpy.core/GenericHandle.h>
#include <string>

namespace coalpy
{
namespace render
{

struct CommandListBundle : GenericHandle<unsigned int> { };

enum class CompileListErrorType
{
    Ok
};

enum class ScheduleErrorType
{
    Ok
};

enum class WaitErrorType
{
    Ok
};

enum class DownloadResult
{
    Ok,
    NotReady,
    Invalid
};

enum ScheduleFlags : int
{
    ScheduleFlags_ReleaseOnSchedule = 1 << 0,
};

struct ScheduleStatus
{
    bool success() const { return type == ScheduleErrorType::Ok; }
    ScheduleErrorType type;
    std::string message;
};

struct CommandBundleCompileResult
{
    bool success() const { return type == CompileListErrorType::Ok; }
    CommandListBundle bundle;
    CompileListErrorType type;
    std::string message;
};

struct WaitBundleStatus
{
    bool success() const { return type == WaitErrorType::Ok; }
    WaitErrorType type;
    std::string message;
};

struct DownloadStatus
{
    bool success() const { return result == DownloadResult::Ok; }

    DownloadResult result;
    void* downloadPtr = nullptr;
    int downloadByteSize = 0;
};



}
}
