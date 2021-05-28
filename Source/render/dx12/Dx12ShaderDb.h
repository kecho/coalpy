#include <coalpy.render/IShaderDb.h>

struct IDxcCompiler3;

namespace coalpy
{

class Dx12ShaderDb : public IShaderDb
{
public:
    explicit Dx12ShaderDb(const ShaderDbDesc& desc);
    virtual void compileShader(const ShaderDesc& desc, ShaderCompilationResult& result) override;
    virtual ~Dx12ShaderDb();

private:
    void SetupDxc();
    ShaderDbDesc m_desc;
    IDxcCompiler3* m_compiler;
};

}
