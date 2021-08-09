#pragma once
#include <vector>
#include <string>

namespace coalpy
{
    namespace FileUtils
    {
        void fixStringPath(const std::string& str, std::string& output);
        void getFileName(const std::string& path, std::string& outName);
        void getFileExt(const std::string& path, std::string& outExt);
        void getDirName(const std::string& path, std::string& outDir);
        void getAbsolutePath(const std::string& path, std::string& outDir);
    }
}
