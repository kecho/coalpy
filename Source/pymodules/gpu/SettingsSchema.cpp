#include "SettingsSchema.h"

namespace coalpy
{
namespace gpu
{

void SettingsSchema::declParameter(std::string name, size_t offset, SettingsParamType type)
{
    m_records.insert(std::pair<std::string,SettingsSchema::Record>(name, SettingsSchema::Record { name, type, offset }));
}

}
}
