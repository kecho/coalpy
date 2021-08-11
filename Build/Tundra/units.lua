local SourceDir = "Source";
local ScriptsDir = "Source/scripts/coalpy";
local ImguiDir = "Source/imgui";
local LibJpgDir = "Source/libjpeg";
local LibPngDir = "Source/libpng";
local ZlibDir = "Source/zlib";
local DxcDir = "External/dxc_2021_04-20/"
local DxcIncludes = DxcDir
local DxcBinaryCompiler = DxcDir.."dxc/bin/x64/dxcompiler.dll"
local DxcBinaryIl = DxcDir.."dxc/bin/x64/dxil.dll"
local PythonDir = "External/Python39-win64/"
local OpenEXRDir = "External/OpenEXR/"
local OpenEXRLibDir = "External/OpenEXR/staticlib/"


local LibIncludes = {
    PythonDir .. "Include",
    ImguiDir,
    LibJpgDir,
    LibPngDir
}

local Libraries = {
    {
        PythonDir.."python39_d.lib",
        OpenEXRLibDir.."Half-2_5_d.lib",
        OpenEXRLibDir.."Iex-2_5_d.lib",
        OpenEXRLibDir.."IexMath-2_5_d.lib",
        OpenEXRLibDir.."IlmImf-2_5_d.lib",
        OpenEXRLibDir.."IlmImfUtil-2_5_d.lib",
        OpenEXRLibDir.."IlmThread-2_5_d.lib",
        OpenEXRLibDir.."Imath-2_5_d.lib",
        Config = { "win64-msvc-debug-*" }
    },
    {
        PythonDir.."python39.lib",
        OpenEXRLibDir.."Half-2_5.lib",
        OpenEXRLibDir.."Iex-2_5.lib",
        OpenEXRLibDir.."IexMath-2_5.lib",
        OpenEXRLibDir.."IlmImf-2_5.lib",
        OpenEXRLibDir.."IlmImfUtil-2_5.lib",
        OpenEXRLibDir.."IlmThread-2_5.lib",
        OpenEXRLibDir.."Imath-2_5.lib",
        Config = { "win64-msvc-release", "win64-msvc-production"  }
    },
    "User32.lib"
}

local Binaries = {
    DxcBinaryCompiler,
    DxcBinaryIl
}

-- Build zlib
local zlibLib = StaticLibrary {
    Name = "zlib",
    Pass = "BuildCode",
    Includes = { ZlibDir },
    Sources = {
        Glob {
            Dir = ZlibDir,
            Extensions = { ".c", ".h" },
            Recursive =  true
        }
    },
    IdeGenerationHints = _G.GenRootIdeHints("zlib");
}

Default(zlibLib)

-- Build imgui
local imguiLib = StaticLibrary {
    Name = "imgui",
    Pass = "BuildCode",
    Includes = ImguiDir,
    Sources = {
        Glob {
            Dir = ImguiDir,
            Extensions = { ".cpp", ".h", ".hpp" },
            Recursive =  true
        }
    },
    IdeGenerationHints = _G.GenRootIdeHints("imgui");
}

Default(imguiLib)

-- Build libjpeg
local libjpegLib = StaticLibrary {
    Name = "libjpeg",
    Pass = "BuildCode",
    Includes = LibJpgDir,
    Sources = {
        Glob {
            Dir = LibJpgDir,
            Extensions = { ".c", ".h" },
            Recursive =  true
        }
    },
    IdeGenerationHints = _G.GenRootIdeHints("libjpeg");
}

Default (libjpegLib)

-- Build libpng
local libpngLib = StaticLibrary {
    Name = "libpng",
    Pass = "BuildCode",
    Includes = { LibPngDir, ZlibDir },
    Depends = { zlibLib },
    Sources = {
        Glob {
            Dir = LibPngDir,
            Extensions = { ".c", ".h" },
            Recursive =  true
        }
    },
    IdeGenerationHints = _G.GenRootIdeHints("libpng");
}

Default (libpngLib)

-- C++ module table-> module name with its dependencies
local CoalPyModuleTable = {
    core    = {},
    tasks   = { "core" },
    files   = { "core", "tasks" },
    texture = { "core", "tasks", "files", "render" },
    render  = { "core", "tasks", "files", "window" },
    window  = { "core" }
}

-- C++ module external includes
local CoalPyModuleIncludes = {
    render = { DxcIncludes, ImguiDir, "Source/modules/window/" },
    texture = { LibJpgDir, LibPngDir, ZlibDir, OpenEXRDir.."include/" }
}

local CoalPyModuleDeps = {
    render = { imguiLib },
    texture = { zlibLib, libpngLib, libjpegLib }
}

-- Module list for the core coalpy module
local CoalPyModules = { "core", "tasks", "render", "files", "window", "texture", "libjpeg", "libpng", "zlib"  }

_G.BuildModules(SourceDir, CoalPyModuleTable, CoalPyModuleIncludes, CoalPyModuleDeps)
_G.BuildPyLib("gpu", "pymodules/gpu", SourceDir, LibIncludes, CoalPyModules, Libraries, CoalPyModuleDeps.render)
_G.BuildProgram("coalpy_tests", "tests", { "CPY_ASSERT_ENABLED=1" }, SourceDir, LibIncludes, CoalPyModules, Libraries)
_G.DeployPyPackage("coalpy", "gpu", Binaries, ScriptsDir)

-- Deploy PIP package
_G.DeployPyPackage("coalpy_pip/src/coalpy", "gpu", Binaries, ScriptsDir)
_G.DeployPyPipFiles("coalpy_pip", SourceDir.."/../README.md", SourceDir.."/pipfiles")
