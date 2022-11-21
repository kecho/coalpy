//Include before inserting into outTypes array enumeration bindings.

#ifndef STRINGIFY
#define STRINGIFY(x) #x
#endif
#define COALPY_ENUM_BEGIN(name, desc) \
    {\
        static const char* s_EnumName ## name = "gpu.Enum" STRINGIFY(name);\
        static const char* s_EnumPyName ## name = STRINGIFY(name);\
        static const char* s_EnumPyDesc ## name = desc;\
        static const EnumEntry s_##name[] = {

#define COALPY_ENUM(pyname, cppname, desc) { #pyname, (int)cppname, desc },
#define COALPY_ENUM_END(name) \
        {nullptr, 0, nullptr}\
        };\
        outTypes.push_back(RenderEnum::constructEnumType(s_EnumName##name, s_EnumPyName##name, s_##name, s_EnumPyDesc##name));\
    }
