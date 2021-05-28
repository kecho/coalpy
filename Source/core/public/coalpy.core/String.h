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

}
