require 'tundra.syntax.glob'
require 'tundra.syntax.files'

local native = require "tundra.native"
local npath = require 'tundra.native.path'

function _G.GetPythonPath()
    local pyroot = native.getenv("PYTHON39_ROOT", nil);   
    if pyroot == nil then
        error("Ensure that the environment variable PYTHON39_ROOT is set, and points to the root python folder containing lib and include.")
    end
    return pyroot
end

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

SharedLibrary {
    Name = "coal",
    Pass = "BuildCode",
    Includes = LibIncludes,
    Libs = PyLibs,
    Sources = {
        Glob {
            Dir = SourceDir.."/coalpy",
            Extensions = { ".cpp", ".h", ".hpp" },
            Recursive =  true
        },
    }
}

Default("coal")

local function DeployPyd()
    local srcDll = "$(OBJECTDIR)$(SEP)coal.dll"
    local dstPyd = "$(OBJECTDIR)$(SEP)coal.pyd"
    local cpyFile = CopyFile {
        Pass = "Deploy",
        Source = srcDll,
        Target = dstPyd
    }

    Default(cpyFile)
end

DeployPyd()
