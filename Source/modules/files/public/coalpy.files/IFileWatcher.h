#pragma once

#include <string>
#include <set>

namespace coalpy
{

class IFileWatchListener
{
public:
    virtual ~IFileWatchListener() {}
    virtual void onFilesChanged(const std::set<std::string>& filesChanged) = 0;
};

struct FileWatchDesc
{
    int pollingRateMS = 1000;
};

class IFileWatcher
{
public:
    static IFileWatcher* create(const FileWatchDesc& desc);
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void addDirectory(const char* directory) = 0;
    virtual void addListener(IFileWatchListener* listener) = 0;
    virtual void removeListener(IFileWatchListener* listener) = 0;
};


}
