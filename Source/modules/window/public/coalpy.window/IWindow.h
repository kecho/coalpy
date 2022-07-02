#pragma once

#include <coalpy.window/WindowDefs.h>

namespace coalpy
{

class WindowInputState;

class IWindow
{
public:
    static IWindow* create(const WindowDesc& desc);
    static void run(const WindowRunArgs& runArgs);
    virtual WindowOsHandle getHandle() const = 0;
    virtual void open() = 0;
    virtual void dimensions(int& w, int& h) const = 0;
    virtual bool isClosed() = 0;
    virtual bool shouldRender() { return !isClosed(); }
    virtual const WindowInputState& inputState() const = 0;
    virtual ~IWindow() {}

    inline void setUserData(void* userData) { m_userData = userData; }
    inline void* userData() const { return m_userData; }

private:
    void* m_userData = nullptr;
};

}
