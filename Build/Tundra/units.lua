local SourceDir = "Source"
local ScriptsDir = "Source/scripts/coalpy"
local ImguiDir = "Source/imgui"
local ImplotDir = "Source/implot"
local LibJpgDir = "Source/libjpeg"
local LibPngDir = "Source/libpng"
local ZlibDir = "Source/zlib"
local cJSONDir = "Source/cjson"
local TinyObjLoaderDir = "Source/tinyobjloader"
local SpirvReflectDir = "Source/spirvreflect"
local DxcDir = "External/dxc/v1.7.2308/"
local DxcIncludes = DxcDir.."inc/"
local DxcLibDir = DxcDir.."lib/x64/"
local DxcBinaryCompiler = DxcDir.."bin/x64/dxcompiler.dll"
local DxcBinaryCompilerSo = DxcDir.."bin/x64/libdxcompiler.so"
local DxcBinaryIl = DxcDir.."bin/x64/dxil.dll"
local OpenEXRDir = "External/OpenEXR/"
local OpenEXRLibDir = "External/OpenEXR/staticlib/"
local PythonDir = "External/Python/"
local PixDir ="External/WinPixEventRuntime_1.0.210818001"
local PixBinaryDll ="External/WinPixEventRuntime_1.0.210818001/bin/WinPixEventRuntime.dll"
local WinVulkanDir = "External/Vulkan/1.3.231.1/"
local WinVulkanIncludes = WinVulkanDir .. "Include"
local WinVulkanLibs = WinVulkanDir .. "Lib"

local PythonModuleVersions =
{
    _G.WindowsPythonModuleTemplate("cp311-win_amd64", "311", PythonDir),
    _G.WindowsPythonModuleTemplate("cp310-win_amd64", "310", PythonDir),
    _G.WindowsPythonModuleTemplate("cp39-win_amd64",  "39",  PythonDir),
    _G.LinuxPythonModuleTemplate("cp311-linux-gcc_amd64", "3.11"),
    _G.LinuxPythonModuleTemplate("cp310-linux-gcc_amd64", "3.10")
}

local LibIncludes = {
    {
        WinVulkanIncludes,
        Config = "win64-*-*"
    },
    ImguiDir,
    ImplotDir,
    LibJpgDir,
    LibPngDir,
    cJSONDir,
    TinyObjLoaderDir
}

local Libraries = {
    {
        "Half-2_5_d.lib",
        "Iex-2_5_d.lib",
        "IexMath-2_5_d.lib",
        "IlmImf-2_5_d.lib",
        "IlmImfUtil-2_5_d.lib",
        "IlmThread-2_5_d.lib",
        "Imath-2_5_d.lib",
        Config = { "win64-msvc-debug-*" }
    },
    {
        "Half-2_5.lib",
        "Iex-2_5.lib",
        "IexMath-2_5.lib",
        "IlmImf-2_5.lib",
        "IlmImfUtil-2_5.lib",
        "IlmThread-2_5.lib",
        "Imath-2_5.lib",
        Config = { "win64-msvc-release", "win64-msvc-production"  }
    },
    {
        "User32.lib",
        "vulkan-1.lib",
        Config = "win64-msvc-*"
    },
    {
        "pthread",
        "dl",
        "vulkan",
        "SDL2",
        "Half",
        "Iex",
        "IexMath",
        "IlmImf",
        "IlmImfUtil",
        "IlmThread",
        "Imath",
        "dxclib",
        "LLVMDxcSupport",
        Config = "linux-*-*"
    }
}

local LibPaths = {
    OpenEXRLibDir,
    -- TODO (Apoorva): The following conditionals rely on the assumption that Windows and Linux are
    -- x64, and that MacOS is arm64. This is not a safe assumption, and should be fixed.
    {
        DxcDir.."lib/x64/",
        Config = { "win64-*-*", "linux-*-*" }
    },
    {
        DxcDir.."lib/arm64/",
        Config = "macosx-*-*"
    },
    -- /TODO
    {
        WinVulkanLibs,
        Config = "win64-msvc-*"
    }
}

local Binaries = {
    -- TODO (Apoorva): Build the new version (v1.7.2308) of DXC for Windows and Linux, and then uncomment the following lines.
    -- DxcBinaryCompiler,
    -- DxcBinaryCompilerSo,
    -- DxcBinaryIl,
    -- PixBinaryDll
}

-- Build zlib
local zlibLib = StaticLibrary {
    Name = "zlib",
    Pass = "BuildCode",
    Includes = { ZlibDir },
    Defines = {
        Config = { "macosx-*-*" },
        {
            "HAVE_UNISTD_H"
        }
    },
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
    Includes = {
        ImguiDir,
        {
            WinVulkanIncludes,
            Config = { "win64-msvc-*" }
        },
        {
            "/usr/include/SDL2/",
            Config = { "linux-gcc-*" }
        }
    },
    Sources = {
        Glob {
            Dir = ImguiDir,
            Extensions = { ".cpp", ".h", ".hpp" },
            Recursive = false
        },
        Glob {
            Dir = ImguiDir.."/cpp/",
            Extensions = { ".cpp", ".h", ".hpp" },
            Recursive = true
        },
        {
            Config = { "win64-msvc-*", "linux-gcc-*" },
            ImguiDir.."$(SEP)backends$(SEP)imgui_impl_vulkan.h",
            ImguiDir.."$(SEP)backends$(SEP)imgui_impl_vulkan.cpp",
        },
        {
            Config = { "win64-msvc-*" },
            {
                ImguiDir.."$(SEP)backends$(SEP)imgui_impl_dx12.cpp",
                ImguiDir.."$(SEP)backends$(SEP)imgui_impl_dx12.h",
                ImguiDir.."$(SEP)backends$(SEP)imgui_impl_win32.h",
                ImguiDir.."$(SEP)backends$(SEP)imgui_impl_win32.cpp",
            }
        },
        {
            Config = { "linux-gcc-*" },
            {
                ImguiDir.."$(SEP)backends$(SEP)imgui_impl_sdl.h",
                ImguiDir.."$(SEP)backends$(SEP)imgui_impl_sdl.cpp",
            }
        },
        {
            Config = { "macosx-*-*" },
            {
                ImguiDir.."$(SEP)backends$(SEP)imgui_impl_metal.h",
                ImguiDir.."$(SEP)backends$(SEP)imgui_impl_metal.mm",
            }
        }
    },
    IdeGenerationHints = _G.GenRootIdeHints("imgui");
}

Default(imguiLib)

-- Build implot
local implotLib = StaticLibrary {
    Name = "implot",
    Pass = "BuildCode",
    Includes = {
        ImguiDir
    },
    Sources = {
        Glob {
            Dir = ImplotDir,
            Extensions = { ".cpp", ".h", ".hpp" },
            Recursive = true
        },
    },
    IdeGenerationHints = _G.GenRootIdeHints("implot");
}

Default(implotLib)

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

-- Build spirvreflect
local spirvreflect = StaticLibrary {
    Name = "spirvreflect",
    Pass = "BuildCode",
    Includes = SpirvReflectDir,
    Sources = {
        Glob {
            Dir = SpirvReflectDir,
            Extensions = { ".cpp", ".h", ".c" },
            Recursive = true
        }
    },
    IdeGenerationHints = _G.GenRootIdeHints("spirvreflect");
}

Default (spirvreflect)

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

-- Build tinyobjloader
local tinyobjloader = StaticLibrary {
    Name = "tinyobjloader",
    Pass = "BuildCode",
    Includes = { TinyObjLoaderDir },
    Sources = {
        Glob {
            Dir = TinyObjLoaderDir,
            Extensions = { ".cc", ".h" },
            Recursive =  true
        }
    },
    IdeGenerationHints = _G.GenRootIdeHints("tinyobjloader");
}

-- Build cJSON
local cjson = StaticLibrary {
    Name = "cjson",
    Pass = "BuildCode",
    Includes = cJSONDir,
    Sources = {
        Glob {
            Dir = cJSONDir,
            Extensions = { ".c", ".h" },
            Recursive =  true
        }
    },
    IdeGenerationHints = _G.GenRootIdeHints("cjson");
}

Default (cjson)

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
    render = {
        DxcIncludes,
        ImguiDir,
        ImplotDir,
        SpirvReflectDir,
        cJSONDir,
        "Source/modules/window",
        {
            WinVulkanIncludes,
            Config = "win64-*-*"
        }
    },
    texture = { LibJpgDir, LibPngDir, ZlibDir,
        {
            OpenEXRDir.."include/OpenEXR",
            Config = "win64-*-*"
        },
        {
            "/usr/include/OpenEXR",
            Config = "linux-*-*"
        },
        {
            OpenEXRDir.."include/OpenEXR",
            Config = "macosx-*-*"
        }
    }
}

local CoalPyModuleDeps = {
    render = { imguiLib, implotLib, spirvreflect, tinyobjloader, cjson },
    texture = { zlibLib, libpngLib, libjpegLib }
}

-- Module list for the core coalpy module
local CoalPyModules = { "core", "tasks", "render", "files", "window", "texture", "libjpeg", "libpng", "zlib", "spirvreflect"  }

_G.BuildModules(SourceDir, CoalPyModuleTable, CoalPyModuleIncludes, CoalPyModuleDeps)
_G.BuildPyLibs(
    PythonModuleVersions, "gpu", "pymodules/gpu",
    SourceDir, LibIncludes, CoalPyModules, Libraries,
    { _G.GetModuleDeps(CoalPyModuleTable, CoalPyModuleDeps), "core" },
    LibPaths)

_G.DeployPyPackage("coalpy", "gpu", PythonModuleVersions, Binaries, ScriptsDir)
_G.BuildProgram("coalpy_tests", "tests", { "CPY_ASSERT_ENABLED=1" }, SourceDir, LibIncludes, CoalPyModules, Libraries, LibPaths)

-- Deploy PIP package
_G.DeployPyPackage("coalpy_pip/src/coalpy", "gpu", PythonModuleVersions, Binaries, ScriptsDir)
_G.DeployPyPipFiles("coalpy_pip", SourceDir.."/../README.md", SourceDir.."/pipfiles")
