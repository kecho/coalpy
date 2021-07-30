#pragma once 

#include <coalpy.render/Resources.h>
#include <coalpy.core/Os.h>
#include <coalpy.core/RefCounted.h>
#include <coalpy.core/Formats.h>

namespace coalpy
{
namespace render
{

struct DisplayConfig
{
    WindowOsHandle handle = {};
    Format format = Format::RGBA_8_UNORM;
    unsigned int buffering = 2u;
    unsigned int width = 128u;
    unsigned int height = 128u;
};

class IDisplay : public RefCounted
{
public:
    virtual ~IDisplay() {}

    // New width, new height.
    virtual void resize(unsigned int width, unsigned int height) = 0;
    virtual Texture texture() = 0;
    virtual void present() = 0;

    // Gets the config of this display
    const DisplayConfig& config() const { return m_config; }

    void setupSwapChain();

protected:
    IDisplay(const DisplayConfig& config)
    : m_config(config) {}

    DisplayConfig m_config;
};

}
}
