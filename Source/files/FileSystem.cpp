#include "FileSystem.h"

namespace coalpy
{

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

}
