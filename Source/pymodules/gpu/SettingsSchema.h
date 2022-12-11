#pragma once

#include <map>
#include <vector>
#include <string>

#define BEGIN_PARAM_TABLE(settingsType)\
    using ThisType=settingsType;\
    static SettingsSchema s_schema;\
    bool serialize(IFileSystem& fs, const char* filename) const { return s_schema.serialize(fs, filename, this); }\
    bool load(IFileSystem& fs, const char* filename) { return s_schema.load(fs, filename, this); }\
    static inline void registerSchema(){\
        ThisType settingsObj;\
        s_schema = SettingsSchema();
#define REGISTER_PARAM( name) ThisType::s_schema.declParameter(\
    std::string(#name), offsetof(ThisType, name), SettingsParamTypeTrait::enumType(settingsObj.name));
#define END_PARAM_TABLE() }
#define IMPLEMENT_SETTING(type) SettingsSchema type::s_schema;

namespace coalpy
{

class IFileSystem;

namespace gpu
{

enum class SettingsParamType
{
    INT, FLOAT, STRING
};

struct SettingsParamTypeTrait
{
    static SettingsParamType enumType(int v) {return SettingsParamType::INT;}
    static SettingsParamType enumType(float v) {return SettingsParamType::FLOAT;}
    static SettingsParamType enumType(std::string v) {return SettingsParamType::STRING;}
};

class SettingsSchema
{
public:
    void declParameter(std::string name, size_t offset, SettingsParamType type);
    bool serialize(IFileSystem& fs, const char* filename, const void* settingsObj);
    bool load(IFileSystem& fs, const char* filename, void* settingsObj);

private:
    struct Record
    {
        std::string name;
        SettingsParamType type;
        size_t offset;
    };

    std::vector<Record> m_records;
    std::map<std::string,int> m_lookups;
};

}
}
