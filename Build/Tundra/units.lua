local SourceDir = "Source";
local ScriptsDir = "Source/scripts/coalpy";
local ImguiDir = "Source/imgui";
local LibJpgDir = "Source/libjpeg";
local LibPngDir = "Source/libpng";
local DxcDir = "External/dxc_2021_04-20/"
local DxcIncludes = DxcDir
local DxcBinaryCompiler = DxcDir.."dxc/bin/x64/dxcompiler.dll"
local DxcBinaryIl = DxcDir.."dxc/bin/x64/dxil.dll"
local PythonDir = "External/Python39-win64/"


local LibIncludes = {
    _G.GetPythonPath() .. "/include",
    ImguiDir,
    LibJpgDir,
    LibPngDir
}

local Libraries = {
    {
        PythonDir.."python39_d.lib",
        Config = { "win64-msvc-debug-*" }
    },
    {
        PythonDir.."python39.lib",
        Config = { "win64-msvc-release", "win64-msvc-production"  }
    },
    "External/Zlib/lib/zlibstatic.lib",
    "User32.lib"
}

local Binaries = {
    DxcBinaryCompiler,
    DxcBinaryIl
}

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
    Includes = { LibPngDir, "External/Zlib/include" },
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
    render  = { "core", "tasks", "files", "window" },
    window  = { "core" }
}

-- C++ module external includes
local CoalPyModuleIncludes = {
    render = { DxcIncludes, ImguiDir, "Source/modules/window/" }
}

local CoalPyModuleDeps = {
    render = { imguiLib, libjpegLib, libpngLib }
}

-- Module list for the core coalpy module
local CoalPyModules = { "core", "tasks", "render", "files", "window"  }

_G.BuildModules(SourceDir, CoalPyModuleTable, CoalPyModuleIncludes, CoalPyModuleDeps)
_G.BuildPyLib("gpu", "pymodules/gpu", SourceDir, LibIncludes, CoalPyModules, Libraries, CoalPyModuleDeps.render)
_G.BuildProgram("coalpy_tests", "tests", { "CPY_ASSERT_ENABLED=1" }, SourceDir, LibIncludes, CoalPyModules, Libraries)
_G.DeployPyPackage("coalpy", "gpu", Binaries, ScriptsDir)

-- Deploy PIP package
_G.DeployPyPackage("coalpy_pip/src/coalpy", "gpu", Binaries, ScriptsDir)
_G.DeployPyPipFiles("coalpy_pip", SourceDir.."/../README.md", SourceDir.."/pipfiles")
