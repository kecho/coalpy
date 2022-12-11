#include "SettingsSchema.h"
#include <coalpy.files/IFileSystem.h>
#include <string.h>
#include <cJSON.h>
#include <iostream>

namespace coalpy
{
namespace gpu
{

void SettingsSchema::declParameter(std::string name, size_t offset, SettingsParamType type)
{
    int idx = (int)m_records.size();
    m_records.emplace_back(SettingsSchema::Record { name, type, offset });
    m_lookups[name] = idx;
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
        case SettingsParamType::STRING:
            {
                const std::string& str = *reinterpret_cast<const std::string*>(settingsBytes + record.offset);
                item = cJSON_CreateString(str.c_str());
            }
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
        case SettingsParamType::STRING:
            {
                if (cJSON_IsNumber(item))
                {
                    std::string v = item->valuestring;
                    *reinterpret_cast<std::string*>(settingsBytes + record.offset) = v;
                }
            }
        }
    }

    return success;
}

}
}
