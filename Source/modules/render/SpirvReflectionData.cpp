#include "SpirvReflectionData.h"
#include <coalpy.core/Assert.h>

#include <iostream>

namespace coalpy
{

bool SpirvReflectionData::load(void* spirvCode, int codeSize)
{
    AddRef();
    if (spirvCode == nullptr || codeSize == 0)
        return false;

    SpvReflectResult result;
    result = spvReflectCreateShaderModule(codeSize, spirvCode, &module);
    if (result != SPV_REFLECT_RESULT_SUCCESS)
        return false;

    uint32_t count = 0;
    result = spvReflectEnumerateDescriptorSets(&module, &count, nullptr);
    if (result != SPV_REFLECT_RESULT_SUCCESS)
        return false;

    descriptorSets.resize(count);
    result = spvReflectEnumerateDescriptorSets(&module, &count, descriptorSets.data());
    CPY_ASSERT(result == SPV_REFLECT_RESULT_SUCCESS);

    return true;
}

SpirvReflectionData::~SpirvReflectionData()
{
    if (m_valid)
        spvReflectDestroyShaderModule(&module);
}

void SpirvReflectionData::AddRef()
{
    ++m_refCount;
}

void SpirvReflectionData::Release()
{
    --m_refCount;
    if (m_refCount == 0)
        delete this;
}


}
