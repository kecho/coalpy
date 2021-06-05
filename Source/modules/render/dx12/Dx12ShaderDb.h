#include <coalpy.core/GenericHandle.h>
#include <coalpy.core/HandleContainer.h>
#include <coalpy.render/IShaderDb.h>
#include <shared_mutex>
#include "Dx12Compiler.h"

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
    ShaderDbDesc m_desc;
    Dx12Compiler m_compiler;

    struct ShaderState
    {
        bool ready;
        bool success;
        Dx12CompileState* compileState;
    };

    mutable std::shared_mutex m_shadersMutex;
    HandleContainer<ShaderHandle, ShaderState> m_shaders;
};

}
