require 'tundra.syntax.glob'
require 'tundra.syntax.files'

local native = require "tundra.native"
local path     = require "tundra.path"

function _G.GetPythonPath()
    local pyroot = native.getenv("PYTHON39_ROOT", nil);   
    if pyroot == nil then
        error("Ensure that the environment variable PYTHON39_ROOT is set, and points to the root python folder containing lib and include.")
    end
    return pyroot
end

function _G.GetModulePublicInclude(sourceDir, moduleName)
    return sourceDir.."\\"..moduleName.."\\public\\"
end

function _G.GetModuleIncludes(sourceDir, moduleList)
    local includeList = {}
    for i, v in ipairs(moduleList) do
        table.insert(includeList, GetModulePublicInclude(sourceDir, v))
    end
    return includeList
end

function _G.GetModuleDeps(moduleList)
    local depList = {}
    for i, v in ipairs(moduleList) do
        table.insert(depList, v)
    end
    return depList
end

function _G.BuildModules(sourceDir, moduleMap)
    for k,v in pairs(moduleMap) do
        local moduleLib = StaticLibrary {
            Name = k,
            Pass = "BuildCode",
            Depends = { _G.GetModuleDeps(v) },
            Includes = {
                _G.GetModuleIncludes(sourceDir, v),
                _G.GetModulePublicInclude(sourceDir, k)
            },
            Sources = {
                Glob {
                    Dir = sourceDir.."/"..k,
                    Extensions = { ".cpp", ".h", ".hpp" },
                    Recursive =  true
                }
            }
        }
        Default(moduleLib)
    end
end

function _G.BuildPyLib(libName, sourceDir, includeList, moduleList, otherDeps)
    local pythonLib = SharedLibrary {
        Name = libName,
        Pass = "BuildCode",
        Includes = { _G.GetModuleIncludes(sourceDir, moduleList), includeList },
        Depends = { 
            _G.GetModuleDeps(moduleList)
        },
        Libs = {
            otherDeps
        },
        Sources = {
            Glob {
                Dir = sourceDir.."/"..libName,
                Extensions = { ".cpp", ".h", ".hpp" },
                Recursive =  true
            },
        }
    }

    Default(pythonLib)
end

function _G.BuildProgram(programName, programSource, defines, sourceDir, includeList, moduleList, otherDeps)
    local prog = Program {
        Name = programName,
        Pass = "BuildCode",
        Env = {
            CPPDEFS = defines
        },
        Includes = { _G.GetModuleIncludes(sourceDir, moduleList), includeList },
        Depends = { 
            _G.GetModuleDeps(moduleList)
        },
        Libs = {
            otherDeps
        },
        Sources = {
            Glob {
                Dir = sourceDir.."/"..programSource,
                Extensions = { ".cpp", ".h", ".hpp" },
                Recursive =  true
            },
        }
    }

    Default(prog)
end

function _G.DeployPyLib(pythonLibName)
    local srcDll = "$(OBJECTDIR)$(SEP)"..pythonLibName..".dll"
    local dstPyd = "$(OBJECTDIR)$(SEP)"..pythonLibName..".pyd"
    local cpyFile = CopyFile {
        Pass = "Deploy",
        Source = srcDll,
        Target = dstPyd
    }

    Default(cpyFile)
end

