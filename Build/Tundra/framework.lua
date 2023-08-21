require 'tundra.syntax.glob'
require 'tundra.syntax.files'

local native = require "tundra.native"
local path  = require "tundra.path"
local npath = require 'tundra.native.path'

function _G.GenRootIdeHints(rootFolder)
   return {
        Msvc = {
            SolutionFolder = rootFolder
        }
    }
end

function _G.WindowsPythonModuleTemplate(tag, version, pythonDir)
    return {
        Tag = tag,
        Includes = pythonDir..version.."/Include",
        Libs =
        {
            {
                pythonDir..version.."/python"..version.."_d.lib",
                Config = "win64-msvc-debug"
            },
            {
                pythonDir..version.."/python"..version..".lib",
                Config = { "win64-msvc-release", "win64-msvc-production" }
            }
        },
        Config = "win64-msvc-*",
        Prefix = "",
        Ext = ".dll",
        PyModExt = ".pyd"
    }
end

function _G.LinuxPythonModuleTemplate(tag, version, pythonDir)
    return {
        Tag = tag,
        Includes = "/usr/include/python"..version,
        Libs = {},
        Config = "linux-*-*",
        Prefix = "lib",
        Ext = ".so",
        PyModExt = ".so"
    }
end

function _G.GetModuleDir(sourceDir, moduleName)
    return sourceDir.."/modules/"..moduleName.."/"
end

function _G.GetModulePublicInclude(sourceDir, moduleName)
    return _G.GetModuleDir(sourceDir, moduleName).."public/"
end

function _G.GetModuleIncludes(sourceDir, moduleList)
    local includeList = {}
    for i, v in ipairs(moduleList) do
        table.insert(includeList, GetModulePublicInclude(sourceDir, v))
    end
    return includeList
end

function _G.GetModuleDeps(moduleList, moduleDeps)
    local depList = {}
    for i, v in pairs(moduleList) do
        if moduleDeps[i] ~= nil then
            table.insert(depList, moduleDeps[i])
        end
    end
    return depList
end

function _G.BuildModules(sourceDir, moduleMap, extraIncludes, extraDeps)
    for k,v in pairs(moduleMap) do
        local externalIncludes = extraIncludes[k]
        local extraModDeps = extraDeps[k]
        local moduleLib = StaticLibrary {
            Name = k,
            Pass = "BuildCode",
            Depends = { v, extraModDeps },
            Includes = {
                _G.GetModuleDir(sourceDir, k),
                _G.GetModuleIncludes(sourceDir, v),
                _G.GetModulePublicInclude(sourceDir, k),
                externalIncludes
            },
            Sources = {
                FGlob {
                    Dir = _G.GetModuleDir(sourceDir, k),
                    Extensions = { ".cpp", ".h", ".hpp" },
                    Filters = {
                        { Pattern = "dx12", Config = "win64-*-*" },
                        Pattern = "vulkan",
                    },
                    Recursive =  true
                }
            },

            IdeGenerationHints = _G.GenRootIdeHints("Modules");
        }
        Default(moduleLib)
    end
end

function _G.BuildPyLib(libName, libFolder, sourceDir, includeList, moduleList, otherLibs, otherDeps, otherLibPaths, targetConfig)
    local pythonLib = SharedLibrary {
        Name = libName,
        Pass = "BuildCode",
        Includes = { _G.GetModuleIncludes(sourceDir, moduleList), includeList },
        Depends = { 
            moduleList,
            otherDeps
        },
        Libs = {
            otherLibs,
        },
        Sources = {
            Glob {
                Dir = sourceDir.."/"..libFolder,
                Extensions = { ".cpp", ".h", ".hpp" },
                Recursive =  true
            },
        },
        LibPaths = otherLibPaths,
        IdeGenerationHints = _G.GenRootIdeHints("PyModules");
        Config = { targetConfig }
    }

    Default(pythonLib)
end

function _G.BuildPyLibs(
    pythonModuleVersions,
    moduleName,
    libFolder,
    sourceDir,
    includeList,
    moduleList,
    otherLibs,
    otherDeps,
    otherLibPaths)

    for i, v in ipairs(pythonModuleVersions) do
        local dllName = moduleName.."."..v.Tag
        _G.BuildPyLib(
            dllName,
            libFolder,
            sourceDir,
            { includeList, v.Includes },
            moduleList,
            { otherLibs, v.Libs },
            otherDeps,
            otherLibPaths,
            v.Config)
    end
end

function _G.BuildProgram(programName, programSource, defines, sourceDir, includeList, moduleList, otherLibs, otherLibPaths)
    local prog = Program {
        Name = programName,
        Pass = "BuildCode",
        Env = {
            CPPDEFS = defines
        },
        Includes = { _G.GetModuleIncludes(sourceDir, moduleList), includeList },
        Depends = { 
            moduleList
        },
        Libs = {
           otherLibs 
        },
        Sources = {
            Glob {
                Dir = sourceDir.."/"..programSource,
                Extensions = { ".cpp", ".h", ".hpp" },
                Recursive =  true
            },
        },
        LibPaths = otherLibPaths,
        IdeGenerationHints = _G.GenRootIdeHints("Apps");
    }

    Default(prog)
end

function _G.DeployPyPackage(packageName, pythonLibName, pythonModuleVersions, binaries, scriptsDir)
    local packageDst = "$(OBJECTDIR)$(SEP)"..packageName
    local binaryDst = packageDst .. "$(SEP)resources$(SEP)"
    for i, v in ipairs(pythonModuleVersions) do
        local cpyPyModuleCmd = CopyFile {
                    Pass = "Deploy",
                    Source = "$(OBJECTDIR)$(SEP)"..v.Prefix..pythonLibName.."."..v.Tag..v.Ext,
                    Target = packageDst.."$(SEP)"..v.Prefix..pythonLibName.."."..v.Tag..v.PyModExt,
                    Config=v.Config
                }
        Default(cpyPyModuleCmd)
    end

    for i, v in ipairs(binaries) do
        local targetName =  binaryDst..npath.get_filename(v)
        local cpCmd = CopyFile {
            Pass = "Deploy",
            Source = v,
            Target = targetName,
        }
        Default(cpCmd)
    end

    local scripts = Glob {
        Dir = scriptsDir,
        Extensions = { ".py", ".hlsl", "READNME", ".jpg", ".png" },
        Recursive = true
    }

    for i, v in ipairs(scripts) do
        local targetName = packageDst.."$(SEP)"..path.remove_prefix(scriptsDir.."/", v)
        local cpCmd = CopyFile {
            Pass = "Deploy",
            Source = v,
            Target = targetName
        }
        Default(cpCmd)
    end
end

function _G.DeployPyPipFiles(destFolder, readmefile, pipfiles)
    local packageDst = "$(OBJECTDIR)$(SEP)"..destFolder
    local cpReadme = CopyFile {
        Pass = "Deploy",
        Source = readmefile,
        Target = packageDst.."$(SEP)README.md"
    }
    Default(cpReadme)

    local cpLicense = CopyFile {
        Pass = "Deploy",
        Source = pipfiles.."/LICENSE",
        Target = packageDst.."$(SEP)LICENSE"
    }
    Default(cpLicense)

    local pipMetaFiles = Glob {
        Dir = pipfiles,
        Extensions = { "LICENSE", ".toml", ".cfg", ".in" },
        Recursive = true
    }
    for i, v in ipairs(pipMetaFiles) do
        local targetName = packageDst.."$(SEP)"..path.remove_prefix(pipfiles.."/", v)
        local cpFile = CopyFile {
            Pass = "Deploy",
            Source = v,
            Target = targetName
        }
        Default(cpFile)
    end
end

