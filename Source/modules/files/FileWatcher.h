#pragma once

#include <coalpy.files/IFileWatcher.h>
#include <string>
#include <set>
#include <thread>
#include <functional>

namespace coalpy
{

struct FileWatchState;

class FileWatcher : public IFileWatcher
{
public:
    FileWatcher(const FileWatchDesc& desc);
    ~FileWatcher();
    virtual void start() override;
    virtual void stop() override;
    virtual void addDirectory(const char* directory) override;
    virtual void addListener(IFileWatchListener* listener) override;
    virtual void removeListener(IFileWatchListener* listener) override;

private:
    void onFileListening();

    FileWatchDesc m_desc;
    FileWatchState* m_state = nullptr;
};

}
