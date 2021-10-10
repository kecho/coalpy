#pragma once

namespace coalpy
{

enum class Keys
{
    #define KEY_DECL(x) x,
    #include <coalpy.window/Keys.inl>
    #undef KEY_DECL
    Count
};

inline const char* keyName(Keys key)
{
    switch (key)
    {
    #define KEY_DECL(x) case Keys::x: return #x;
    #include <coalpy.window/Keys.inl>
    #undef KEY_DECL

    default:
        return "Uknown";
    }
}


}
