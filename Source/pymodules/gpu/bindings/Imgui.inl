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
COALPY_FN(begin, begin,R"(
    Begin method, starts the scope of a ImGUI interface. This method must have a corresponding end at some point denoting 
    denoting the scope of all functions required.

    Parameters:
        name (str): Mandatory name of this interface panel.
        is_open (bool)(optional): Specifies the previous state of this panel. True for open, false otherwise.
                                  Update this state with the return value of this function.

        is_fullscreen (bool)(optional): sets this window to follow the main viewport.
        flags (int)(optional): See coalpy.gpu.ImGuiWindowFlags for flag definitions

    Returns:
        returns a boolean, True if the X for closing this panel was pressed, False otherwise.
)")

COALPY_FN(end, end, R"(
    End method. Must be paired with a corresponding begin method.
)")

COALPY_FN(push_id, pushId, R"(
    Pushes a custom id name for a scope. 
    See (docs/FAQ.md or http://dearimgui.org/faq) for more details about how ID are handled in dear imgui.
    This call to push_id must be paired with a corresponding pop_id call.

    Parameters:
        text (str): The identifier for this id stack scope.
)")

COALPY_FN(pop_id, popId, R"(
    Pops a corresponding name for a scope. Must be paired with a push_id.
)")

COALPY_FN(begin_main_menu_bar, beginMainMenuBar, R"(
    Starts the scope of a main menu at the top of the window. Use the MainMenu and BeginMenu/EndMenu functions to add / remove components.
    Must be paired with a end_main_menu_bar method.

    Returns:
        if returns true, call end_main_menu_bar. If returns false means its not rendered.
)")

COALPY_FN(end_main_menu_bar, endMainMenuBar, R"(
    End method for main menu bar. Must be paired with a begin_main_menu_bar method.
    
    Only call this if begin_main_menu_bar returns true.
)")

COALPY_FN(begin_menu, beginMenu, R"(
    Starts a menu item in the menu bar. Must be paird with an end_menu method.
    Parameters:
        label (str): name of this menu item. Can recursively have more menu items.

    Returns:
        if returns true, call end_menu. If returns false means its not rendered.
)")

COALPY_FN(menu_item, menuItem, R"(
    Parameters:
        label (str) : label of this menu item.
        shortcut (str): short cut key for this menu item.
        enabled (bool) : enables / disables menu item.

    Returns:
        True if activated. False otherwise.
)")

COALPY_FN(end_menu, endMenu, R"(
    End method for menu item. Must be paired with a begin_menu method.
    Only call this if begin_menu returns true.
)")

COALPY_FN(collapsing_header, collapsingHeader, R"(
    Returns the state of a header label (collapsed or non collapsed).

    Parameters:
        (label) str : label of this item.

    Returns:
        True if open, False otherwise.
)")

COALPY_FN(show_demo_window, showDemoWindow, R"(
    Shows the demo window.
)")

COALPY_FN(slider_float, sliderFloat, R"(
    Draws a slider for a float value.

    Parameters:
        label (str): the label name for this slider.
        v (float): the actual value to draw the slider.
        v_min (float); the minimum possible value.
        v_max (float): the maximum possible value.
        fmt (str)(optional): A formatting value to draw the float. For example %.3f draws 3 decimal precision.

    Returns:
        The new float value that the user set. Feed this value back on v on the next call to see a proper state update.
)")

COALPY_FN(input_float, inputFloat, R"(
    Draws a box to input a single float value.
    
    Parameters:
        label (str): the label name for this box.
        v (float): the actual value to draw the slider.
        step (float)(optional): step value when scrolling slow
        step_fast (float)(optional): step value when scrolling fast.
        fmt (str)(optional): A formatting value to draw the float. For example %.3f draws 3 decimal precision.

    Returns:
        The new float value that the user set. Feed this value back on v on the next call to see a proper state update.
)")

COALPY_FN(input_float3, inputFloat3, R"(
    Draws a box to input a float2 value.
    
    Parameters:
        label (str): the label name for this box.
        v (float): the actual value to draw the slider. (tuple or array)
        step (float)(optional): step value when scrolling slow
        step_fast (float)(optional): step value when scrolling fast.
        fmt (str)(optional): A formatting value to draw the float. For example %.3f draws 3 decimal precision.

    Returns:
        The new float2 value that the user set. Feed this value back on v on the next call to see a proper state update.
)")

COALPY_FN(input_float3, inputFloat3, R"(
    Draws a box to input a float3 value.
    
    Parameters:
        label (str): the label name for this box.
        v (float): the actual value to draw the slider. (tuple or array)
        step (float)(optional): step value when scrolling slow
        step_fast (float)(optional): step value when scrolling fast.
        fmt (str)(optional): A formatting value to draw the float. For example %.3f draws 3 decimal precision.

    Returns:
        The new float3 value that the user set. Feed this value back on v on the next call to see a proper state update.
)")

COALPY_FN(input_float4, inputFloat4, R"(
    Draws a box to input a float4 value.
    
    Parameters:
        label (str): the label name for this box.
        v (float): the actual value to draw the slider. (tuple or array)
        step (float)(optional): step value when scrolling slow
        step_fast (float)(optional): step value when scrolling fast.
        fmt (str)(optional): A formatting value to draw the float. For example %.3f draws 3 decimal precision.

    Returns:
        The new float4 value that the user set. Feed this value back on v on the next call to see a proper state update.
)")

COALPY_FN(input_text, inputText, R"(
    Draws a box for input text.
    
    Parameters:
        label (str): the label name for this box.
        v (str): the actual string value to display by default.

    Returns:
        The new str value that the user set. Feed this value back on v on the next call to see a proper state update.
)")

COALPY_FN(text, text, R"(
    Draws a box with solid text.

    Parameters:
        text (str): the label value to display
)")

COALPY_FN(button, button, R"(
    Draws a button.

    Parameters:
        label (str): the label name for this button.

    Returns:
        True if button was pressed, false otherwise.
)")

COALPY_FN(checkbox, checkbox, R"(
    Draws a checkbox ui item.

    Parameters:
        label (str): the label name for this checkbox.
        v (bool): The current state of the checkbox

    Returns:
        The new bool value that the user set. Feed this value back on v on the next call to see a proper state update.
)")

COALPY_FN(begin_combo, beginCombo, R"(
    begins a combo box. Must be paired with an end_combo call.
    To insert items, use selectable() between begin_combo/end_combo

    Parameters:
        label (str): the label name for this comboxbox.
        preview_value (str): preview string value

    Returns:
        If value returned is true, must call end_combo.
)")

COALPY_FN(end_combo, endCombo, R"(
    Ends a combo box. Must be paired with an begin_combo call only if it returns true..
)")

COALPY_FN(selectable, selectable, R"(
    Draws a selectable item. 

    Parameters:
        label (str): the label name for this selectable.
        selected (str): the state if this value is selected. 

    Returns:
        True if item is selected, false otherwise.
)")

COALPY_FN(separator, separator, R"(
    Draw a separator.
)")

COALPY_FN(same_line, sameLine, R"(
    Sets the same line for drawing next item.

    Parameters:
        offset_from_start_x (float)(optional): offset from the start of this x
        spacing (float)(optional): spacing amount
)")

COALPY_FN(new_line, newLine, R"(
    Draws new line.
)")

COALPY_FN(image, image, R"(
    Draws an image

    Parameters:
        texture (Texture): coalpy.gpu.Texture object to draw in the UI.
        size (tuple): size of the image in pixels
        uv0 (tuple)(optional): uv coordinate of bottom (default (0.0, 0.0))
        uv1 (tuple)(optional): uv coordinate of top  (default (1.0, 1.0))
        tint_col (tuple): size 4 tuple with image tint color (default (1,1,1,1)).
        border_col (tuple): size 4 tuple with background color (default (0,0,0,0)).
)")

COALPY_FN(get_mouse_pos, getMousePos, R"(
    Gets the relative mouse position of the current window

    Returns:
        Mouse pos (tuple x, y)
)")

COALPY_FN(get_window_size, getWindowSize, R"(
    Gets the current window size

    Returns:
        Window size (tuple x, y)
)")

COALPY_FN(set_window_size, setWindowSize, R"(
    Sets the current window size

    Parameters:
        size (tuple x, y): the window size to set
)")

COALPY_FN(is_window_focused, isWindowFocused, R"(
    Returns boolean with current window focused.

    Parameters:
        flags (int)(optional): see coalpy.gpu.ImguiFocusedFlags

    Returns:
        True if window is focused, false otherwise
)")

COALPY_FN(set_window_focus, setWindowFocus, R"(
    Sets the current windows focused.
)")

COALPY_FN(dockspace, dockspace, R"(
    Creates an explicit dockspace

    Parameters:
        label (str): the docking space name
)")

//Imgui focus flags enums
COALPY_ENUM_BEGIN(ImGuiFocusedFlags, "ImGUI Focused flags")
COALPY_ENUM(ChildWindows,             ImGuiFocusedFlags_ChildWindows                  , R"(Return true if any children of the window is focused)")
COALPY_ENUM(RootWindow,               ImGuiFocusedFlags_RootWindow                    , R"(Test from root window (top most parent of the current hierarchy))")
COALPY_ENUM(AnyWindow,                ImGuiFocusedFlags_AnyWindow                     , R"(Return true if any window is focused. Important: If you are trying to tell how to dispatch your low-level inputs, do NOT use this. Use 'io.WantCaptureMouse' instead! Please read the FAQ!)")
COALPY_ENUM(NoPopupHierarchy,         ImGuiFocusedFlags_NoPopupHierarchy              , R"(Do not consider popup hierarchy (do not treat popup emitter as parent of popup) (when used with _ChildWindows or _RootWindow))")
COALPY_ENUM(DockHierarchy,            ImGuiFocusedFlags_DockHierarchy                 , R"(Consider docking hierarchy (treat dockspace host as parent of docked window) (when used with _ChildWindows or _RootWindow))")
COALPY_ENUM(RootAndChildWindows,      ImGuiFocusedFlags_RootAndChildWindows           , "All the windows.")
COALPY_ENUM_END(ImGuiFocusedFlags)

//Imgui window enums
COALPY_ENUM_BEGIN(ImGuiWindowFlags,   "ImGUI Window flags")
COALPY_ENUM(NoTitleBar,               ImGuiWindowFlags_NoTitleBar                , R"(Disable title-bar)")
COALPY_ENUM(NoResize,                 ImGuiWindowFlags_NoResize                  , R"(Disable user resizing with the lower-right grip)")
COALPY_ENUM(NoMove,                   ImGuiWindowFlags_NoMove                    , R"(Disable user moving the window)")
COALPY_ENUM(NoScrollbar,              ImGuiWindowFlags_NoScrollbar               , R"(Disable scrollbars (window can still scroll with mouse or programmatically))")
COALPY_ENUM(NoScrollWithMouse,        ImGuiWindowFlags_NoScrollWithMouse         , R"(Disable user vertically scrolling with mouse wheel. On child window, mouse wheel will be forwarded to the parent unless NoScrollbar is also set.)")
COALPY_ENUM(NoCollapse,               ImGuiWindowFlags_NoCollapse                , R"(Disable user collapsing window by double-clicking on it. Also referred to as Window Menu Button (e.g. within a docking node).)")
COALPY_ENUM(AlwaysAutoResize,         ImGuiWindowFlags_AlwaysAutoResize          , R"(Resize every window to its content every frame)")
COALPY_ENUM(NoBackground,             ImGuiWindowFlags_NoBackground              , R"(Disable drawing background color (WindowBg, etc.) and outside border. Similar as using SetNextWindowBgAlpha(0.0f).)")
COALPY_ENUM(NoSavedSettings,          ImGuiWindowFlags_NoSavedSettings           , R"(Never load/save settings in .ini file)")
COALPY_ENUM(NoMouseInputs,            ImGuiWindowFlags_NoMouseInputs             , R"(Disable catching mouse, hovering test with pass through.)")
COALPY_ENUM(MenuBar,                  ImGuiWindowFlags_MenuBar                   , R"(Has a menu-bar)")
COALPY_ENUM(HorizontalScrollbar,      ImGuiWindowFlags_HorizontalScrollbar       , R"(Allow horizontal scrollbar to appear (off by default). You may use SetNextWindowContentSize(ImVec2(width,0.0f)); prior to calling Begin() to specify width. Read code in imgui_demo in the "Horizontal Scrolling" section.)")
COALPY_ENUM(NoFocusOnAppearing,       ImGuiWindowFlags_NoFocusOnAppearing        , R"(Disable taking focus when transitioning from hidden to visible state)")
COALPY_ENUM(NoBringToFrontOnFocus,    ImGuiWindowFlags_NoBringToFrontOnFocus     , R"(Disable bringing window to front when taking focus (e.g. clicking on it or programmatically giving it focus))")
COALPY_ENUM(AlwaysVerticalScrollbar,  ImGuiWindowFlags_AlwaysVerticalScrollbar   , R"(Always show vertical scrollbar (even if ContentSize.y < Size.y))")
COALPY_ENUM(AlwaysHorizontalScrollbar,ImGuiWindowFlags_AlwaysHorizontalScrollbar , R"(Always show horizontal scrollbar (even if ContentSize.x < Size.x))")
COALPY_ENUM(AlwaysUseWindowPadding,   ImGuiWindowFlags_AlwaysUseWindowPadding    , R"(Ensure child windows without border uses style.WindowPadding (ignored by default for non-bordered child windows, because more convenient))")
COALPY_ENUM(NoNavInputs,              ImGuiWindowFlags_NoNavInputs               , R"(No gamepad/keyboard navigation within the window)")
COALPY_ENUM(NoNavFocus,               ImGuiWindowFlags_NoNavFocus                , R"(No focusing toward this window with gamepad/keyboard navigation (e.g. skipped by CTRL+TAB))")
COALPY_ENUM(UnsavedDocument,          ImGuiWindowFlags_UnsavedDocument           , R"(Display a dot next to the title. When used in a tab/docking context, tab is selected when clicking the X + closure is not assumed (will wait for user to stop submitting the tab). Otherwise closure is assumed when pressing the X, so if you keep submitting the tab may reappear at end of tab bar.)")
COALPY_ENUM(NoDocking,                ImGuiWindowFlags_NoDocking                 , R"(Disable docking of this window)")
COALPY_ENUM_END(ImGuiWindowFlags)

#undef COALPY_ENUM_END
#undef COALPY_ENUM_BEGIN
#undef COALPY_ENUM
#undef COALPY_FN 
