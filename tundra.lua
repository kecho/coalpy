
Build {
    Units = {
        "Build/Tundra/framework.lua",
        "Build/Tundra/units.lua"
    },

    Passes = {
        CodeGeneration = { Name = "CodeGeneration" , BuildOrder = 1 },
        BuildCode = { Name = "BuildCode", BuildOrder = 2 },
        Deploy = { Name = "Deploy", BuildOrder = 3 }
    },

    Configs = {
        {
            Name = "win64-msvc",
            DefaultOnHost = "windows",
            Tools = { "msvc-vs2019"  },
            Env = {
                CPPPATH = {
                    "$(OBJECTDIR)$(SEP)Source"
                },
                SHLIBOPTS = "/NODEFAULTLIB:library",
                CXXOPTS = { "/EHsc", "/std:c++17" },
                CXXOPTS_DEBUG   = { "/Od", "/MDd",  "/D \"_DEBUG\"" },
                CXXOPTS_RELEASE = { "/Ox", "/MD" },
                CXXOPTS_PRODUCTION = { "/O2", "/MD" },
            },
        },
        {
            Name = "linux-gcc",
            DefaultOnHost = "linux",
            Tools = { "gcc" },
            Env = {
                CPPPATH = {
                    "$(OBJECTDIR)$(SEP)Source"
                },
                CXXOPTS = { "-std=c++17", "-fPIC", "-Wno-multichar", "-Wno-write-strings", "-mlzcnt" },
                CXXOPTS_DEBUG   = { "-ggdb",  "-D _DEBUG=1" },
                CCOPTS = { "-fPIC", "-Wno-multichar" },
            },
            ReplaceEnv = {
                LD = "$(CXX)"
            },
        },
        {
            Name = "macosx-clang",
            DefaultOnHost = "macosx",
            Tools = { "clang" },
            Env = {
                CPPPATH = {
                    "$(OBJECTDIR)$(SEP)Source"
                },
                CXXOPTS = { "-std=c++17", "-fPIC", "-Wno-multichar", "-Wno-write-strings", "-fms-extensions", "-x objective-c++" },
                CXXOPTS_DEBUG   = { "-g",  "-D _DEBUG=1" },
                CCOPTS = { "-fPIC", "-Wno-multichar" },
            },
            ReplaceEnv = {
                -- Ideally the MetalKit framework should not need to be linked.
                -- But for some reason, MTLCreateSystemDefaultDevice() returns nil.
                LD = "$(CXX) -framework Foundation -framework CoreFoundation -framework CoreServices -framework Metal -framework MetalKit"
            },
        },
    },
    
    Env = {
        GENERATE_PDB = "1"
    },

    IdeGenerationHints = {
        Msvc = {
            -- Remap config names to MSVC platform names (affects things like header scanning & debugging)
            PlatformMappings = {
                ['win64-msvc'] = 'Win',
            },
            -- Remap variant names to MSVC friendly names
            VariantMappings = {
                ['release-default']    = 'release',
                ['debug-default']  = 'debug',
                ['production-default']    = 'production',
            },
        },
        
        -- Override solutions to generate and what units to put where.
        MsvcSolutions = {
            ['coalpy.sln'] = {},          -- receives all the units due to empty set
        },
        
        BuildAllByDefault = true,
    }
}
