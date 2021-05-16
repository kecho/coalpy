local SourceDir = "Source";

local LibIncludes = {
    _G.GetPythonPath() .. "/include"
}

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

-- C++ module table-> module name with its dependencies
local CoalPyModuleTable = {
    core    = {},
    tasks   = { "core" },
    files   = { "core", "tasks" },
    shader  = { "core", "tasks", "files" }
}

-- Module list for the core coalpy module
local CoalPyModules = { "core", "shader", "tasks", "files"  }


_G.BuildModules(SourceDir, CoalPyModuleTable)
_G.BuildPyLib("coalpy", SourceDir, LibIncludes, CoalPyModules, PyLibs)
_G.BuildProgram("coalpy_tests", "tests", { "CPY_ASSERT_ENABLED=1" }, SourceDir, LibIncludes, CoalPyModules, PyLibs)
_G.DeployPyLib("coalpy")
