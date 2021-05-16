#include "FileSystem.h"
#include <coalpy.tasks/ITaskSystem.h>

namespace coalpy
{

FileSystem::FileSystem(const FileSystemDesc& desc)
: m_desc(desc)
, m_ts(*desc.taskSystem)
{
    
}

AsyncFileHandle FileSystem::open(const FileOpenRequest& request)
{
    return AsyncFileHandle();
}

AsyncFileHandle FileSystem::write(const FileWriteRequest& request)
{
    return AsyncFileHandle();
}


void FileSystem::wait(AsyncFileHandle handle)
{
}

bool FileSystem::openStatus (AsyncFileHandle handle, FileOpenResponse& response)
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
