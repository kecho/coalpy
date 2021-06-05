#pragma once

#include <coalpy.files/FileDefs.h>
#include <string>
#include <vector>

namespace coalpy
{

namespace InternalFileSystem
{
    enum
    {
        bufferSize = 16 * 1024 //16kb buffer size
    };

    enum RequestType
    {
        Read,
        Write
    };

    typedef void* OpaqueFileHandle;

    struct PathInfo
    {
        std::vector<std::string> directoryList;
        std::string path;
        std::string filename;
    };


    bool valid(OpaqueFileHandle h);

    OpaqueFileHandle openFile(const char* filename, RequestType request);

    bool readBytes(OpaqueFileHandle h, char*& outputBuffer, int& bytesRead, bool& isEof);

    bool writeBytes(OpaqueFileHandle h, const char* buffer, int bufferSize);

    void close(OpaqueFileHandle& h);

    void fixStringPath(std::string& str);

    void getPathInfo(const std::string& filePath, PathInfo& pathInfo);

    void getFileName(const std::string& path, std::string& outName);

    bool createDirectory(const char* str);

    bool deleteDirectory(const char* str);

    bool deleteFile(const char* str);

    void getAttributes(const std::string& dirName_in, bool& exists, bool& isDir, bool& isDots);

    bool carvePath(const std::string& path, bool lastIsFile = true);

    void enumerateFiles(const std::string& path, std::vector<std::string>& files);
}

}
