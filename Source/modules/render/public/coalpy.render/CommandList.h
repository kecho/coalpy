#pragma once

#include <coalpy.render/Resources.h>
#include <coalpy.shader/ShaderDefs.h>

namespace coalpy
{
namespace render
{

struct ComputeCommand
{
    friend class CommandList;
    inline void setConstants(Buffer constBuffer) { setConstants(&constBuffer, 1); }
    inline void setInResources(InResourceTable inTables) { setInResources(&inTables, 1); }
    inline void setOutResources(OutResourceTable outTables) { setOutResources(&outTables, 1); }
    void setShader(ShaderHandle shader);
    void setConstants(Buffer* constBuffers, int bufferCounts);
    void setInResources(InResourceTable* inTables, int inputTablesCount);
    void setOutResources(OutResourceTable* outTables, int inputTablesCount);
    void setDispatch(const char* debugNameMarker, int x, int y, int z);

private:
    struct InternalComputeCmd* m_data;
};

struct CopyCommand
{
    friend class CommandList;
    void setResources(ResourceHandle source, ResourceHandle destination);

private:
    struct InternalCopyCommand* m_data;
};

struct UploadCommand
{
    friend class CommandList;
    void setData(const void* source, ResourceHandle destination);

private:
    struct InternalUploadCommand* m_data;
};

class CommandList
{
public:
    CommandList();
    ~CommandList();
    ComputeCommand addComputeCommand();
    CopyCommand    addCopyCommand();
    UploadCommand  addUploadCommand();

private:
    struct InternalCmdList* m_cmdList;
};


}
}
