#pragma once

#include <coalpy.core/GenericHandle.h>
#include <coalpy.core/HandleContainer.h>
#include <coalpy.render/IShaderDb.h>
#include <coalpy.files/IFileWatcher.h>
#include <coalpy.files/Utils.h>
#include <DxcCompiler.h>
#include <shared_mutex>
#include <atomic>
#include <set>

namespace coalpy
{

struct CompileState;

namespace render
{
    class IDevice;
}

typedef void* ShaderGPUPayload;

class BaseShaderDb : public IShaderDb, public IFileWatchListener
{
public:
    explicit BaseShaderDb(const ShaderDbDesc& desc);
    virtual ShaderHandle requestCompile(const ShaderDesc& desc) override;
    virtual ShaderHandle requestCompile(const ShaderInlineDesc& desc) override;
    virtual void addPath(const char* path) override;
    virtual void resolve(ShaderHandle handle) override;
    virtual bool isValid(ShaderHandle handle) const override;
    virtual void onFilesChanged(const std::set<std::string>& filesChanged) override;
    virtual ~BaseShaderDb();

    void requestRecompile(ShaderHandle handle);

    void setParentDevice(render::IDevice* device) { m_parentDevice = device; }
    render::IDevice* parentDevice() const { return m_parentDevice; }

private:
    void preparePdbDir();
    void prepareIoJob(CompileState& state, const std::string& resolvedPath);
    void prepareCompileJobs(CompileState& state);

    ShaderDbDesc m_desc;
    DxcCompiler m_compiler;

    struct ShaderFileRecipe
    {
        ShaderType type;
        std::string name;
        std::string mainFn;
        std::string path;
        std::string source;
        std::vector<std::string> defines;
    };

    struct ShaderState
    {
        bool ready;
        bool success;
        ShaderFileRecipe recipe;
        std::string debugName;
        IDxcBlob* shaderBlob;
        std::atomic<bool> compiling;
        CompileState* compileState;
        std::atomic<ShaderGPUPayload> payload;
    };

    ShaderState& createShaderState(ShaderHandle& outHandle);

    mutable std::shared_mutex m_shadersMutex;
    HandleContainer<ShaderHandle, ShaderState*> m_shaders;

    void startLiveEdit();
    void stopLiveEdit();
    IFileWatcher* m_liveEditWatcher;
    mutable std::shared_mutex m_dependencyMutex;
    using FileToShaderHandlesMap = std::unordered_map<FileLookup, std::set<ShaderHandle>>;
    using ShaderHandleToFilesMap = std::unordered_map<ShaderHandle, std::set<FileLookup>>;
    FileToShaderHandlesMap m_fileToShaders;
    ShaderHandleToFilesMap m_shadersToFiles;
    render::IDevice* m_parentDevice = nullptr;
    std::vector<std::string> m_additionalPaths;

    bool m_pdbDirReady = false;
    bool m_createdPdbDir = false;
};

}
