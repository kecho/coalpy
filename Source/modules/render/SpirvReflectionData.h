#pragma once

#include <spirv_reflect.h>
#include <string>

namespace coalpy
{

class SpirvReflectionData
{
public:
    SpirvReflectionData()
        : m_refCount(0), m_valid(false), module({})
    {
    }

    ~SpirvReflectionData();

    bool load(void* spirvCode, int size);
    void AddRef();
    void Release();

    std::string mainFn;
    SpvReflectShaderModule module;
    std::vector<SpvReflectDescriptorSet*> descriptorSets;

private:
    bool m_valid;
    int m_refCount;
};

}
