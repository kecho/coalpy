#include <iostream>
#include <coalpy.core/Assert.h>

//hack bring definition of assert into this unit for release builds
#ifndef _DEBUG
#include "../core/Assert.cpp"
#endif

int main(int argc, char* argv[])
{
    return 0;
}
