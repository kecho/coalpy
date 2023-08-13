#include "SettingsSchema.h"
#include <coalpy.files/IFileSystem.h>
#include <string.h>
#include <cJSON.h>
#include <iostream>
#include <dictobject.h>

namespace coalpy
{
namespace gpu
{

void SettingsSchema::declParameter(std::string name, const char* doc, size_t offset, SettingsParamType type)
{
    int idx = (int)m_records.size();
    m_records.emplace_back(SettingsSchema::Record { name, doc, type, offset });
    m_lookups[name] = idx;
}

void SettingsSchema::genPyGetSetterTable()
{
    m_pyGetSetters.resize(m_records.size() + 1);
    m_pyGetSetters[m_records.size()] = { nullptr };

    for (int i = 0; i < (int)m_records.size(); ++i)
    {
        Record& r = m_records[i];
        PyGetSetDef& getSetter = m_pyGetSetters[i];
        getSetter.name = r.name.c_str();
        getSetter.doc = r.doc;
        getSetter.get = &pyGetter;
        getSetter.set = &pySetter;
        getSetter.closure = (void*)&r;
    }
}

PyObject* SettingsSchema::pyGetter(PyObject* instance, void* closure)
{
    const Record& record = *reinterpret_cast<Record*>(closure);
    const char* settingsBytes = reinterpret_cast<const char*>(instance);
    PyObject* memberVal = nullptr;
    switch (record.type)
    {
    case SettingsParamType::INT:
        {
            int v = *reinterpret_cast<const int*>(settingsBytes + record.offset);
            memberVal = Py_BuildValue("i", v);
        }
        break;
    case SettingsParamType::FLOAT:
        {
            float v = *reinterpret_cast<const float*>(settingsBytes + record.offset);
            memberVal = Py_BuildValue("f", v);
        }
        break;
    case SettingsParamType::STRING:
        {
            const std::string& str = *reinterpret_cast<const std::string*>(settingsBytes + record.offset);
            memberVal = Py_BuildValue("s", str.c_str());
        }
        break;
    case SettingsParamType::BOOL:
        {
            bool b = *reinterpret_cast<const bool*>(settingsBytes + record.offset);
            memberVal = b ? Py_True : Py_False;
            Py_INCREF(memberVal);
        }
        break;
    }
    return memberVal;
}

int SettingsSchema::pySetter(PyObject* instance, PyObject* value, void* closure)
{
    const Record& record = *reinterpret_cast<Record*>(closure);
    char* settingsBytes = reinterpret_cast<char*>(instance);
    int ret = 0;
    switch (record.type)
    {
    case SettingsParamType::INT:
        {
            int& v = *reinterpret_cast<int*>(settingsBytes + record.offset);
            ret = PyArg_Parse(value, "i", &v);
        }
        break;
    case SettingsParamType::FLOAT:
        {
            float& v = *reinterpret_cast<float*>(settingsBytes + record.offset);
            ret = PyArg_Parse(value, "f", &v);
        }
        break;
    case SettingsParamType::STRING:
        {
            std::string& str = *reinterpret_cast<std::string*>(settingsBytes + record.offset);
            char* val = nullptr;
            ret = PyArg_Parse(value, "s", &val);
            if (ret != 0)
                str = val;
        }
        break;
    case SettingsParamType::BOOL:
        {
            bool& b = *reinterpret_cast<bool*>(settingsBytes + record.offset);
            int val = 0;
            ret = PyArg_Parse(value, "p", &val);
            b = (bool)val;
        }
        break;
    default:
        return -1;
    }
    
    return ret == 0 ? -1 : 0;
}

bool SettingsSchema::serialize(IFileSystem& fs, const char* filename, const void* settingsObj)
{
    const char* settingsBytes = (const char*)settingsObj;
    cJSON* root = cJSON_CreateObject();
    for (auto& record : m_records)
    {
        cJSON* item = nullptr;
        switch (record.type)
        {
        case SettingsParamType::INT:
            {
                int v = *reinterpret_cast<const int*>(settingsBytes + record.offset);
                item = cJSON_CreateNumber(v);
            }
            break;
        case SettingsParamType::FLOAT:
            {
                float v = *reinterpret_cast<const float*>(settingsBytes + record.offset);
                item = cJSON_CreateNumber(v);
            }
            break;
        case SettingsParamType::STRING:
            {
                const std::string& str = *reinterpret_cast<const std::string*>(settingsBytes + record.offset);
                item = cJSON_CreateString(str.c_str());
            }
            break;
        case SettingsParamType::BOOL:
            {
                const bool& b = *reinterpret_cast<const bool*>(settingsBytes + record.offset);
                item = cJSON_CreateBool(b ? 1 : 0);
            }
            break;
        }

        if (item != nullptr)
            cJSON_AddItemToObject(root, record.name.c_str(), item);
    }

    char* str = cJSON_Print(root);
    bool success = true;
    AsyncFileHandle handle = fs.write(FileWriteRequest(
        filename, [&](FileWriteResponse response)
        {
            success = success && response.status != FileStatus::Fail;
        }, str, strlen(str)));
    fs.execute(handle);
    fs.wait(handle);
    fs.closeHandle(handle);
    free(str);
    cJSON_Delete(root);
    return success;
}

bool SettingsSchema::load(IFileSystem& fs, const char* filename, void* settingsObj)
{
    bool success = false;
    cJSON* root = nullptr;
    AsyncFileHandle handle = fs.read(FileReadRequest(
        filename, [&](FileReadResponse& response)
        {
            if (response.status == FileStatus::Reading)
            {
                root = cJSON_Parse(response.buffer);
                success = true;
            }
        }));

    fs.execute(handle);
    fs.wait(handle);
    fs.closeHandle(handle);

    if (!success || root == nullptr)
        return false;

    char* settingsBytes = (char*)settingsObj;
    for (auto& record : m_records)
    {
        cJSON* item = cJSON_GetObjectItemCaseSensitive(root, record.name.c_str());
        if (item == nullptr)
            continue;

        switch (record.type)
        {
        case SettingsParamType::INT:
            {
                if (cJSON_IsNumber(item))
                {
                    int v = (int)item->valuedouble;
                    *reinterpret_cast<int*>(settingsBytes + record.offset) = v;
                }
            }
            break;
        case SettingsParamType::FLOAT:
            {
                if (cJSON_IsNumber(item))
                {
                    float v = (float)item->valuedouble;
                    *reinterpret_cast<float*>(settingsBytes + record.offset) = v;
                }
            }
            break;
        case SettingsParamType::STRING:
            {
                if (cJSON_IsString(item))
                {
                    std::string v = item->valuestring;
                    *reinterpret_cast<std::string*>(settingsBytes + record.offset) = v;
                }
            }
            break;
        case SettingsParamType::BOOL:
            {
                if (cJSON_IsBool(item))
                {
                    bool v = cJSON_IsTrue(item);
                    *reinterpret_cast<bool*>(settingsBytes + record.offset) = v;
                }
            }
            break;
        }
    }

    return success;
}

}
}
