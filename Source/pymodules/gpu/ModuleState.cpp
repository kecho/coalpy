#include <coalpy.render/IShaderService.h>
#include <coalpy.render/IShaderDb.h>
#include <coalpy.files/IFileSystem.h>
#include <coalpy.tasks/ITaskSystem.h>
#include "ModuleState.h"

namespace coalpy
{
namespace gpu
{


ModuleState::ModuleState()
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
        desc.fs = m_fs;
        desc.ts = m_ts;
        m_db = IShaderDb::create(desc);
    }

    {
        ShaderServiceDesc desc;
        desc.db = m_db;
        desc.watchDirectory = ".";
        desc.fileWatchPollingRate = 1000;
        m_ss = IShaderService::create(desc);
    }
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

}
}
