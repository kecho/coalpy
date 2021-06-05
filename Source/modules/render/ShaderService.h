#pragma once

#include <coalpy.render/IShaderService.h>
#include <coalpy.core/HandleContainer.h>
#include <coalpy.tasks/ThreadQueue.h>
#include <shared_mutex>
#include <string>
#include <memory>
#include <thread>

namespace coalpy
{

class IFileSystem;
class ITaskSystem;

class ShaderService : public IShaderService
{
public:
    ShaderService(const ShaderServiceDesc& desc);
    virtual void start() override;
    virtual void stop()  override;

private:
    IShaderDb*   m_db;
};


}
