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

COALPY_FN(setup_axes, setupAxes, R"(
    Sets the label and/or flags for primary X and Y axes (shorthand for two calls to SetupAxis).

    Parameters:
        x_label (str)
        y_label (str)
        x_flags (int)(optional): see coalpy.gpu.ImPlotAxisFlags 
        y_flags (int)(optional): see coalpy.gpu.ImPlotAxisFlags
)")

COALPY_FN(setup_axis_limits, setupAxisLimits, R"(
    Parameters:
        axis (int): see coalpy.gpu.ImAxis 
        v_min (float)
        v_max (float)
        cond (int)(optional): see coalpy.gpu.ImPlotCond , default is Once 
)")

COALPY_FN(plot_line, plotLine, R"(
    Parameters:
        label (str)
        values (byte object, or numpy array): array must be contiguous floating point values (x, y) laid down in memory. object must implement buffer protocol.
        count: the number of points
        offset: offset of points.

    Note:
        value is treated as a circular buffer, that is, elements are read in order from offset to count using modulus index.
)")

COALPY_FN(plot_shaded, plotShaded, R"(
    Parameters:
        label (str)
        values (byte object, or numpy array): array must be contiguous floating point values (x, y) laid down in memory. object must implement buffer protocol.
        count: the number of points
        yref (float): can be +/-float('inf')
        offset: offset of points.

    Note:
        value is treated as a circular buffer, that is, elements are read in order from offset to count using modulus index.
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

COALPY_ENUM_BEGIN(ImPlotAxisFlags, "")
COALPY_ENUM(NoLabel       , ImPlotAxisFlags_NoLabel        , R"(the axis label will not be displayed (axis labels also hidden if the supplied string name is NULL)")
COALPY_ENUM(NoGridLines   , ImPlotAxisFlags_NoGridLines    , R"(rid lines will be displayed)")
COALPY_ENUM(NoTickMarks   , ImPlotAxisFlags_NoTickMarks    , R"(ick marks will be displayed)")
COALPY_ENUM(NoTickLabels  , ImPlotAxisFlags_NoTickLabels   , R"(xt labels will be displayed)")
COALPY_ENUM(NoInitialFit  , ImPlotAxisFlags_NoInitialFit   , R"(will not be initially fit to data extents on the first rendered frame)")
COALPY_ENUM(NoMenus       , ImPlotAxisFlags_NoMenus        , R"(the user will not be able to open context menus with right-click)")
COALPY_ENUM(Opposite      , ImPlotAxisFlags_Opposite       , R"(xis ticks and labels will be rendered on conventionally opposite side (i.e, right or top)")
COALPY_ENUM(Foreground    , ImPlotAxisFlags_Foreground     , R"(d lines will be displayed in the foreground (i.e. on top of data) in stead of the background)")
COALPY_ENUM(LogScale      , ImPlotAxisFlags_LogScale       , R"( logartithmic (base 10) axis scale will be used (mutually exclusive with ImPlotAxisFlags_Time)")
COALPY_ENUM(Time          , ImPlotAxisFlags_Time           , R"(   axis will display date/time formatted labels (mutually exclusive with ImPlotAxisFlags_LogScale)")
COALPY_ENUM(Invert        , ImPlotAxisFlags_Invert         , R"( the axis will be inverted)")
COALPY_ENUM(AutoFit       , ImPlotAxisFlags_AutoFit        , R"(axis will be auto-fitting to data extents)")
COALPY_ENUM(RangeFit      , ImPlotAxisFlags_RangeFit       , R"(xis will only fit points if the point is in the visible range of the **orthogonal** axis)")
COALPY_ENUM(LockMin       , ImPlotAxisFlags_LockMin        , R"(the axis minimum value will be locked when panning/zooming)")
COALPY_ENUM(LockMax       , ImPlotAxisFlags_LockMax        , R"(the axis maximum value will be locked when panning/zooming)")
COALPY_ENUM(Lock          , ImPlotAxisFlags_Lock           , R"()")
COALPY_ENUM(NoDeforations ,  ImPlotAxisFlags_NoDecorations , R"()")
COALPY_ENUM(AuxDefault    , ImPlotAxisFlags_AuxDefault     , R"()")
COALPY_ENUM_END(ImPlotAxisFlags)

COALPY_ENUM_BEGIN(ImAxis, "")
COALPY_ENUM(X1, ImAxis_X1, R"(enabled by default)")
COALPY_ENUM(X2, ImAxis_X2, R"(disabled by default)")
COALPY_ENUM(X3, ImAxis_X3, R"(disabled by default)")
COALPY_ENUM(Y1, ImAxis_Y1, R"(enabled by default)")
COALPY_ENUM(Y2, ImAxis_Y2, R"(disabled by default)")
COALPY_ENUM(Y3, ImAxis_Y3, R"(disabled by default)")
COALPY_ENUM(COUNT, ImAxis_COUNT, "")
COALPY_ENUM_END(ImAxis)

COALPY_ENUM_BEGIN(ImPlotCond, "")
COALPY_ENUM(None  , ImPlotCond_None   , R"(No condition (always set the variable), same as _Always)")
COALPY_ENUM(Always, ImPlotCond_Always , R"(No condition (always set the variable))")
COALPY_ENUM(Once  , ImPlotCond_Once   , R"(Set the variable once per runtime session (only the first call will succeed))")
COALPY_ENUM_END(ImPlotCond)
#undef COALPY_ENUM_END
#undef COALPY_ENUM_BEGIN
#undef COALPY_ENUM
#undef COALPY_FN 
