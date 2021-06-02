#pragma once

#include <string>
#include <set>
#include <thread>
#include <functional>

namespace coalpy
{

using OnFileChangedFn = std::function<void(const std::set<std::string>& filesChanged)>;
struct FileWatchState;

class FileWatcher
{
public:
    ~FileWatcher();
    void start(
        const char* directory,
        OnFileChangedFn onFileChanged,
        int pollingRateMs = 1000);
    void stop();

private:
    void onFileListening();
    FileWatchState* m_state;
    std::string m_rootDir;
    int m_pollingRateMs;
    OnFileChangedFn m_onFileChangedFn;
};

}
