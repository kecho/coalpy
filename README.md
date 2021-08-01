# coalpy
compute abstraction layer for python

Coalpy is a free low friction python 3.9 native module for windows. Coalpy's main goal is to make DX12 GPU compute pipelines easy to write and deploy.

[![Build Status](https://travis-ci.com/kecho/coalpy.svg?branch=master)](https://travis-ci.com/kecho/coalpy)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

## Documentation

Get started with API documentation here:
[coalpy.gpu documentation](https://kecho.github.io/coalpy/Docs/coalpy.gpu.html)

## About Contributing

Use this guide if you plan to build coalpy python module from source.

Building requirements:
* Install Python 3.9x
* Set the environment variable PYTHON39_ROOT to the root installation of your python library.
* Ensure you have a version of visual studio with support for VC 
* Ensure you are on the latest and greatest version of windows 10


## Compiling in Windows

To build run the script

```
build.bat debug
```
for release mode
```
build.bat release
```

Generate a solution for visual studio using

```
gensln.bat
```

Run the tests by running:

```
t2-output\win64-msvc-debug-default\coalpy_tests.exe
```

## Creating python package

cd into the t2-output\win64-msvc-[debug|release]-default director. Inside there will be a coalpy_pip directory.
In here run the command 

```
python -m build
```

Ensure you have the latest version of the build module. To configure metadata of the pip package modify things inside the Source/pipfiles folder.

## Credits

coalpy utilizes a few external libraries. Thank you for the authors of these marvelous open source libraries, ensure to check them out / donate and contribute if you can.
and all the people that provided feedback / encouragement and support.

* [libjpeg - JPEG library by @LuaDist](https://github.com/LuaDist/libjpeg/)
* [libpng - PNG library by @glennrp](https://github.com/glennrp/libpng)
* [zlib - compression library by @madler](https://github.com/madler/zlib)
* [imgui - User interface library by @ocornut](https://github.com/ocornut/imgui)
* My amazing wife Katie: I love you.
