#include <coalpy.core/GenericHandle.h>
#include <coalpy.core/HandleContainer.h>
#include <coalpy.render/IShaderDb.h>
#include <shared_mutex>
#include "Dx12Compiler.h"
#include <atomic>
#include <set>

namespace coalpy
{

struct Dx12CompileState;

class Dx12ShaderDb : public IShaderDb
{
public:
    explicit Dx12ShaderDb(const ShaderDbDesc& desc);
    virtual ShaderHandle requestCompile(const ShaderDesc& desc) override;
    virtual ShaderHandle requestCompile(const ShaderInlineDesc& desc) override;
    virtual void resolve(ShaderHandle handle) override;
    virtual bool isValid(ShaderHandle handle) const override;
    virtual ~Dx12ShaderDb();

private:
    void prepareCompileJobs(Dx12CompileState& state);

    ShaderDbDesc m_desc;
    Dx12Compiler m_compiler;

    struct ShaderFileRecipe
    {
        ShaderType type;
        std::string name;
        std::string mainFn;
        std::string path;
    };

    struct ShaderState
    {
        bool ready;
        bool success;
        ShaderFileRecipe recipe;
        bool hasRecipe;
        IDxcBlob* shaderBlob;
        std::atomic<bool> compiling;
        Dx12CompileState* compileState;
    };

    ShaderState& createShaderState(ShaderHandle& outHandle);

    mutable std::shared_mutex m_shadersMutex;
    HandleContainer<ShaderHandle, ShaderState*> m_shaders;

    
    std::unordered_map<unsigned int, std::set<ShaderHandle>> m_fileLookups;
};

}
