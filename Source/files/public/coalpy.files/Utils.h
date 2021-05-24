#pragma once
#include <vector>
#include <string>

namespace coalpy
{
    namespace FileUtils
    {
        void fixStringPath(const std::string& str, std::string& output);
        void getFileName(const std::string& path, std::string& outName);
    }
}
