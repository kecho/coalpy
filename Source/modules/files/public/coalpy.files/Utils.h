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

    struct FileLookup
    {
        std::string filename;
        unsigned int hash;
    
        FileLookup();
        FileLookup(const char* filename);
        FileLookup(const std::string& filename);
    
        bool operator==(const FileLookup& other) const
        {
            return hash == other.hash;
        }
    
        bool operator>(const FileLookup& other) const
        {
            return hash > other.hash;
        }
    
        bool operator>=(const FileLookup& other) const
        {
            return hash >= other.hash;
        }
    
        bool operator<(const FileLookup& other) const
        {
            return hash < other.hash;
        }
    
        bool operator<=(const FileLookup& other) const
        {
            return hash < other.hash;
        }
    };

}

namespace std
{
    template<>
    struct hash<coalpy::FileLookup>
    {
        std::size_t operator()(const coalpy::FileLookup& key) const
        {
            return (std::size_t)key.hash;
        }
    };
}

