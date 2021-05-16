#pragma once
#include <coalpy.files/FileDefs.h>
#include <string>
#include <vector>

namespace coalpy
{

class ITaskSystem;
class ByteBuffer;

class IFileSystem
{
public:
    static IFileSystem* create(const FileSystemDesc& desc);
    virtual AsyncFileHandle read (const FileReadRequest& request) = 0;
    virtual AsyncFileHandle write(const FileWriteRequest& request) = 0;
    virtual void wait(AsyncFileHandle handle) = 0;
    virtual bool readStatus (AsyncFileHandle handle, FileReadResponse& response) = 0;
    virtual bool writeStatus(AsyncFileHandle handle, FileWriteResponse& response) = 0;
    virtual void closeHandle(AsyncFileHandle handle) = 0;
    virtual bool carveDirectoryPath(const char* directoryName) = 0;
    virtual bool enumerateFiles(std::vector<std::string>& dirList) = 0;
    virtual bool deleteDirectory(const char* directoryName) = 0;
    virtual bool deleteFile(const char* fileName) = 0;
};

}
