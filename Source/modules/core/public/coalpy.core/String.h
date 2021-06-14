#pragma once

#include <string>
#include <locale>
#include <codecvt>

namespace coalpy
{

inline std::string ws2s(const std::wstring& wstr)
{
    using convert_typeX = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<convert_typeX, wchar_t> converterX;
    return converterX.to_bytes(wstr);
}

inline std::wstring s2ws(const std::string& s)
{
    using convert_typeX = std::codecvt_utf8_utf16<wchar_t>;
    std::wstring_convert<convert_typeX> converterX;
    return converterX.from_bytes(s);
}

inline unsigned int stringHash(const char* str)
{
    //FNV hash
    const unsigned int fnvPrime = 0x811C9DC5;
	unsigned int hash = 0;
	unsigned int i = 0;
    const char* curr = str;
    while (*curr)
	{
		hash *= fnvPrime;
		hash ^= *curr++;
	}

	return hash;
}

inline unsigned int stringHash(const std::string& str)
{
    return stringHash(str.c_str());
}


}
