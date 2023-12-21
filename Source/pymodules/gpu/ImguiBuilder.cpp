#include "ImguiBuilder.h"
#include "HelperMacros.h"
#include "PyUtils.h"
#include "TypeIds.h"
#include "CoalpyTypeObject.h"
#include "Resources.h"
#include <functional>
#include <imgui_internal.h> //for docking API 
#include <misc/cpp/imgui_stdlib.h>

namespace coalpy
{

namespace gpu
{

namespace methods
{
    #include "bindings/MethodDecl.h"
    #include "bindings/Imgui.inl"
}

static PyMethodDef g_imguiMethods[] = {
    #include "bindings/MethodDef.h"
    #include "bindings/Imgui.inl"
    FN_END
};

void ImguiBuilder::constructType(CoalpyTypeObject& o)
{
    auto& t = o.pyObj;
    t.tp_name = "gpu.ImguiBuilder";
    t.tp_basicsize = sizeof(ImguiBuilder);
    t.tp_doc   = R"(
    Imgui builder class. Use this class' methods to build a Dear ImGUI on a specified window.
    To utilize you need to instantiate a Window object. In its constructor set use_imgui to True (which is by default).
    On the on_render function, you will receive a RenderArgs object. There will be a ImguiBuilder object in the imgui member that you can now
    query to construct your user interface.
    Coalpy does not support creation of a ImguiBuilder object, it will always be passed as an argument on the RenderArgs of the window.
    )";
    t.tp_flags = Py_TPFLAGS_DEFAULT;
    t.tp_new = PyType_GenericNew;
    t.tp_init = ImguiBuilder::init;
    t.tp_dealloc = ImguiBuilder::destroy;
    t.tp_methods = g_imguiMethods;
}

int ImguiBuilder::init(PyObject* self, PyObject * vargs, PyObject* kwds)
{
    auto* obj = (ImguiBuilder*)self;
    new (obj) ImguiBuilder;
    return 0;
}

void ImguiBuilder::destroy(PyObject* self)
{
    ((ImguiBuilder*)self)->~ImguiBuilder();
    Py_TYPE(self)->tp_free(self);
}

ImTextureID ImguiBuilder::getTextureID(Texture* texture)
{
    if (activeRenderer == nullptr)
        return nullptr;

    CPY_ASSERT(texture->texture.valid());

    auto it = textureReferences.insert(texture);
    if (it.second)
        Py_INCREF(texture);

    texture->hasImguiRef = true;

    //unregistration occurs in ModuleState's texture's destructor.
    //the renderer has an internal hashmap that keeps track of the texture's allocated id.
    return activeRenderer->registerTexture(texture->texture);
}

void ImguiBuilder::clearTextureReferences()
{
    for (auto t : textureReferences)
        Py_DECREF(t);

    textureReferences.clear();
}


/*** Imgui API Wrappers***/
namespace methods
{

bool CheckImgui(PyObject* builder)
{
    auto& imguiBuilder = *(ImguiBuilder*)builder;
    if (imguiBuilder.enabled)
        return true;

    ModuleState& moduleState = parentModule(builder);
    PyErr_SetString(moduleState.exObj(), "Cannot use the imgui object outside of a on_render callback. Ensure you create a window, and use the imgui object provided from the renderArgs.");
    return false;
}

#define CHECK_IMGUI \
    if (!CheckImgui(self))\
        return nullptr;

PyObject* begin(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI
    char* name = nullptr;
    int is_openint = -1;
    int is_fullscreenint = 0;
    int flags = 0;
    static char* argnames[] = { "name", "is_open", "is_fullscreen", "flags", nullptr };
    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "s|ppi", argnames, &name, &is_openint, &is_fullscreenint, &flags))
        return nullptr;

    bool is_open = is_openint ? true : false;

    if (is_fullscreenint)
    {
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        flags |= ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;
    }

    ImGui::Begin(name, is_openint == -1 ? nullptr : &is_open, flags);
    if (is_open)
        Py_RETURN_TRUE;
    else
        Py_RETURN_FALSE;
}

PyObject* end(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI
    ImGui::End();
    Py_RETURN_NONE;
}

PyObject* showDemoWindowImgui(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI
    ImGui::ShowDemoWindow();
    Py_RETURN_NONE;
}

PyObject* sliderFloat(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI
    char* label;
    float v, v_min, v_max;
    char* fmt = "%.3f";
    static char* argnames[] = { "label", "v", "v_min", "v_max", "fmt", nullptr };
    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "sfff|s", argnames, &label, &v, &v_min, &v_max, &fmt))
        return nullptr;
    
    ImGui::SliderFloat(label, &v, v_min, v_max, fmt);
    return PyFloat_FromDouble((double)v);
}

PyObject* inputFloat(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI
    char* label;
    float v;
    float step = 0.0f, step_fast = 0.0f;
    char* fmt = "%.3f";
    static char* argnames[] = { "label", "v", "step", "step_fast", "fmt", nullptr };
    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "sf|ffs", argnames, &label, &v, &step, &step_fast, &fmt))
        return nullptr;
    
    ImGui::InputFloat(label, &v, step, step_fast, fmt);
    return PyFloat_FromDouble((double)v);
}

PyObject* inputFloat2(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    ModuleState& moduleState = parentModule(self);

    CHECK_IMGUI
    char* label;
    PyObject* v;
    char* fmt = "%.3f";
    static char* argnames[] = { "label", "v", "fmt", nullptr };
    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "sO|s", argnames, &label, &v, &fmt))
        return nullptr;

    float vecVal[2];
    if (!getTupleValuesFloat(v, vecVal, 2, 2)) 
    {
        PyErr_SetString(moduleState.exObj(), "Cannot parse v value. Must be a tuple or list of length 2");
        return nullptr;
    }
    
    ImGui::InputFloat2(label, vecVal, fmt);
    return Py_BuildValue("(ff)", vecVal[0], vecVal[1]);
}

PyObject* inputFloat3(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    ModuleState& moduleState = parentModule(self);

    CHECK_IMGUI
    char* label;
    PyObject* v;
    char* fmt = "%.3f";
    static char* argnames[] = { "label", "v", "fmt", nullptr };
    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "sO|s", argnames, &label, &v, &fmt))
        return nullptr;

    float vecVal[3];
    if (!getTupleValuesFloat(v, vecVal, 3, 3)) 
    {
        PyErr_SetString(moduleState.exObj(), "Cannot parse v value. Must be a tuple or list of length 3");
        return nullptr;
    }
    
    ImGui::InputFloat3(label, vecVal, fmt);
    return Py_BuildValue("(fff)", vecVal[0], vecVal[1], vecVal[2]);
}

PyObject* inputFloat4(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    ModuleState& moduleState = parentModule(self);

    CHECK_IMGUI
    char* label;
    PyObject* v;
    char* fmt = "%.3f";
    static char* argnames[] = { "label", "v", "fmt", nullptr };
    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "sO|s", argnames, &label, &v, &fmt))
        return nullptr;

    float vecVal[4];
    if (!getTupleValuesFloat(v, vecVal, 4, 4)) 
    {
        PyErr_SetString(moduleState.exObj(), "Cannot parse v value. Must be a tuple or list of length 4");
        return nullptr;
    }
    
    ImGui::InputFloat4(label, vecVal, fmt);
    return Py_BuildValue("(ffff)", vecVal[0], vecVal[1], vecVal[2], vecVal[3]);
}

PyObject* inputText(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI
    char* label;
    char* inputStr;
    static char* argnames[] = { "label", "v", nullptr };
    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "ss", argnames, &label, &inputStr))
        return nullptr;

    std::string str = inputStr;
    ImGui::InputText(label, &str);
    return Py_BuildValue("s", str.c_str());
}

PyObject* text(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI
    char* text;
    static char* argnames[] = { "text", nullptr };
    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "s", argnames, &text))
        return nullptr;

    ImGui::Text("%s", text);
    Py_RETURN_NONE;
}

PyObject* pushId(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI
    char* text;
    static char* argnames[] = { "text", nullptr };
    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "s", argnames, &text))
        return nullptr;

    ImGui::PushID("%s");
    Py_RETURN_NONE;
}

PyObject* popId(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI
    char* text;
    ImGui::PopID();
    Py_RETURN_NONE;
}

PyObject* collapsingHeader(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI
    char* label;
    int flags = 0;
    static char* argnames[] = { "label", "flags", nullptr };
    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "s|i", argnames, &label, &flags))
        return nullptr;

    bool ret = ImGui::CollapsingHeader(label, flags);
    if (ret)
        Py_RETURN_TRUE;
    else
        Py_RETURN_FALSE;
}

PyObject* beginMainMenuBar(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI
    if (ImGui::BeginMainMenuBar())
    {
        Py_RETURN_TRUE;
    }
    else
    {
        Py_RETURN_FALSE;
    }
}

PyObject* endMainMenuBar(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI
    ImGui::EndMainMenuBar();
    Py_RETURN_NONE;
}

PyObject* beginMenu(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI
    char* label;
    static char* argnames[] = { "label", nullptr };
    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "s", argnames, &label))
        return nullptr;

    bool ret = ImGui::BeginMenu(label);
    if (ret)
        Py_RETURN_TRUE;
    else
        Py_RETURN_FALSE;
}

PyObject* menuItem(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI
    char* label;
    char* shortcut = nullptr;
    int enabledint = 1;
    static char* argnames[] = { "label", "shortcut", "enabled", nullptr };
    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "s|sp", argnames, &label, &shortcut, &enabledint))
        return nullptr;

    bool ret = ImGui::MenuItem(label, shortcut, false, (bool)enabledint);
    if (ret)
        Py_RETURN_TRUE;
    else
        Py_RETURN_FALSE;
}

PyObject* endMenu(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI
    ImGui::EndMenu();
    Py_RETURN_NONE;
}

PyObject* button(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI
    char* label;
    static char* argnames[] = { "label", nullptr };
    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "s", argnames, &label))
        return nullptr;

    bool ret = ImGui::Button(label);
    if (ret)
        Py_RETURN_TRUE;
    else
        Py_RETURN_FALSE;
}

PyObject* checkbox(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI
    char* label;
    int vint;
    static char* argnames[] = { "label", "v", nullptr };
    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "sp", argnames, &label, &vint))
        return nullptr;

    bool v = (bool)vint;
    bool ret = ImGui::Checkbox(label, &v);
    if (v)
        Py_RETURN_TRUE;
    else
        Py_RETURN_FALSE;
}

PyObject* beginCombo(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI
    char* label;
    char* previewValue;
    static char* argnames[] = { "label", "preview_value", nullptr };
    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "ss", argnames, &label, &previewValue))
        return nullptr;

    bool ret = ImGui::BeginCombo(label, previewValue);
    if (ret)
        Py_RETURN_TRUE;
    else
        Py_RETURN_FALSE;
}

PyObject* endCombo(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI
    ImGui::EndCombo();
    Py_RETURN_NONE;
}

PyObject* selectable(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI
    char* label;
    int selectedint;
    static char* argnames[] = { "label", "selected", nullptr };
    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "sp", argnames, &label, &selectedint))
        return nullptr;

    bool ret = ImGui::Selectable(label, (bool)selectedint);
    if (ret)
        Py_RETURN_TRUE;
    else
        Py_RETURN_FALSE;
}

PyObject* sameLine(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI
    float offsetFromX = 0.0f;
    float spacing = -1.0f;
    static char* argnames[] = { "offset_from_start_x", "spacing", nullptr };
    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "|ff", argnames, &offsetFromX, &spacing))
        return nullptr;

    ImGui::SameLine(offsetFromX, spacing);
    Py_RETURN_NONE;
}

PyObject* separator(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI
    ImGui::Separator();
    Py_RETURN_NONE;
}

PyObject* newLine(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI
    ImGui::NewLine();
    Py_RETURN_NONE;
}

PyObject* image(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI
    auto& imguiBuilder = *(ImguiBuilder*)self;
    ModuleState& moduleState = parentModule(self);
    static char* argnames[] = { "texture", "size", "uv0", "uv1", "tint_col", "border_col", nullptr };
    PyObject* textureObj = nullptr;
    ImVec2 size(0.0f,0.0f);
    ImVec2 uv0(0.0f, 0.0f);
    ImVec2 uv1(1.0f, 1.0f);
    ImVec4 tintCol(1.0f, 1.0f, 1.0f, 1.0f);
    ImVec4 borderCol(0.0f, 0.0f, 0.0f, 0.0f);

    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "O(ff)|(ff)(ff)(ffff)(ffff)", argnames, 
        &textureObj, &size.x, &size.y, &uv0.x, &uv0.y, &uv1.x, &uv1.y,
        &tintCol.x, &tintCol.y, &tintCol.z, &tintCol.w,
        &borderCol.x, &borderCol.y, &borderCol.z, &borderCol.w))
        return nullptr;


    PyTypeObject* textureType = moduleState.getType(Texture::s_typeId);
    if (textureObj->ob_type != textureType)
    {
        PyErr_SetString(moduleState.exObj(), "texture must be a gpu.Texture object.");
        return nullptr;
    }

    Texture* texture = (Texture*)textureObj;
    ImTextureID textureID = imguiBuilder.getTextureID(texture);
    if (textureID == nullptr)
    {
        PyErr_SetString(moduleState.exObj(), "Internal failure: cannot convert a gpu.Texture to internal ImTextureID.");
        return nullptr;
    }

    ImGui::Image(textureID, size, uv0, uv1, tintCol, borderCol);

    Py_RETURN_NONE;
}

PyObject* imageButton(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI
    auto& imguiBuilder = *(ImguiBuilder*)self;
    ModuleState& moduleState = parentModule(self);
    static char* argnames[] = { "texture", "size", "uv0", "uv1", "frame_padding", "bg_col", "tint_col", nullptr };
    PyObject* textureObj = nullptr;
    ImVec2 size(0.0f,0.0f);
    ImVec2 uv0(0.0f, 0.0f);
    ImVec2 uv1(1.0f, 1.0f);
    int framePadding = -1;
    ImVec4 bgCol(0.0f, 0.0f, 0.0f, 0.0f);
    ImVec4 tintCol(1.0f, 1.0f, 1.0f, 1.0f);

    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "O(ff)|(ff)(ff)i(ffff)(ffff)", argnames, 
        &textureObj, &size.x, &size.y, &uv0.x, &uv0.y, &uv1.x, &uv1.y,
        &framePadding, 
        &bgCol.x, &bgCol.y, &bgCol.z, &bgCol.w,
        &tintCol.x, &tintCol.y, &tintCol.z, &tintCol.w))
        return nullptr;


    PyTypeObject* textureType = moduleState.getType(Texture::s_typeId);
    if (textureObj->ob_type != textureType)
    {
        PyErr_SetString(moduleState.exObj(), "texture must be a gpu.Texture object.");
        return nullptr;
    }

    Texture* texture = (Texture*)textureObj;
    ImTextureID textureID = imguiBuilder.getTextureID(texture);
    if (textureID == nullptr)
    {
        PyErr_SetString(moduleState.exObj(), "Internal failure: cannot convert a gpu.Texture to internal ImTextureID.");
        return nullptr;
    }

    if (ImGui::ImageButton(textureID, size, uv0, uv1, framePadding, bgCol, tintCol))
        Py_RETURN_TRUE;
    else
        Py_RETURN_FALSE;
}

PyObject* getItemRectSize(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI
    ImVec2 sz = ImGui::GetItemRectSize();
    return Py_BuildValue("(ff)", sz.x, sz.y);
}

PyObject* setTooltip(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI
    auto& imguiBuilder = *(ImguiBuilder*)self;
    ModuleState& moduleState = parentModule(self);
    static char* argnames[] = { "text",  nullptr };
    char* text;

    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "s", argnames, &text))
        return nullptr;

    ImGui::SetTooltip("%s", text);
    Py_RETURN_NONE;
}

PyObject* isItemHovered(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI
    auto& imguiBuilder = *(ImguiBuilder*)self;
    ModuleState& moduleState = parentModule(self);
    static char* argnames[] = { "flags",  nullptr };
    int flags = 0;

    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "|i", argnames, &flags))
        return nullptr;

    if (ImGui::IsItemHovered(flags))
        Py_RETURN_TRUE;
    else
        Py_RETURN_FALSE;
}


PyObject* getMousePos (PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI;
    ImVec2 mousePos = ImGui::GetMousePos();
    return Py_BuildValue("(ff)", mousePos.x, mousePos.y);
}

PyObject* getCursorPos (PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI;
    ImVec2 cursorPos = ImGui::GetCursorPos();
    return Py_BuildValue("(ff)", cursorPos.x, cursorPos.y);
}

PyObject* getCursorScreenPos (PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI;
    ImVec2 cursorPos = ImGui::GetCursorScreenPos();
    return Py_BuildValue("(ff)", cursorPos.x, cursorPos.y);
}

PyObject* isMouseDown (PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI;
    auto& imguiBuilder = *(ImguiBuilder*)self;
    ModuleState& moduleState = parentModule(self);
    static char* argnames[] = { "key",  nullptr };
    int key = 0;

    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "i", argnames, &key))
        return nullptr;

    if (ImGui::IsMouseDown(key))
        Py_RETURN_TRUE;
    else
        Py_RETURN_FALSE;
}

PyObject* isMouseClicked (PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI;
    auto& imguiBuilder = *(ImguiBuilder*)self;
    ModuleState& moduleState = parentModule(self);
    static char* argnames[] = { "key",  nullptr };
    int key = 0;

    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "i", argnames, &key))
        return nullptr;

    if (ImGui::IsMouseClicked(key))
        Py_RETURN_TRUE;
    else
        Py_RETURN_FALSE;
}

PyObject* isMouseReleased (PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI;
    auto& imguiBuilder = *(ImguiBuilder*)self;
    ModuleState& moduleState = parentModule(self);
    static char* argnames[] = { "key",  nullptr };
    int key = 0;

    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "i", argnames, &key))
        return nullptr;

    if (ImGui::IsMouseReleased(key))
        Py_RETURN_TRUE;
    else
        Py_RETURN_FALSE;
}

PyObject* isMouseDoubleClicked (PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI;
    auto& imguiBuilder = *(ImguiBuilder*)self;
    ModuleState& moduleState = parentModule(self);
    static char* argnames[] = { "key",  nullptr };
    int key = 0;

    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "i", argnames, &key))
        return nullptr;

    if (ImGui::IsMouseDoubleClicked(key))
        Py_RETURN_TRUE;
    else
        Py_RETURN_FALSE;
}


PyObject* getID(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI;
    ModuleState& moduleState = parentModule(self);
    static char* argnames[] = { "name", nullptr };

    char* name;
    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "s", argnames, &name))
        return nullptr;

    auto id = ImGui::GetID(name);
    return Py_BuildValue("i", id);
}

PyObject* settingsLoaded(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI;
    ImGuiContext& g = *GImGui;
    if (g.SettingsLoaded && !g.SettingsIniData.empty())
        Py_RETURN_TRUE;
    else
        Py_RETURN_FALSE;
}

PyObject* getWindowSize(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI;
    ImVec2 winSize = ImGui::GetWindowSize();
    return Py_BuildValue("(ff)", winSize.x, winSize.y);
}

PyObject* getViewportWorkSize(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI;
    auto* vp = ImGui::GetWindowViewport();
    if (vp == nullptr)
        return Py_BuildValue("(ff)", 0.0f, 0.0f);
    ImVec2 winSize = vp->WorkSize;
    return Py_BuildValue("(ff)", winSize.x, winSize.y);
}

PyObject* getViewportWorkPos(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI;
    auto* vp = ImGui::GetWindowViewport();
    if (vp == nullptr)
        return Py_BuildValue("(ff)", 0.0f, 0.0f);
    ImVec2 pos = vp->WorkPos;
    return Py_BuildValue("(ff)", pos.x, pos.y);
}

PyObject* getWindowContentRegionMin(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI;
    ImVec2 crMin = ImGui::GetWindowContentRegionMin();
    return Py_BuildValue("(ff)", crMin.x, crMin.y);
}

PyObject* getWindowContentRegionMax(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI;
    ImVec2 crMax = ImGui::GetWindowContentRegionMax();
    return Py_BuildValue("(ff)", crMax.x, crMax.y);
}

PyObject* getWindowPos(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI;
    ImVec2 winPos = ImGui::GetWindowPos();
    return Py_BuildValue("(ff)", winPos.x, winPos.y);
}

PyObject* setWindowSize(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI;
    ModuleState& moduleState = parentModule(self);
    static char* argnames[] = { "size", nullptr };

    ImVec2 sz = {};
    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "(ff)", argnames, &sz.x, &sz.y))
        return nullptr;

    ImGui::SetWindowSize(sz);
    Py_RETURN_NONE;
}

PyObject* isWindowFocused(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI;
    ModuleState& moduleState = parentModule(self);
    static char* argnames[] = { "flags", nullptr };

    int flags = 0;
    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "|i", argnames, &flags))
        return nullptr;

    if (ImGui::IsWindowFocused(flags))
        Py_RETURN_TRUE;
    else
        Py_RETURN_FALSE;
}

PyObject* isWindowHovered(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI;
    ModuleState& moduleState = parentModule(self);
    static char* argnames[] = { "flags", nullptr };

    int flags = 0;
    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "|i", argnames, &flags))
        return nullptr;

    if (ImGui::IsWindowHovered(flags))
        Py_RETURN_TRUE;
    else
        Py_RETURN_FALSE;
}

PyObject* setWindowFocus(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI;
    ImGui::SetWindowFocus();
    Py_RETURN_NONE;
}

PyObject* isKeyDown(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI;
    auto& imguiBuilder = *(ImguiBuilder*)self;
    ModuleState& moduleState = parentModule(self);
    static char* argnames[] = { "key", nullptr };

    int key;
    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "i", argnames, &key))
        return nullptr;

    if (ImGui::IsKeyDown((ImGuiKey)key))
        Py_RETURN_TRUE;
    else
        Py_RETURN_FALSE;
}

PyObject* isKeyPressed(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI;
    auto& imguiBuilder = *(ImguiBuilder*)self;
    ModuleState& moduleState = parentModule(self);
    static char* argnames[] = { "key", "repeat", nullptr };

    int key;
    int repeatInt = 1;
    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "i|p", argnames, &key, &repeatInt))
        return nullptr;

    if (ImGui::IsKeyPressed((ImGuiKey)key, (bool)repeatInt))
        Py_RETURN_TRUE;
    else
        Py_RETURN_FALSE;
}

PyObject* isKeyReleased(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI;
    auto& imguiBuilder = *(ImguiBuilder*)self;
    ModuleState& moduleState = parentModule(self);
    static char* argnames[] = { "key", nullptr };

    int key;
    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "i", argnames, &key))
        return nullptr;

    if (ImGui::IsKeyReleased((ImGuiKey)key))
        Py_RETURN_TRUE;
    else
        Py_RETURN_FALSE;
}

PyObject* getKeyPressedAmount(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI;
    auto& imguiBuilder = *(ImguiBuilder*)self;
    ModuleState& moduleState = parentModule(self);
    static char* argnames[] = { "key", "repeat_delay", "rate", nullptr };

    int key;
    float repeat_delay;
    float rate;
    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "iff", argnames, &key, &repeat_delay, &rate))
        return nullptr;

    int retVal = ImGui::GetKeyPressedAmount((ImGuiKey)key, repeat_delay, rate);
    return Py_BuildValue("i", retVal);
}

PyObject* getKeyName(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI;
    auto& imguiBuilder = *(ImguiBuilder*)self;
    ModuleState& moduleState = parentModule(self);
    static char* argnames[] = { "key", nullptr };

    int key;
    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "i", argnames, &key))
        return nullptr;

    const char* keyName = ImGui::GetKeyName((ImGuiKey)key);
    return Py_BuildValue("s",keyName);
}

PyObject* setNextFrameWantCaptureKeyboard(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI;
    auto& imguiBuilder = *(ImguiBuilder*)self;
    ModuleState& moduleState = parentModule(self);
    static char* argnames[] = { "want_capture_keyboard", nullptr };

    int want_capture_keyboard;
    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "i", argnames, &want_capture_keyboard))
        return nullptr;

    ImGui::SetNextFrameWantCaptureKeyboard(want_capture_keyboard);
    Py_RETURN_NONE;
}

PyObject* setWindowDock(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI
    auto& imguiBuilder = *(ImguiBuilder*)self;
    ModuleState& moduleState = parentModule(self);
    static char* argnames[] = { "window_name", "dock_id", nullptr };

    const char* window_name = nullptr;
    int dock_id = -1;
    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "si", argnames, &window_name, &dock_id))
        return nullptr;

    ImGuiWindow* w = ImGui::FindWindowByName(window_name);
    if (w != nullptr)
        ImGui::SetWindowDock(w, dock_id, 0);
    Py_RETURN_NONE;
}

PyObject* dockspace(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI
    auto& imguiBuilder = *(ImguiBuilder*)self;
    ModuleState& moduleState = parentModule(self);
    static char* argnames[] = { "label", "dock_id", nullptr };

    char* label = nullptr; 
    int dock_id = -1;
    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "|si", argnames, &label, &dock_id))
        return nullptr;

    ImGuiID dockspaceId = dock_id == -1 ? ImGui::GetID(label ? label : "default_dock") : dock_id;
    dockspaceId = ImGui::DockSpace(dockspaceId);
    return Py_BuildValue("i", dockspaceId);
}


PyObject* dockBuilderDockWindow(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI
    auto& imguiBuilder = *(ImguiBuilder*)self;
    ModuleState& moduleState = parentModule(self);
    static char* argnames[] = { "window_name", "node_id", nullptr };

    char* window_name = nullptr; 
    int node_id = -1;
    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "si", argnames, &window_name, &node_id))
        return nullptr;

    ImGui::DockBuilderDockWindow(window_name, node_id);
    Py_RETURN_NONE;
}

PyObject* dockBuilderAddNode(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI
    auto& imguiBuilder = *(ImguiBuilder*)self;
    ModuleState& moduleState = parentModule(self);
    static char* argnames[] = { "node_id", "flags", nullptr };

    int node_id = -1;
    int flags = 0;
    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "i|i", argnames, &node_id, &flags))
        return nullptr;

    ImGuiID id = ImGui::DockBuilderAddNode(node_id, flags);
    return Py_BuildValue("i", id);
}

PyObject* dockBuilderRemoveNode(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI
    auto& imguiBuilder = *(ImguiBuilder*)self;
    ModuleState& moduleState = parentModule(self);
    static char* argnames[] = { "node_id", nullptr };
    int node_id = -1;
    int flags = 0;
    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "i", argnames, &node_id))
        return nullptr;

    ImGui::DockBuilderRemoveNode(node_id);
    Py_RETURN_NONE;
}

PyObject* dockBuilderRemoveChildNodes(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI
    auto& imguiBuilder = *(ImguiBuilder*)self;
    ModuleState& moduleState = parentModule(self);
    static char* argnames[] = { "node_id", nullptr };
    int node_id = -1;
    int flags = 0;
    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "i", argnames, &node_id))
        return nullptr;

    ImGui::DockBuilderRemoveNodeChildNodes(node_id);
    Py_RETURN_NONE;
}

PyObject* dockBuilderSetNodePos(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI
    auto& imguiBuilder = *(ImguiBuilder*)self;
    ModuleState& moduleState = parentModule(self);
    static char* argnames[] = { "node_id", "pos", nullptr };
    int node_id = -1;
    ImVec2 pos = {};
    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "i(ff)", argnames, &node_id, &pos.x, &pos.y))
        return nullptr;

    ImGui::DockBuilderSetNodePos(node_id, pos);
    Py_RETURN_NONE;
}

PyObject* dockBuilderSetNodeSize(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI
    auto& imguiBuilder = *(ImguiBuilder*)self;
    ModuleState& moduleState = parentModule(self);
    static char* argnames[] = { "node_id", "size", nullptr };
    int node_id = -1;
    ImVec2 sz = {};
    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "i(ff)", argnames, &node_id, &sz.x, &sz.y))
        return nullptr;

    ImGui::DockBuilderSetNodeSize(node_id, sz);
    Py_RETURN_NONE;
}

PyObject* dockBuilderSplitNode(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI
    auto& imguiBuilder = *(ImguiBuilder*)self;
    ModuleState& moduleState = parentModule(self);
    static char* argnames[] = { "node_id", "split_dir", "split_ratio", nullptr };
    int node_id = -1;
    int split_dir = 0;
    float split_ratio;

    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "iif", argnames, &node_id, &split_dir, &split_ratio))
        return nullptr;
    
    ImGuiID id_at_dir, id_at_op_dir;
    ImGuiID ret = ImGui::DockBuilderSplitNode(node_id, split_dir, split_ratio, &id_at_dir, &id_at_op_dir);
    return Py_BuildValue("(iii)", ret, id_at_dir, id_at_op_dir);
}

PyObject* dockBuilderFinish(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI
    ModuleState& moduleState = parentModule(self);
    static char* argnames[] = { "node_id", nullptr };
    int node_id = -1;
    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "i", argnames, &node_id))
        return nullptr;

    ImGui::DockBuilderFinish(node_id);
    Py_RETURN_NONE;
}

PyObject* dockBuilderNodeExists(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI
    auto& imguiBuilder = *(ImguiBuilder*)self;
    ModuleState& moduleState = parentModule(self);
    static char* argnames[] = { "label", "dock_id", nullptr };

    char* label = nullptr; 
    int dock_id = -1;
    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "|si", argnames, &label, &dock_id))
        return nullptr;

    if (dock_id == -1 && label == nullptr)
        Py_RETURN_FALSE;

    ImGuiID dockspaceId = dock_id == -1 ? ImGui::GetID(label) : dock_id;
    if (ImGui::DockBuilderGetNode(dockspaceId) == nullptr)
        Py_RETURN_FALSE;
    else
        Py_RETURN_TRUE;
}

PyObject* beginTabBar(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI
    auto& imguiBuilder = *(ImguiBuilder*)self;
    ModuleState& moduleState = parentModule(self);
    static char* argnames[] = { "id", "flags", nullptr };
    char* id = nullptr;
    int flags = 0;
    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "s|i", argnames, &id, &flags))
        return nullptr;

    if (ImGui::BeginTabBar(id, flags))
        Py_RETURN_TRUE;
    else
        Py_RETURN_FALSE;
}

PyObject* endTabBar(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI
    ImGui::EndTabBar();
    Py_RETURN_NONE;
}

PyObject* beginTabItem(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI
    auto& imguiBuilder = *(ImguiBuilder*)self;
    ModuleState& moduleState = parentModule(self);
    static char* argnames[] = { "label", "open", "flags", nullptr };
    char* label = nullptr;
    int openInt = -1;
    int flags = 0;
    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "s|qi", argnames, &label, &openInt, &flags))
        return nullptr;

    bool openValue = openInt == 1;
    if (ImGui::BeginTabItem(label, openInt == -1 ? nullptr : &openValue, flags))
        Py_RETURN_TRUE; 
    else
        Py_RETURN_FALSE;
}

PyObject* endTabItem(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI
    ImGui::EndTabItem();
    Py_RETURN_NONE;
}

PyObject* tabItemButton(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI
    auto& imguiBuilder = *(ImguiBuilder*)self;
    ModuleState& moduleState = parentModule(self);
    static char* argnames[] = { "label", "flags", nullptr };
    char* label = nullptr;
    int flags = 0;
    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "s|i", argnames, &label, &flags))
        return nullptr;

    if (ImGui::TabItemButton(label, flags))
        Py_RETURN_TRUE;
    else
        Py_RETURN_FALSE;
}

PyObject* setTabItemClosed(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI
    auto& imguiBuilder = *(ImguiBuilder*)self;
    ModuleState& moduleState = parentModule(self);
    static char* argnames[] = { "label", nullptr };
    char* label = nullptr;
    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "s", argnames, &label))
        return nullptr;

    ImGui::SetTabItemClosed(label);
    Py_RETURN_NONE;
}

PyObject* treeNode(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI
    auto& imguiBuilder = *(ImguiBuilder*)self;
    ModuleState& moduleState = parentModule(self);
    static char* argnames[] = { "label", "flags", nullptr };
    char* label = nullptr;
    int flags = 0;
    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "s|i", argnames, &label, &flags))
        return nullptr;

    if (ImGui::TreeNodeEx(label, flags))
        Py_RETURN_TRUE;
    else
        Py_RETURN_FALSE;
}

PyObject* treeNodeWithId(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI
    auto& imguiBuilder = *(ImguiBuilder*)self;
    ModuleState& moduleState = parentModule(self);
    static char* argnames[] = { "id", "text", "flags", nullptr };
    int id = 0;
    char* text = nullptr;
    int flags = 0;
    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "is|i", argnames, &id, &text, &flags))
        return nullptr;

    if (ImGui::TreeNodeEx((void*)(uintptr_t)id, flags, text))
        Py_RETURN_TRUE;
    else
        Py_RETURN_FALSE;
}

PyObject* treePop(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI
    ImGui::TreePop();
    Py_RETURN_NONE;
}

PyObject* colorEdit3(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    ModuleState& moduleState = parentModule(self);
    CHECK_IMGUI

    char* label = nullptr;
    PyObject* c = nullptr;
    int flags = 0;

    static char* argnames[] = { "label", "col", "flags", nullptr };
    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "sO|i", argnames, &label, &c, &flags))
        return nullptr;

    float colVal[3];
    if (!getTupleValuesFloat(c, colVal, 3, 3)) 
    {
        PyErr_SetString(moduleState.exObj(), "Cannot parse col value. Must be a tuple or list of length 3");
        return nullptr;
    }

    if (ImGui::ColorEdit3(label, colVal, flags))
        return Py_BuildValue("(fff)", colVal[0], colVal[1], colVal[2]);
    else
        Py_RETURN_NONE;
}


PyObject* colorEdit4(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    ModuleState& moduleState = parentModule(self);
    CHECK_IMGUI

    char* label = nullptr;
    PyObject* c = nullptr;
    int flags = 0;

    static char* argnames[] = { "label", "col", "flags", nullptr };
    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "sO|i", argnames, &label, &c, &flags))
        return nullptr;

    float colVal[4];
    if (!getTupleValuesFloat(c, colVal, 4, 4)) 
    {
        PyErr_SetString(moduleState.exObj(), "Cannot parse col value. Must be a tuple or list of length 4");
        return nullptr;
    }

    if (ImGui::ColorEdit4(label, colVal, flags))
        return Py_BuildValue("(ffff)", colVal[0], colVal[1], colVal[2], colVal[3]);
    else
        Py_RETURN_NONE;
}

PyObject* colorPicker3(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    ModuleState& moduleState = parentModule(self);
    CHECK_IMGUI

    char* label = nullptr;
    PyObject* c = nullptr;
    int flags = 0;

    static char* argnames[] = { "label", "col", "flags", nullptr };
    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "sO|i", argnames, &label, &c, &flags))
        return nullptr;

    float colVal[3];
    if (!getTupleValuesFloat(c, colVal, 3, 3)) 
    {
        PyErr_SetString(moduleState.exObj(), "Cannot parse col value. Must be a tuple or list of length 3");
        return nullptr;
    }

    if (ImGui::ColorPicker3(label, colVal, flags))
        return Py_BuildValue("(fff)", colVal[0], colVal[1], colVal[2]);
    else
        Py_RETURN_NONE;
}

PyObject* colorPicker4(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    ModuleState& moduleState = parentModule(self);
    CHECK_IMGUI

    char* label = nullptr;
    PyObject* c = nullptr;
    int flags = 0;

    static char* argnames[] = { "label", "col", "flags", nullptr };
    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "sO|i", argnames, &label, &c, &flags))
        return nullptr;

    float colVal[4];
    if (!getTupleValuesFloat(c, colVal, 4, 4)) 
    {
        PyErr_SetString(moduleState.exObj(), "Cannot parse col value. Must be a tuple or list of length 4");
        return nullptr;
    }

    if (ImGui::ColorPicker4(label, colVal, flags))
        return Py_BuildValue("(ffff)", colVal[0], colVal[1], colVal[2], colVal[3]);
    else
        Py_RETURN_NONE;
}

PyObject* colorButton(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    ModuleState& moduleState = parentModule(self);
    CHECK_IMGUI

    char* label = nullptr;
    PyObject* c = nullptr;
    PyObject* s = nullptr;
    int flags = 0;

    static char* argnames[] = { "desc_id", "col", "flags", "size", nullptr };
    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "sO|iO", argnames, &label, &c, &flags, &s))
        return nullptr;

    float sz[2] = { 0.0f, 0.0f };
    if (s != nullptr && !getTupleValuesFloat(s, sz, 2, 2)) 
    {
        PyErr_SetString(moduleState.exObj(), "Cannot parse size value. Must be a tuple or list of length 4");
        return nullptr;
    }

    float colVal[4];
    if (!getTupleValuesFloat(c, colVal, 4, 4)) 
    {
        PyErr_SetString(moduleState.exObj(), "Cannot parse col value. Must be a tuple or list of length 4");
        return nullptr;
    }

    ImVec2 sizeArg(sz[0], sz[1]);
    ImVec4 colArg(colVal[0], colVal[1], colVal[2], colVal[3]);
    if (ImGui::ColorButton(label, colArg, flags, sizeArg))
        Py_RETURN_TRUE;
    else
        Py_RETURN_FALSE;
}

PyObject* setColorEditOptions(PyObject* self, PyObject* vargs, PyObject* kwds)
{
    CHECK_IMGUI
    static char* argnames[] = { "flags", nullptr };
    int flags = 0;
    if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "i", argnames, &flags))
        return nullptr;

    ImGui::SetColorEditOptions(flags);
    Py_RETURN_NONE;
}


}//methods

}//gpu

}//coalpy
