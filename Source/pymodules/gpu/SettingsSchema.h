#pragma once

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <map>
#include <vector>
#include <string>
#include <cstddef>

#define BEGIN_PARAM_TABLE(settingsType)\
    using ThisType=settingsType;\
    static SettingsSchema s_schema;\
    bool serialize(IFileSystem& fs, const char* filename) const { return s_schema.serialize(fs, filename, this); }\
    bool load(IFileSystem& fs, const char* filename) { return s_schema.load(fs, filename, this); }\
    static inline void registerSchema(){\
        ThisType settingsObj;\
        s_schema = SettingsSchema();
#define REGISTER_PARAM(name, docStr) ThisType::s_schema.declParameter(\
    std::string(#name), docStr, offsetof(ThisType, name), SettingsParamTypeTrait::enumType(settingsObj.name));
#define END_PARAM_TABLE()\
     ThisType::s_schema.genPyGetSetterTable();}

#define IMPLEMENT_SETTING(type) SettingsSchema type::s_schema;

namespace coalpy
{

class IFileSystem;

namespace gpu
{

enum class SettingsParamType
{
    BOOL, INT, FLOAT, STRING
};

struct SettingsParamTypeTrait
{
    static SettingsParamType enumType(int v)   {return SettingsParamType::INT;}
    static SettingsParamType enumType(float v) {return SettingsParamType::FLOAT;}
    static SettingsParamType enumType(bool v)  {return SettingsParamType::BOOL;}
    static SettingsParamType enumType(std::string v) {return SettingsParamType::STRING;}
};

class SettingsSchema
{
public:
    void declParameter(std::string name, const char* doc, size_t offset, SettingsParamType type);
    bool serialize(IFileSystem& fs, const char* filename, const void* settingsObj);
    bool load(IFileSystem& fs, const char* filename, void* settingsObj);
    void genPyGetSetterTable();
    PyGetSetDef* pyGetSetTable() { return m_pyGetSetters.data(); }

private:
    struct Record
    {
        std::string name;
        const char* doc;
        SettingsParamType type;
        size_t offset;
    };

    static PyObject* pyGetter(PyObject* instance, void* closure);
    static int pySetter(PyObject* instance, PyObject* value, void* closure);

    std::vector<Record> m_records;
    std::map<std::string,int> m_lookups;
    std::vector<PyGetSetDef> m_pyGetSetters;
};

}
}
