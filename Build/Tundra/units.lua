local SourceDir = "Source";

local LibIncludes = {
    _G.GetPythonPath() .. "/include"
}

local CoalPyModuleTable = {
    core = {},
    shader = { "core" }
}

local CoalPyModules = { "core", "shader" }

local PyLibs = {
    {
        "External/Python39-win64/python39_d.lib",
        Config = { "win64-msvc-debug-*" }
    },
    {
        "External/Python39-win64/python39.lib",
        Config = { "win64-msvc-release", "win64-msvc-production"  }
    }
}

_G.BuildModules(SourceDir, CoalPyModuleTable)
_G.BuildPyLib("coalpy", SourceDir, LibIncludes, CoalPyModules, PyLibs)
_G.DeployPyLib("coalpy")
