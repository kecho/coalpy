#include "ModuleState.h"
#include <coalpy.core/Assert.h>
#include <coalpy.render/IShaderService.h>
#include <coalpy.render/IShaderDb.h>
#include <coalpy.files/IFileSystem.h>
#include <coalpy.tasks/ITaskSystem.h>
#include "CoalpyTypeObject.h"
#include <iostream>

namespace coalpy
{
namespace gpu
{

ModuleState::ModuleState(CoalpyTypeObject** types, int typesCount)
: m_ss(nullptr), m_fs(nullptr), m_ts(nullptr)
{
    {
        TaskSystemDesc desc;
        desc.threadPoolSize = 16;
        m_ts = ITaskSystem::create(desc);
    }

    {
        FileSystemDesc desc;
        desc.taskSystem = m_ts;
        m_fs = IFileSystem::create(desc);
    }

    {
        ShaderDbDesc desc;
        desc.resolveOnDestruction = true; //this is so there is a nice clean destruction
        desc.fs = m_fs;
        desc.ts = m_ts;
        desc.enableLiveEditing = true;
        desc.onErrorFn = [this](ShaderHandle handle, const char* shaderName, const char* shaderErrorStr)
        {
            onShaderCompileError(handle, shaderName, shaderErrorStr);
        };
        m_db = IShaderDb::create(desc);
    }

    {
        ShaderServiceDesc desc;
        desc.db = m_db;
        desc.watchDirectory = ".";
        desc.fileWatchPollingRate = 1000;
        m_ss = IShaderService::create(desc);
    }

    registerTypes(types, typesCount);
}

void ModuleState::registerTypes(CoalpyTypeObject** types, int typesCount)
{
    for (auto& t : m_types)
        t = nullptr;

    for (int i = 0; i < (int)TypeId::Counts; ++i)
    {
        CPY_ASSERT(m_types[i] == nullptr);
        CPY_ASSERT((int)types[i]->typeId == i);
        m_types[i] = types[i];
        types[i]->moduleState = this;
    }

    for (int i = 0; i < (int)TypeId::Counts; ++i)
        CPY_ASSERT_MSG(m_types[i] != nullptr, "Missing type");
}

ModuleState::~ModuleState()
{
    delete m_ss;
    delete m_db;
    delete m_fs;
    delete m_ts;
}

void ModuleState::startServices()
{
    m_ts->start();
    m_ss->start();
}

void ModuleState::stopServices()
{
    m_ss->stop();
    m_ts->signalStop();
    m_ts->join();
}

void ModuleState::onShaderCompileError(ShaderHandle handle, const char* shaderName, const char* shaderErrorString)
{
    std::lock_guard lock(m_shaderErrorMutex);
    std::cerr << "[" << shaderName << "] " << shaderErrorString << std::endl;
}

}
}
