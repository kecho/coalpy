#include "InternalFileSystem.h"
#include <coalpy.core/Assert.h>
#include <sstream>

#ifdef _WIN32 
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <coalpy.core/ClTokenizer.h>
#endif

namespace coalpy
{
#ifdef _WIN32
namespace InternalFileSystem
{
    struct WindowsFile
    {
        HANDLE h;
        unsigned int fileSize;
        OVERLAPPED overlapped;
        char buffer[bufferSize];
    };

    bool valid(OpaqueFileHandle h)
    {
        return h != nullptr;
    }

    OpaqueFileHandle openFile(const char* filename, RequestType request)
    {
        HANDLE h = CreateFileA(
            filename, //file name
            request == RequestType::Read ? GENERIC_READ : GENERIC_WRITE, //dwDesiredAccess
            0u, //dwShareMode
            NULL, //lpSecurityAttributes
            request == RequestType::Read ? OPEN_EXISTING : CREATE_ALWAYS,//dwCreationDisposition
            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, //dwFlagsAndAttributes
            NULL); //template attribute

        if (h == INVALID_HANDLE_VALUE)
            return nullptr;

        auto* wf = new WindowsFile;
        wf->h = h;
        wf->fileSize = GetFileSize(wf->h, NULL);
        wf->overlapped = {};
        wf->overlapped.hEvent = CreateEvent(
            NULL, //default security attribute
            TRUE, //manual reset event
            FALSE, //initial state = signaled
            NULL);//unnamed

        return (OpaqueFileHandle)wf;
    }

    bool readBytes(OpaqueFileHandle h, char*& outputBuffer, int& bytesRead, bool& isEof)
    {
        CPY_ASSERT(h != nullptr);
        isEof = false;
        if (h == nullptr)
            return false;

        auto* wf = (WindowsFile*)h;
        CPY_ASSERT(wf->h != INVALID_HANDLE_VALUE);

        DWORD dwordBytesRead;
        bool result = ReadFile(
            wf->h,
            wf->buffer,
            bufferSize,
            &dwordBytesRead,
            &wf->overlapped);

        if (!result)
        {
            auto dwError = GetLastError();
            switch (dwError)
            {
            case ERROR_HANDLE_EOF:
                {
                    isEof = true;
                    result = true;
                    break;
                }
            case ERROR_IO_PENDING:
                {
                    bool overlappedSuccess = GetOverlappedResult(wf->h,
                                                  &wf->overlapped,
                                                  &dwordBytesRead,
                                                  TRUE); 
                    if (!overlappedSuccess)
                    {
                        switch (dwError = GetLastError())
                        {
                        case ERROR_HANDLE_EOF:  
                            isEof = true;
                            result = true;
                            break;
                        case ERROR_IO_INCOMPLETE:
                            result = false;
                            break;
                        default:
                            result = false;
                            break;
                        }
                    }
                    else
                    {
                        result = true;
                        ResetEvent(wf->overlapped.hEvent);
                    }

                    break;
                }
            default:
                //file IO issue occured here
                result = false;
            }
        }

        if (result)
            wf->overlapped.Offset += dwordBytesRead;
        if (wf->overlapped.Offset >= wf->fileSize)
            isEof = true;

        bytesRead = (int)dwordBytesRead;
        outputBuffer = wf->buffer;
        return result;
    }

    bool writeBytes(OpaqueFileHandle h, const char* buffer, int bufferSize)
    {
        CPY_ASSERT(h != nullptr);
        if (h == nullptr)
            return false;

        auto* wf = (WindowsFile*)h;
        CPY_ASSERT(wf->h != INVALID_HANDLE_VALUE);

        DWORD dwordBytesWritten;
        bool result = WriteFile(
            wf->h,
            buffer,
            (DWORD)bufferSize,
            &dwordBytesWritten,
            &wf->overlapped);

        if (!result)
        {
            auto dwError = GetLastError();
            switch (dwError)
            {
            case ERROR_IO_PENDING:
                {
                    bool overlappedSuccess = GetOverlappedResult(wf->h,
                                                  &wf->overlapped,
                                                  &dwordBytesWritten,
                                                  TRUE); 
                    if (!overlappedSuccess)
                    {
                        result = false;
                    }
                    else
                    {
                        result = true;
                        ResetEvent(wf->overlapped.hEvent);
                    }

                    break;
                }
            default:
                //file IO issue occured here
                result = false;
            }
        }

        if (bufferSize != dwordBytesWritten)
            return false;

        return result;
    }

    void close(OpaqueFileHandle h)
    {
        CPY_ASSERT(h != nullptr);
        auto* wf = (WindowsFile*)h;
        CPY_ASSERT(wf->h != INVALID_HANDLE_VALUE);
        CloseHandle(wf->h);
        CloseHandle(wf->overlapped.hEvent);
        delete wf;
    }

    void fixStringPath(std::string& str)
    {
        for (auto& c : str)
        {
            if (c == '/')
                c = '\\';
        }
    }

    void getPathInfo(const std::string& filePath, PathInfo& pathInfo)
    {
        pathInfo = {};
        auto dirCandidates = ClTokenizer::splitString(filePath, '\\');
        for (auto d : dirCandidates)
            if (d != "")
                pathInfo.directoryList.push_back(d);

        if (pathInfo.directoryList.empty())
            return;

        pathInfo.filename = pathInfo.directoryList.back();
        pathInfo.directoryList.pop_back();
    
        std::stringstream ss;
        for (auto& d : pathInfo.directoryList)
        {
            ss << d  << "\\";
        }
        pathInfo.path = std::move(ss.str());
    }

    bool createDirectory(const char* str)
    {
        return CreateDirectoryA(str, nullptr);
    }

    bool deleteDirectory(const char* str)
    {
        return RemoveDirectoryA(str);
    }

    bool deleteFile(const char* str)
    {
        return DeleteFile(str);
    }

    void getAttributes(const std::string& dirName_in, bool& exists, bool& isDir)
    {
        exists = false;
        isDir = false;
        DWORD ftyp = GetFileAttributesA(dirName_in.c_str());
        if (ftyp == INVALID_FILE_ATTRIBUTES)
        {
            exists = false;
            isDir = false;
            return;
        }
        
        exists = true;
        if (ftyp & FILE_ATTRIBUTE_DIRECTORY)
        {
            isDir = true;
            return;
        }
        
        return;
    }

    bool carvePath(const std::string& path, bool lastIsFile)
    {
        bool exists, isDir;
        getAttributes(path, exists, isDir);
        if (exists)
            return lastIsFile ? !isDir : isDir;

        //ok so the path doesnt really exists, lets carve it.
        PathInfo pathInfo;
        getPathInfo(path, pathInfo);
        if (pathInfo.filename == "")
            return false;

        if (!lastIsFile)
            pathInfo.directoryList.push_back(pathInfo.filename);

        std::stringstream ss;
        for (auto& d : pathInfo.directoryList)
        {
            ss << d << "\\";
            auto currentPath = ss.str();
            getAttributes(currentPath, exists, isDir);
            if (exists && !isDir)
                return false;

            if (!exists && !createDirectory(currentPath.c_str()))
                return false;
        }

        return true;
    }


}
#else
    #error "Platform not supported"
#endif
}
