local SourceDir = "Source";
local DxcDir = "External/dxc_2021_04-20/"
local DxcIncludes = DxcDir
local DxcBinary = DxcDir.."dxc/bin/x64/dxcompiler.dll"
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
    }
}

local Binaries = {
    DxcBinary
}

-- C++ module table-> module name with its dependencies
local CoalPyModuleTable = {
    core    = {},
    tasks   = { "core" },
    files   = { "core", "tasks" },
    shader  = { "core", "tasks", "files", "render" },
    render  = { "core", "tasks", "files" }
}

-- C++ module external includes
local CoalPyModuleIncludes = {
    render = { DxcIncludes }
}

-- Module list for the core coalpy module
local CoalPyModules = { "core", "shader", "tasks", "render", "files"  }

_G.BuildModules(SourceDir, CoalPyModuleTable, CoalPyModuleIncludes)
_G.BuildPyLib("coalpy", SourceDir, LibIncludes, CoalPyModules, Libraries)
_G.BuildProgram("coalpy_tests", "tests", { "CPY_ASSERT_ENABLED=1" }, SourceDir, LibIncludes, CoalPyModules, Libraries)
_G.DeployPyLib("coalpy", Binaries)
