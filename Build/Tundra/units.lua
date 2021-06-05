local SourceDir = "Source";
local ScriptsDir = "Scripts/coalpy";
local DxcDir = "External/dxc_2021_04-20/"
local DxcIncludes = DxcDir
local DxcBinaryCompiler = DxcDir.."dxc/bin/x64/dxcompiler.dll"
local DxcBinaryIl = DxcDir.."dxc/bin/x64/dxil.dll"
local PythonDir = "External/Python39-win64/"


local LibIncludes = {
    _G.GetPythonPath() .. "/include"
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
    "User32.lib"
}

local Binaries = {
    DxcBinaryCompiler,
    DxcBinaryIl
}

-- C++ module table-> module name with its dependencies
local CoalPyModuleTable = {
    core    = {},
    tasks   = { "core" },
    files   = { "core", "tasks" },
    render  = { "core", "tasks", "files" },
    window  = { "core" }
}

-- C++ module external includes
local CoalPyModuleIncludes = {
    render = { DxcIncludes, "Source/render/" }
}

-- Module list for the core coalpy module
local CoalPyModules = { "core", "tasks", "render", "files", "window"  }

_G.BuildModules(SourceDir, CoalPyModuleTable, CoalPyModuleIncludes)
_G.BuildPyLib("gpu", "pymodules/gpu", SourceDir, LibIncludes, CoalPyModules, Libraries)
_G.BuildProgram("coalpy_tests", "tests", { "CPY_ASSERT_ENABLED=1" }, SourceDir, LibIncludes, CoalPyModules, Libraries)
_G.DeployPyPackage("coalpy", "gpu", Binaries, ScriptsDir)
