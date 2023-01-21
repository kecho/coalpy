#pragma once

#include <coalpy.render/IImguiRenderer.h>
#include <coalpy.render/Resources.h>
#include <imgui.h>
#include <implot.h>

namespace coalpy
{
namespace render
{

class BaseImguiRenderer : public IimguiRenderer
{
public:
    BaseImguiRenderer(const IimguiRendererDesc& desc);
    virtual ~BaseImguiRenderer();

    virtual void newFrame() override;
    virtual void activate() override;
    virtual void render() override;
    virtual void endFrame() override;
    virtual ImTextureID registerTexture(Texture texture) override;
    virtual void unregisterTexture(Texture texture) override;
    virtual bool isTextureRegistered(Texture texture) const override;

protected:
    void setCoalpyStyle();
    IimguiRendererDesc m_desc;
    ImGuiContext* m_context;
    ImPlotContext* m_plotContext;
    int m_windowHookId;
};


}
}
