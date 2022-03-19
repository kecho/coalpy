#include <coalpy.files/Utils.h>
#include <coalpy.core/String.h>

namespace coalpy
{

FileLookup::FileLookup()
: filename(""), hash(0u)
{
}

FileLookup::FileLookup(const char* file)
: filename(file)
{
    hash = stringHash(filename);
}

FileLookup::FileLookup(const std::string& filename)
: filename(filename)
{
    hash = stringHash(filename);
}

}
