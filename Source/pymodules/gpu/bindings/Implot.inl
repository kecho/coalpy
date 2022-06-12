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

COALPY_FN(begin_plot, beginPlot, R"(
    Starts a 2D plotting context. If this function returns true, EndPlot() MUST
    be called! You are encouraged to use the following convention:
   
    if implot.begin_plot(...):
        ...
        implot.end_plot()

    Parameters:
        title_id (str): must be unique to the current ImGui ID scope. If you need to avoid ID
                 collisions or don't want to display a title in the plot, use double hashes
                 (e.g. "MyPlot##HiddenIdText" or "##NoTitle").
        size (optional)(tuple 2 f): is the **frame** size of the plot widget, not the plot area. The default
          size of plots (i.e. when ImVec2(0,0)) can be modified in your ImPlotStyle.
            default value is (-1.0, 0.0)
    
        flags (int): see coalpy.gpu.ImPlotFlags
    
    Returns:
        True if is open, False otherwise
        
)")

COALPY_FN(end_plot, endPlot, R"(
    Only call EndPlot() if BeginPlot() returns true! Typically called at the end
    of an if statement conditioned on BeginPlot(). See example above.
)")


/*
COALPY_FN(begin_subplots, beginSubplots, R"(
)")

COALPY_FN(end_subplots, endSubplots, R"(
)")
*/

COALPY_ENUM_BEGIN(ImPlotFlags, "")
COALPY_ENUM(NoTitle       , ImPlotFlags_NoTitle      ,  R"( the plot title will not be displayed (titles are also hidden if preceeded by double hashes, e.g. "##MyPlot") )")
COALPY_ENUM(NoLegend      , ImPlotFlags_NoLegend     ,  R"( the legend will not be displayed )")
COALPY_ENUM(NoMouseText   , ImPlotFlags_NoMouseText  ,  R"( the mouse position, in plot coordinates, will not be displayed inside of the plot )")
COALPY_ENUM(NoInputs      , ImPlotFlags_NoInputs     ,  R"( the user will not be able to interact with the plot )")
COALPY_ENUM(NoMenus       , ImPlotFlags_NoMenus      ,  R"( the user will not be able to open context menus )")
COALPY_ENUM(NoBoxSelect   , ImPlotFlags_NoBoxSelect  ,  R"( the user will not be able to box-select )")
COALPY_ENUM(NoChild       , ImPlotFlags_NoChild      ,  R"( a child window region will not be used to capture mouse scroll (can boost performance for single ImGui window applications) )")
COALPY_ENUM(NoFrame       , ImPlotFlags_NoFrame      ,  R"( the ImGui frame will not be rendered )")
COALPY_ENUM(Equal         , ImPlotFlags_Equal        ,  R"( x and y axes pairs will be constrained to have the same units/pixel )")
COALPY_ENUM(Crosshairs    , ImPlotFlags_Crosshairs   ,  R"( the default mouse cursor will be replaced with a crosshair when hovered )")
COALPY_ENUM(AntiAliased   , ImPlotFlags_AntiAliased  ,  R"(pot items will be software anti-aliased (not recommended for high density plots, prefer MSAA) )")
COALPY_ENUM(CanvasOnly    , ImPlotFlags_CanvasOnly   , R"()")
COALPY_ENUM_END(ImPlotFlags)

#undef COALPY_ENUM_END
#undef COALPY_ENUM_BEGIN
#undef COALPY_ENUM
#undef COALPY_FN 
