#pragma once

#include <coalpy.render/IimguiRenderer.h>
#include <coalpy.render/Resources.h>
#include <imgui.h>
#include <implot.h>

namespace coalpy
{
namespace render
{


struct ImguiContexts
{
    ImGuiContext* imgui;
    ImPlotContext* implot;
};

class BaseImguiRenderer : public IimguiRenderer
{
public:
    BaseImguiRenderer(const IimguiRendererDesc& desc);
    virtual ~BaseImguiRenderer();

    virtual void newFrame() override;
    virtual void activate() override;
    virtual void render() override;

protected:
    void setCoalpyStyle();
    IimguiRendererDesc m_desc;
    ImguiContexts m_contexts;
    int m_windowHookId;
};


}
}
