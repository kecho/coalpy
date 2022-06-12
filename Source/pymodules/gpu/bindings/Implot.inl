#ifndef COALPY_FN
#define COALPY_FN(pyname, cppname, desc)
#endif

#ifndef COALPY_ENUM
#define COALPY_ENUM(pyname, cppname, desc)
#endif

#ifndef COALPY_ENUM_BEGIN
#define COALPY_ENUM_BEGIN(name, desc)
#endif

#ifndef COALPY_ENUM_END
#define COALPY_ENUM_END(name)
#endif

//Add here all your imgui forwarding functions. Add implementation in ImguiBuilder.cpp
//COALPY_FN(python name, c++ name, documentation)
COALPY_FN(show_demo_window, showDemoWindow, R"(
    Shows the demo window.
)")

#undef COALPY_ENUM_END
#undef COALPY_ENUM_BEGIN
#undef COALPY_ENUM
#undef COALPY_FN 
