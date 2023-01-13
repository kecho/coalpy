#include "ModuleSettings.h"
#include "CoalpyTypeObject.h"
#include "HelperMacros.h"

namespace coalpy
{
namespace gpu
{

IMPLEMENT_SETTING(ModuleSettings)

namespace methods
{
    #include "bindings/MethodDecl.h"
    #include "bindings/ModuleSettings.inl"
}

static PyMethodDef g_moduleSettingsMethods[] = {
    #include "bindings/MethodDef.h"
    #include "bindings/ModuleSettings.inl"
    FN_END
};

const char* ModuleSettings::sSettingsFileName = ".coalpy_settings.json";

void ModuleSettings::constructType(CoalpyTypeObject& o)
{
    ModuleSettings::registerSchema();
    auto& t = o.pyObj;
    t.tp_name = "gpu.Settings";
    t.tp_basicsize = sizeof(ModuleSettings);
    t.tp_flags = Py_TPFLAGS_DEFAULT;
    t.tp_getset = ModuleSettings::s_schema.pyGetSetTable();
    t.tp_methods = g_moduleSettingsMethods;
}

namespace methods
{
    PyObject* saveSettings(PyObject* self, PyObject* vargs, PyObject* kwds)
    {
        ModuleState& moduleState = parentModule(self);
        if (!moduleState.checkValidDevice())
            return nullptr;

        ModuleSettings* settingsObj = reinterpret_cast<ModuleSettings*>(self);

        static char* arguments[] = { "filename", nullptr };
        char* fileName = nullptr;
        if (!PyArg_ParseTupleAndKeywords(vargs, kwds, "|s", arguments, &fileName))
            return nullptr;

        const char* targetName = fileName == nullptr ? ModuleSettings::sSettingsFileName : fileName;
        if (settingsObj->serialize(moduleState.fs(), targetName))
            Py_RETURN_TRUE;
        else
            Py_RETURN_FALSE;
    }
}

}
}
