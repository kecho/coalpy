#ifndef IMGUI_FN
#define IMGUI_FN(pyname, cppname, desc)
#endif

#ifndef IMGUI_ENUM
#define IMGUI_ENUM(pyname, cppname, desc)
#endif

//Add here all your imgui forwarding functions. Add implementation in ImguiBuilder.cpp
//IMGUI_FN(python name, c++ name, documentation)
IMGUI_FN(begin, begin,R"(
    Begin method, starts the scope of a ImGUI interface. This method must have a corresponding end at some point denoting 
    denoting the scope of all functions required.

    Parameters:
        name (str): Mandatory name of this interface panel.
        is_open (bool)(optional): Specifies the previous state of this panel. True for open, false otherwise.
                                  Update this state with the return value of this function.

        is_fullscreen (bool)(optional): sets this window to follow the main viewport.
        flags (int)(optional): See ImguiFlags.window_* flags

    Returns:
        returns a boolean, True if the X for closing this panel was pressed, False otherwise.
)")

IMGUI_FN(end, end, R"(
    End method. Must be paired with a corresponding begin method.
)")

IMGUI_FN(push_id, pushId, R"(
    Pushes a custom id name for a scope. 
    See (docs/FAQ.md or http://dearimgui.org/faq) for more details about how ID are handled in dear imgui.
    This call to push_id must be paired with a corresponding pop_id call.

    Parameters:
        text (str): The identifier for this id stack scope.
)")

IMGUI_FN(pop_id, popId, R"(
    Pops a corresponding name for a scope. Must be paired with a push_id.
)")

IMGUI_FN(begin_main_menu_bar, beginMainMenuBar, R"(
    Starts the scope of a main menu at the top of the window. Use the MainMenu and BeginMenu/EndMenu functions to add / remove components.
    Must be paired with a end_main_menu_bar method.

    Returns:
        if returns true, call end_main_menu_bar. If returns false means its not rendered.
)")

IMGUI_FN(end_main_menu_bar, endMainMenuBar, R"(
    End method for main menu bar. Must be paired with a begin_main_menu_bar method.
    
    Only call this if begin_main_menu_bar returns true.
)")

IMGUI_FN(begin_menu, beginMenu, R"(
    Starts a menu item in the menu bar. Must be paird with an end_menu method.
    Parameters:
        label (str): name of this menu item. Can recursively have more menu items.

    Returns:
        if returns true, call end_menu. If returns false means its not rendered.
)")

IMGUI_FN(menu_item, menuItem, R"(
    Parameters:
        label (str) : label of this menu item.
        shortcut (str): short cut key for this menu item.
        enabled (bool) : enables / disables menu item.

    Returns:
        True if activated. False otherwise.
)")

IMGUI_FN(end_menu, endMenu, R"(
    End method for menu item. Must be paired with a begin_menu method.
    Only call this if begin_menu returns true.
)")

IMGUI_FN(collapsing_header, collapsingHeader, R"(
    Returns the state of a header label (collapsed or non collapsed).

    Parameters:
        (label) str : label of this item.

    Returns:
        True if open, False otherwise.
)")

IMGUI_FN(show_demo_window, showDemoWindow, R"(
    Shows the demo window.
)")

IMGUI_FN(slider_float, sliderFloat, R"(
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

IMGUI_FN(input_float, inputFloat, R"(
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

IMGUI_FN(input_float3, inputFloat3, R"(
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

IMGUI_FN(input_float3, inputFloat3, R"(
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

IMGUI_FN(input_float4, inputFloat4, R"(
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

IMGUI_FN(input_text, inputText, R"(
    Draws a box for input text.
    
    Parameters:
        label (str): the label name for this box.
        v (str): the actual string value to display by default.

    Returns:
        The new str value that the user set. Feed this value back on v on the next call to see a proper state update.
)")

IMGUI_FN(text, text, R"(
    Draws a box with solid text.

    Parameters:
        text (str): the label value to display
)")

IMGUI_FN(button, button, R"(
    Draws a button.

    Parameters:
        label (str): the label name for this button.

    Returns:
        True if button was pressed, false otherwise.
)")

IMGUI_FN(checkbox, checkbox, R"(
    Draws a checkbox ui item.

    Parameters:
        label (str): the label name for this checkbox.
        v (bool): The current state of the checkbox

    Returns:
        The new bool value that the user set. Feed this value back on v on the next call to see a proper state update.
)")

IMGUI_FN(begin_combo, beginCombo, R"(
    begins a combo box. Must be paired with an end_combo call.
    To insert items, use selectable() between begin_combo/end_combo

    Parameters:
        label (str): the label name for this comboxbox.
        preview_value (str): preview string value

    Returns:
        If value returned is true, must call end_combo.
)")

IMGUI_FN(end_combo, endCombo, R"(
    Ends a combo box. Must be paired with an begin_combo call only if it returns true..
)")

IMGUI_FN(selectable, selectable, R"(
    Draws a selectable item. 

    Parameters:
        label (str): the label name for this selectable.
        selected (str): the state if this value is selected. 

    Returns:
        True if item is selected, false otherwise.
)")

IMGUI_FN(separator, separator, R"(
    Draw a separator.
)")

IMGUI_FN(same_line, sameLine, R"(
    Sets the same line for drawing next item.

    Parameters:
        offset_from_start_x (float)(optional): offset from the start of this x
        spacing (float)(optional): spacing amount
)")

IMGUI_FN(new_line, newLine, R"(
    Draws new line.
)")

IMGUI_FN(image, image, R"(
    Draws an image

    Parameters:
        texture (Texture): coalpy.gpu.Texture object to draw in the UI.
        size (tuple): size of the image in pixels
        uv0 (tuple)(optional): uv coordinate of bottom (default (0.0, 0.0))
        uv1 (tuple)(optional): uv coordinate of top  (default (1.0, 1.0))
        tint_col (tuple): size 4 tuple with image tint color (default (1,1,1,1)).
        border_col (tuple): size 4 tuple with background color (default (0,0,0,0)).
)")

IMGUI_FN(dockspace, dockspace, R"(
    Creates an explicit dockspace

    Parameters:
        label (str): the docking space name
)")

IMGUI_ENUM(window_no_title_bar,               ImGuiWindowFlags_NoTitleBar                , R"(Disable title-bar)")
IMGUI_ENUM(window_no_resize,                  ImGuiWindowFlags_NoResize                  , R"(Disable user resizing with the lower-right grip)")
IMGUI_ENUM(window_no_move,                    ImGuiWindowFlags_NoMove                    , R"(Disable user moving the window)")
IMGUI_ENUM(window_no_scrollbar,               ImGuiWindowFlags_NoScrollbar               , R"(Disable scrollbars (window can still scroll with mouse or programmatically))")
IMGUI_ENUM(window_no_scrll_with_mouse,        ImGuiWindowFlags_NoScrollWithMouse         , R"(Disable user vertically scrolling with mouse wheel. On child window, mouse wheel will be forwarded to the parent unless NoScrollbar is also set.)")
IMGUI_ENUM(window_no_collapse,                ImGuiWindowFlags_NoCollapse                , R"(Disable user collapsing window by double-clicking on it. Also referred to as Window Menu Button (e.g. within a docking node).)")
IMGUI_ENUM(window_always_auto_resize,         ImGuiWindowFlags_AlwaysAutoResize          , R"(Resize every window to its content every frame)")
IMGUI_ENUM(window_no_background,              ImGuiWindowFlags_NoBackground              , R"(Disable drawing background color (WindowBg, etc.) and outside border. Similar as using SetNextWindowBgAlpha(0.0f).)")
IMGUI_ENUM(window_no_saved_settings,          ImGuiWindowFlags_NoSavedSettings           , R"(Never load/save settings in .ini file)")
IMGUI_ENUM(window_no_mouse_inputs,            ImGuiWindowFlags_NoMouseInputs             , R"(Disable catching mouse, hovering test with pass through.)")
IMGUI_ENUM(window_menu_bar,                   ImGuiWindowFlags_MenuBar                   , R"(Has a menu-bar)")
IMGUI_ENUM(window_horizontal_scrollbar,       ImGuiWindowFlags_HorizontalScrollbar       , R"(Allow horizontal scrollbar to appear (off by default). You may use SetNextWindowContentSize(ImVec2(width,0.0f)); prior to calling Begin() to specify width. Read code in imgui_demo in the "Horizontal Scrolling" section.)")
IMGUI_ENUM(window_no_focus_on_appearing,      ImGuiWindowFlags_NoFocusOnAppearing        , R"(Disable taking focus when transitioning from hidden to visible state)")
IMGUI_ENUM(window_no_bring_to_front_on_focus, ImGuiWindowFlags_NoBringToFrontOnFocus     , R"(Disable bringing window to front when taking focus (e.g. clicking on it or programmatically giving it focus))")
IMGUI_ENUM(window_always_vertical_scrollbar,  ImGuiWindowFlags_AlwaysVerticalScrollbar   , R"(Always show vertical scrollbar (even if ContentSize.y < Size.y))")
IMGUI_ENUM(window_always_horizontal_scrollbar,ImGuiWindowFlags_AlwaysHorizontalScrollbar , R"(Always show horizontal scrollbar (even if ContentSize.x < Size.x))")
IMGUI_ENUM(window_always_use_window_padding,  ImGuiWindowFlags_AlwaysUseWindowPadding    , R"(Ensure child windows without border uses style.WindowPadding (ignored by default for non-bordered child windows, because more convenient))")
IMGUI_ENUM(window_no_nav_inputs,              ImGuiWindowFlags_NoNavInputs               , R"(No gamepad/keyboard navigation within the window)")
IMGUI_ENUM(window_no_nav_focus,               ImGuiWindowFlags_NoNavFocus                , R"(No focusing toward this window with gamepad/keyboard navigation (e.g. skipped by CTRL+TAB))")
IMGUI_ENUM(window_unsaved_document,           ImGuiWindowFlags_UnsavedDocument           , R"(Display a dot next to the title. When used in a tab/docking context, tab is selected when clicking the X + closure is not assumed (will wait for user to stop submitting the tab). Otherwise closure is assumed when pressing the X, so if you keep submitting the tab may reappear at end of tab bar.)")
IMGUI_ENUM(window_no_docking,                 ImGuiWindowFlags_NoDocking                 , R"(Disable docking of this window)")

#undef IMGUI_ENUM
#undef IMGUI_FN 
