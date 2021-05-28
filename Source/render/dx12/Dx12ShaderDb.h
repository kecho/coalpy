#include <coalpy.render/IShaderDb.h>

namespace coalpy
{

class Dx12Compiler;

class Dx12ShaderDb : public IShaderDb
{
public:
    explicit Dx12ShaderDb(const ShaderDbDesc& desc);
    virtual void compileShader(const ShaderDesc& desc, ShaderCompilationResult& result) override;
    virtual ~Dx12ShaderDb();

private:
    ShaderDbDesc m_desc;
    Dx12Compiler& m_compiler;
};

}
