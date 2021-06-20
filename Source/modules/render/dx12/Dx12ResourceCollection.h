#pragma once

#include <coalpy.core/HandleContainer.h>

namespace coalpy
{
namespace render
{

class Dx12Device;

class Dx12ResourceCollection
{
public:
    Dx12ResourceCollection(Dx12Device& device);
    ~Dx12ResourceCollection();

private:
    struct ResourceContainer
    {
    };

    Dx12Device& m_device;
};

}
}
