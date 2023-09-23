#include "InternalFileSystem.h"
#include <coalpy.core/Assert.h>
#include <sstream>
#include <cstring>
#include <coalpy.core/ClTokenizer.h>

#ifdef _WIN32 
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#elif defined(__linux__)
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <linux/limits.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#elif defined(__APPLE__)
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#endif

namespace coalpy
{
#ifdef _WIN32
#define SEP '\\'
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
        bool retry = true;
        UINT attempt = 0;
        HANDLE h = INVALID_HANDLE_VALUE;
        while (retry)
        {
            h = CreateFileA(
                filename, //file name
                request == RequestType::Read ? GENERIC_READ : GENERIC_WRITE, //dwDesiredAccess
                request == RequestType::Read ? (FILE_SHARE_READ | FILE_SHARE_WRITE) : 0u, //dwShareMode
                NULL, //lpSecurityAttributes
                request == RequestType::Read ? OPEN_EXISTING : CREATE_ALWAYS,//dwCreationDisposition
                FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, //dwFlagsAndAttributes
                NULL); //template attribute

            ++attempt;
            if (h == INVALID_HANDLE_VALUE)
            {
                if (GetLastError() == ERROR_SHARING_VIOLATION && attempt < 10)
                {
                    Sleep(2.0); //sleep for 2ms, if this is the 10th attempt, just fail.
                    continue;
                }
                return nullptr;
            }
            retry = false;
        }

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

    void close(OpaqueFileHandle& h)
    {
        CPY_ASSERT(h != nullptr);
        auto* wf = (WindowsFile*)h;
        CPY_ASSERT(wf->h != INVALID_HANDLE_VALUE);
        CloseHandle(wf->h);
        CloseHandle(wf->overlapped.hEvent);
        h = {};
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
        auto dirCandidates = ClTokenizer::splitString(filePath, SEP);
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
            ss << d  << SEP;
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

    void getFileName(const std::string& path, std::string& outName)
    {
        int index = path.size() - 1;
        for (; index >= 0 && path[index] != '\\'; --index);
        index++;
        outName = path.c_str() + index;
    }

    void getAttributes(const std::string& dirName_in, bool& exists, bool& isDir, bool& isDots)
    {
        exists = false;
        isDir = false;
        isDots = false;
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
            std::string dirNameSanitized = dirName_in;
            std::string filename;
            fixStringPath(dirNameSanitized);
            getFileName(dirNameSanitized, filename);
            isDots = filename == "." || filename == "..";
            return;
        }
    
        return;
    }

    bool carvePath(const std::string& path, bool lastIsFile)
    {
        bool exists, isDir, isDots;
        getAttributes(path, exists, isDir, isDots);
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
            getAttributes(currentPath, exists, isDir, isDots);
            if (exists && !isDir)
                return false;

            if (!exists && !createDirectory(currentPath.c_str()))
                return false;
        }

        return true;
    }

    void enumerateFiles(const std::string& path, std::vector<std::string>& files)
    {
        WIN32_FIND_DATA data = {};
        std::string query = path + "\\*";
        HANDLE hFind = FindFirstFile(query.c_str(), &data);      // DIRECTORY
    
        if ( hFind == INVALID_HANDLE_VALUE )
            return;
    
        do {
            std::string fileName = data.cFileName;
            std::string fullPath = path + "\\" + fileName;
            files.push_back(fullPath);
        } while (FindNextFile(hFind, &data));
        FindClose(hFind);
    }

    void getAbsolutePath(const std::string& path, std::string& outDir)
    {
        const int DestBufferSize = 1024;
        char destination[DestBufferSize];
        auto charsWritten = GetFullPathNameA(path.c_str(), 1024, destination, NULL);
        CPY_ASSERT_MSG(charsWritten != DestBufferSize, "getAbsolutePath failed, not enough characters (path too long)");
        if (charsWritten == 0)
        {
            outDir = path;
        }
        else
        {
            outDir = destination;
        }
    }

}
#elif defined(__linux__) || defined(__APPLE__)
#define SEP '/'
//TODO stubbing linux methods. Fill in with posix??
namespace InternalFileSystem
{
    struct PosixFile 
    {
        int h;
        unsigned int fileSize;
        ssize_t offset;
        char buffer[bufferSize];
    };

    bool valid(OpaqueFileHandle h)
    {
        return (PosixFile*)h != nullptr;
    }

    OpaqueFileHandle openFile(const char* filename, RequestType request)
    {
        int fd = ::open(filename, request == InternalFileSystem::Read ? O_RDONLY : (O_CREAT | O_TRUNC | O_WRONLY ), S_IRUSR | S_IWUSR);

        if (fd == -1)
            return nullptr;

        struct stat statbuf;
        int err = fstat(fd, &statbuf);
        if (err < 0)
        {
            ::close(fd);
            return nullptr;
        }
        auto* pf = new PosixFile { fd, (unsigned)statbuf.st_size, 0u };
        return (OpaqueFileHandle)pf;
    }

    bool readBytes(OpaqueFileHandle h, char*& outputBuffer, int& bytesRead, bool& isEof)
    {
        auto* pf = (PosixFile*)h;
        if (pf == nullptr || pf->h == -1)
            return false;

        ssize_t preadBytes = pread(pf->h, pf->buffer, std::min((ssize_t)bufferSize, pf->fileSize - pf->offset), pf->offset);
        if (preadBytes == -1)
            return false;

        pf->offset += preadBytes;
        bytesRead = preadBytes;
        outputBuffer = pf->buffer;
        isEof = pf->offset >= pf->fileSize;
        return true;
    }

    bool writeBytes(OpaqueFileHandle h, const char* buffer, int bufferSize)
    {
        auto* pf = (PosixFile*)h;
        if (pf == nullptr || pf->h == -1)
            return false;
        
        ssize_t pwriteBytes = pwrite(pf->h, buffer, (size_t)bufferSize, 0u);
        return pwriteBytes != -1;
    }

    void close(OpaqueFileHandle& h)
    {
        auto* pf = (PosixFile*)h;
        if (pf == nullptr)
            return;

        ::close(pf->h);
        delete pf;
        h = {};
    }

    void fixStringPath(std::string& str)
    {
        for (auto& c : str)
        {
            if (c == '\\')
                c = '/';
        }
    }

    void getPathInfo(const std::string& filePath, PathInfo& pathInfo)
    {
        pathInfo = {};
        auto dirCandidates = ClTokenizer::splitString(filePath, '/');
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
            ss << d  << "/";
        }
        pathInfo.path = std::move(ss.str());
    }

    void getFileName(const std::string& path, std::string& outName)
    {
        int index = path.size() - 1;
        for (; index >= 0 && path[index] != '/'; --index);
        index++;
        outName = path.c_str() + index;
    }

    bool createDirectory(const char* str)
    {
        int status = mkdir(str, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        return status == 0; 
    }

    bool deleteDirectory(const char* path)
    {
        return rmdir(path) == 0;
    }

    bool deleteFile(const char* str)
    {
        return unlink(str) == 0;
    }

    void getAttributes(const std::string& dirName_in, bool& exists, bool& isDir, bool& isDots)
    {
        struct stat statbuf;
        int err = stat(dirName_in.c_str(), &statbuf);
        exists = false;
        isDir = false;
        isDots = false;
        if (err < 0)
            return;

        exists = true;
        if (S_ISDIR(statbuf.st_mode) != 0)
        {
            isDir = true;
            std::string dirNameSanitized = dirName_in;
            std::string filename;
            fixStringPath(dirNameSanitized);
            getFileName(dirNameSanitized, filename);
            isDots = filename == "." || filename == "..";
        }
    }

    bool carvePath(const std::string& path, bool lastIsFile)
    {
        bool exists, isDir, isDots;
        getAttributes(path, exists, isDir, isDots);
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
            ss << d << "/";
            auto currentPath = ss.str();
            getAttributes(currentPath, exists, isDir, isDots);
            if (exists && !isDir)
                return false;

            if (!exists && !createDirectory(currentPath.c_str()))
                return false;
        }

        return true;
    }

    void enumerateFiles(const std::string& path, std::vector<std::string>& files)
    {
        struct stat stat_path;
        // stat for the path
        int s = stat(path.c_str(), &stat_path);
        if (s != 0)
            return;

        // if path does not exists or is not dir - exit with status -1
        if (S_ISDIR(stat_path.st_mode) == 0)
            return;

        // if not possible to read the directory for this user
        DIR *dir;
        if ((dir = opendir(path.c_str())) == NULL)
        {
            return;
        }

        // the length of the path
        size_t path_len = path.size();
        char *full_path;
        struct dirent *entry;
        struct stat stat_entry;

        // iteration through entries in the directory
        while ((entry = readdir(dir)) != nullptr)
        {
            // skip entries "." and ".."
            if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
                continue;

            // determinate a full path of an entry
            full_path = (char*)calloc(path_len + strlen(entry->d_name) + 1, sizeof(char));
            strcpy(full_path, path.c_str());
            strcat(full_path, "/");
            strcat(full_path, entry->d_name);
            files.push_back(full_path);
            free(full_path);
        }

        closedir(dir);
    }

    void getAbsolutePath(const std::string& path, std::string& outDir)
    {
        char resolvedPath[PATH_MAX] = {};
        realpath(path.c_str(), resolvedPath);
        outDir = resolvedPath;
    }

}
#else
    #error "Platform not supported"
#endif

namespace FileUtils
{
    void fixStringPath(const std::string& str, std::string& output)
    {
        auto cpy = str;
        InternalFileSystem::fixStringPath(cpy);        
        output = std::move(cpy);
    }

    void getFileName(const std::string& path, std::string& outName)
    {
        InternalFileSystem::getFileName(path, outName);
    }

    void getFileExt(const std::string& path, std::string& outExt)
    {
        std::string outName;
        InternalFileSystem::getFileName(path, outName);

        int extIndex = -1;
        for (int i = 0; i < (int)outName.size(); ++i)
        {
            int index = outName.size() - 1 - i;
            if (outName[index] == '.')
            {
                extIndex = index;
                break;
            }
        }

        if (extIndex == -1)
        {
            outExt = "";
        }
        else
        {
            outExt = outName.c_str() + extIndex + 1;
        }
    }

    void getDirName(const std::string& path, std::string& outName)
    {
        std::string cpy = path;
        InternalFileSystem::fixStringPath(cpy);
        int foundIndex = 0;
        for (int i = 0; i < (int)cpy.size(); ++i)
        {
            int currIndex = (int)cpy.size() - 1 - i; 
            char c = cpy[currIndex];
            if (c == SEP)
            {
                foundIndex = currIndex;
                break;
            }
        }

        outName = cpy.substr(0, foundIndex);
    }

    void getAbsolutePath(const std::string& path, std::string& outDir)
    {
        InternalFileSystem::getAbsolutePath(path, outDir);
    }
}

}
