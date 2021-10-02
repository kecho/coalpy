# coalpy
Compute abstraction layer for python.

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Build Status](https://app.travis-ci.com/kecho/coalpy.svg?branch=master)](https://app.travis-ci.com/github/kecho/coalpy/branches)

Coalpy is a free low friction python 3.9 native module for Windows. Coalpy's main goal is to make modern DX12 GPU compute software easy to write and deploy.

![seascape_example](docs/images/seascape.png?raw=true "Seascape shadertoy by Alexander Alekseev aka TDM - 2014.")

## Python module documentation

Get started with API documentation here:
[coalpy.gpu documentation](https://kecho.github.io/coalpy/coalpy.gpu.html)

## About Contributing

Use this guide if you plan to build coalpy python module from source.

### Building requirements
* Install Python 3.9x
* Ensure you have a version of visual studio with support for VC 
* Ensure you are on the latest and greatest version of windows 10

### Directories
* Source - everything inside here gets compiled by the build system.
    * modules - Internal c++ implementation of coalpy.
    * pymodules/gpu - The Python glue code, which translates the c++ coalpy module into public functions and types ready to be used in python.
    * scripts - Python scripts that get packaged with the coalpy module.
    * [libpng|libjpeg|zlib|imgui] - External libraries, which are compiled in source.
    * tests - Internal graphics and system test suites for coalpy, does not test the Python layer, only the internal layer.
* Build - Build scripts for the tundra build system.
* External - External precompiled libraries used.
* Tools - Executables and tools used to assist in building. In this case the tundra build system is contained here.
* docs - Documentation folder. This documentation utilizes github pages, which can be seen [here](https://kecho.github.io/coalpy/coalpy.gpu.html)

## Compiling in Windows

To compile in Windows, use the build.bat script file:

```
build.bat debug
```
for release mode
```
build.bat release
```

To generate a solution for visual studio, use the gensln.bat script: 

```
gensln.bat
```

To run the internal c++ tests, run the coalpy_tests.exe program generated:

```
t2-output\win64-msvc-debug-default\coalpy_tests.exe
```

For information about the test suites commands, such as filters / repeating tests etc use the -h flag.

## Creating Python package

cd into the t2-output\win64-msvc-[debug|release]-default\coalpy_pip directory. Run the command:

```
python -m build
```

Ensure you have the latest version of the build module. To configure metadata of the pip package modify things inside the Source/pipfiles folder.

## Updating documentation

All documentation of the coalpy Python API is self contained inside the Source/pymodules/gpu folder. Inside we fill doc strings inside types and functions exported to Python.
To generate the documentation, cd into the t2-output/win64-msvc-[release|debug|production]-default folder, and run the commands:

```
python -m pydoc -w coalpy
python -m pydoc -w coalpy.gpu
```

This will generate coalpy.html and coalpy.gpu.html which can be copied into the docs directory.

## Credits

Coalpy utilizes a few great open source libraries. Links with the authors and source provided here:

* [libjpeg - JPEG library by @LuaDist](https://github.com/LuaDist/libjpeg/)
* [libpng - PNG library by @glennrp](https://github.com/glennrp/libpng)
* [zlib - compression library by @madler](https://github.com/madler/zlib)
* [imgui - User interface library by @ocornut](https://github.com/ocornut/imgui)
* [DirectXShaderCompiler - hlsl compiler by @microsoft](https://github.com/microsoft/DirectXShaderCompiler)


Special thanks to my amazing wife Katie: I love you.
