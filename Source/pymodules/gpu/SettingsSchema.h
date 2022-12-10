#pragma once

#include <map>
#include <string>
#include <variant>

#define BEGIN_PARAM_TABLE(settingsType)\
    using ThisType=settingsType;\
    static SettingsSchema s_schema;\
    static inline void registerSchema(){\
        s_schema = SettingsSchema();
#define REGISTER_PARAM(type, name) ThisType::s_schema.declParameter(\
    std::string(#name), offsetof(ThisType, name), SettingsParamTypeTrait<type>::enumType());
#define END_PARAM_TABLE() }
#define IMPLEMENT_SETTING(type) SettingsSchema type::s_schema;

namespace coalpy
{
namespace gpu
{

enum class SettingsParamType
{
    INT, FLOAT, STRING
};

template<typename T>
struct SettingsParamTypeTrait {};
template<> struct SettingsParamTypeTrait<int>         { static inline SettingsParamType enumType() {return SettingsParamType::INT;} };
template<> struct SettingsParamTypeTrait<float>       { static inline SettingsParamType enumType() {return SettingsParamType::FLOAT;} };
template<> struct SettingsParamTypeTrait<std::string> { static inline SettingsParamType enumType() {return SettingsParamType::STRING;} };

class SettingsSchema
{
public:
    void declParameter(std::string name, size_t offset, SettingsParamType type);

private:
    struct Record
    {
        std::string name;
        SettingsParamType type;
        size_t offset;
    };

    std::map<std::string,Record> m_records;
};

}
}
