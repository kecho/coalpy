#pragma once

namespace coalpy
{

enum class Keys
{
    //keys
    A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,

    //numbers
    k0, k1, k2, k3, k4, k5, k6, k7, k8, k9,

    //f keys
    F1, F2, F3, F4, F5, F6, F7, F8, F9,

    //special
    Space, Slash, SemiColon, Equal, LeftBrac, RightBrac, Enter, Space, 
    LShift, RShift, LAlt, RAlt,

    //arrows
    Up, Down, Right, Left, 

    //mouse
    MouseLeft, MouseRight,

    Counts
};

static inline const char* keyToStr(Keys k)
{
    switch (k)
    {

    }
}

}
