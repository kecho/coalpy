#include <coalpy.core/GenericHandle.h>
#include <coalpy.core/HandleContainer.h>
#include <coalpy.render/IShaderDb.h>
#include <coalpy.files/IFileWatcher.h>
#include <shared_mutex>
#include "Dx12Compiler.h"
#include <atomic>
#include <set>

struct ID3D12PipelineState;

namespace coalpy
{

struct Dx12CompileState;

struct Dx12FileLookup
{
    std::string filename;
    unsigned int hash;

    Dx12FileLookup();
    Dx12FileLookup(const char* filename);
    Dx12FileLookup(const std::string& filename);

    bool operator==(const Dx12FileLookup& other) const
    {
        return hash == other.hash;
    }

    bool operator>(const Dx12FileLookup& other) const
    {
        return hash > other.hash;
    }

    bool operator>=(const Dx12FileLookup& other) const
    {
        return hash >= other.hash;
    }

    bool operator<(const Dx12FileLookup& other) const
    {
        return hash < other.hash;
    }

    bool operator<=(const Dx12FileLookup& other) const
    {
        return hash < other.hash;
    }
};

}

namespace std
{
    template<>
    struct hash<coalpy::Dx12FileLookup>
    {
        std::size_t operator()(const coalpy::Dx12FileLookup& key) const
        {
            return (std::size_t)key.hash;
        }
    };
}

namespace coalpy
{

namespace render
{
    class Dx12Device;
}

class Dx12ShaderDb : public IShaderDb, public IFileWatchListener
{
public:
    explicit Dx12ShaderDb(const ShaderDbDesc& desc);
    virtual ShaderHandle requestCompile(const ShaderDesc& desc) override;
    virtual ShaderHandle requestCompile(const ShaderInlineDesc& desc) override;
    virtual void addPath(const char* path) override;
    virtual void resolve(ShaderHandle handle) override;
    virtual bool isValid(ShaderHandle handle) const override;
    virtual void onFilesChanged(const std::set<std::string>& filesChanged) override;
    virtual ~Dx12ShaderDb();

    void requestRecompile(ShaderHandle handle);

    void setParentDevice(render::Dx12Device* device) { m_parentDevice = device; }
    render::Dx12Device* parentDevice() const { return m_parentDevice; }

    ID3D12PipelineState* unsafeGetCsPso(ShaderHandle handle)
    {
        return m_shaders[handle]->csPso;
    }

private:
    void prepareIoJob(Dx12CompileState& state, const std::string& resolvedPath);
    void prepareCompileJobs(Dx12CompileState& state);

    ShaderDbDesc m_desc;
    Dx12Compiler m_compiler;

    struct ShaderFileRecipe
    {
        ShaderType type;
        std::string name;
        std::string mainFn;
        std::string path;
        std::string source;
    };

    struct ShaderState
    {
        bool ready;
        bool success;
        ShaderFileRecipe recipe;
        std::string debugName;
        IDxcBlob* shaderBlob;
        std::atomic<bool> compiling;
        Dx12CompileState* compileState;
        std::atomic<ID3D12PipelineState*> csPso;
    };

    bool updateComputePipelineState(ShaderState& state);

    ShaderState& createShaderState(ShaderHandle& outHandle);

    mutable std::shared_mutex m_shadersMutex;
    HandleContainer<ShaderHandle, ShaderState*> m_shaders;

    void startLiveEdit();
    void stopLiveEdit();
    IFileWatcher* m_liveEditWatcher;
    mutable std::shared_mutex m_dependencyMutex;
    using FileToShaderHandlesMap = std::unordered_map<Dx12FileLookup, std::set<ShaderHandle>>;
    using ShaderHandleToFilesMap = std::unordered_map<ShaderHandle, std::set<Dx12FileLookup>>;
    FileToShaderHandlesMap m_fileToShaders;
    ShaderHandleToFilesMap m_shadersToFiles;
    render::Dx12Device* m_parentDevice = nullptr;
    std::vector<std::string> m_additionalPaths;
};

}
