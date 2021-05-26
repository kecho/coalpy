#include <coalpy.render/IShaderDb.h>

namespace coalpy
{

class Dx12ShaderDb : public IShaderDb
{
public:
    virtual void compileShader(const ShaderDesc& desc, ShaderCompilationResult& result) override;
    virtual ~Dx12ShaderDb(){}
};

}
