#pragma once
#include <coalpy.files/FileDefs.h>

namespace coalpy
{

class ITaskSystem;
class ByteBuffer;

class IFileSystem
{

static IFileSystem* create(const FileSystemDesc& desc);
virtual AsyncFileHandle open (const FileOpenRequest& request) = 0;
virtual AsyncFileHandle write(const FileWriteRequest& request) = 0;
virtual void wait(AsyncFileHandle handle) = 0;
virtual bool openStatus (AsyncFileHandle handle, FileOpenResponse& response) = 0;
virtual bool writeStatus(AsyncFileHandle handle, FileWriteResponse& response) = 0;
virtual void closeHandle(AsyncFileHandle handle) = 0;

};

}
