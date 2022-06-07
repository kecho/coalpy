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

COALPY_FN(image_button, imageButton, R"(
    Draws an image button

    Parameters:
        texture (Texture): coalpy.gpu.Texture object to draw in the UI.
        size (tuple): size of the image in pixels
        uv0 (tuple)(optional): uv coordinate of bottom (default (0.0, 0.0))
        uv1 (tuple)(optional): uv coordinate of top  (default (1.0, 1.0))
        frame_padding (int)(optional): default -1. < 0 uses from style (default), 0 no framing, >0 set framing size
        bg_col (tuple)(optional): size 4 tuple with image tint color (default (1,1,1,1)).
        tint_col (tuple)(optional): size 4 tuple with background color (default (0,0,0,0)).

    Return:
        True if button is pressed, false otherwise
)")

COALPY_FN(get_item_rect_size, getItemRectSize, R"(
    Get prev items rec size
    
    Return:
        size (tuple 2 float)
)")

COALPY_FN(get_window_content_region_min, getWindowContentRegionMin, R"(
    Get the remaining content region

    Return:
        region min (tuple 2 float)
)")

COALPY_FN(get_window_content_region_max, getWindowContentRegionMax, R"(
    Get the remaining content region

    Return:
        region min (tuple 2 float)
)")

COALPY_FN(set_tooltip, setTooltip, R"(
    Sets tooltip of prev item

    Parameters:
        text (str) text of tooltip
)")

COALPY_FN(is_item_hovered, isItemHovered, R"(
    flags (int): see coalpy.gpu.ImGuiHoveredFlags

    Returns:
        True if is hovered, false otherwise
 )")

COALPY_FN(get_id, getID, R"(
    Parameters:
        name (str)
    Returns:
        int (id)
)")

COALPY_FN(settings_loaded, settingsLoaded, R"(
    Returns:
        True if the .ini settings have been loaded, False otherwise
)")

COALPY_FN(get_mouse_pos, getMousePos, R"(
    Gets the absolute mouse position

    Returns:
        Mouse pos (tuple x, y)
)")

COALPY_FN(get_window_pos, getWindowPos, R"(
    Gets the absolute window position

    Returns
        Window position (tuple x, y)
)")

COALPY_FN(get_window_size, getWindowSize, R"(
    Gets the current window size

    Returns:
        Window size (tuple x, y)
)")

COALPY_FN(get_window_work_size, getWindowWorkSize, R"(
    Gets the current window size, minus bars and stuff

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

COALPY_FN(set_window_dock, setWindowDock, R"(
    Parameters:
        window_name (str)
        dock_id (int)
)")

COALPY_FN(dockspace, dockspace, R"(
    Creates an explicit dockspace

    Parameters:
        label (str): the docking space name
        dock_id (int)(optional): the dockspace id. If provided, dockspace name will be ignored.

    Returns:
        dockspace_id (int): id of the dockspace used.
)")

COALPY_FN(dockbuilder_dock_window, dockBuilderDockWindow, R"(
    Parameters:
        window_name (str)
        node_id (int)
)")

COALPY_FN(dockbuilder_add_node, dockBuilderAddNode, R"(
    Parameters:
        node_id (int)
        flags (int): see coalpy.gpu.ImGuiDockNodeFlags

    Returns:
        dock_id added.
)")

COALPY_FN(dockbuilder_remove_node, dockBuilderRemoveNode, R"(
    Parameters:
        node_id (int)
)")

COALPY_FN(dockbuilder_remove_child_nodes, dockBuilderRemoveChildNodes, R"(
    Parameters:
        node_id (int)
)")

COALPY_FN(dockbuilder_set_node_pos, dockBuilderSetNodePos, R"(
    Parameters:
        node_id (int)
        pos (tuple 2)
)")

COALPY_FN(dockbuilder_set_node_size, dockBuilderSetNodeSize, R"(
    Parameters:
        node_id (int)
        size (tuple 2)
)")

COALPY_FN(dockbuilder_split_node, dockBuilderSplitNode, R"(
    Parameters:
        node_id (int)
        split_dir (int): see coalpy.gpu.ImGuiDir 
        split_ratio (float)
    
    Returns:
        tuple 2: (dock_id_at_dir, dock_id_opposite_dir)
)")

COALPY_FN(dockbuilder_finish, dockBuilderFinish, R"(
    Parameters:
        node_id (int)
)")

COALPY_FN(dockbuilder_node_exists, dockBuilderNodeExists, R"(
    Tests wether a dockspace exists. Ideal to setup dock layout at init.
    Use an id or a string.

    Parameters:
        label (str) 
        dock_id (int)(optional) - if node_id is used, name will be ignored

    Returns:
        True if it exists, False otherwise
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

//Imgui dir enum
COALPY_ENUM_BEGIN(ImGuiDir, "")
COALPY_ENUM(Left  , ImGuiDir_Left  , "")
COALPY_ENUM(Right , ImGuiDir_Right , "")
COALPY_ENUM(Up    , ImGuiDir_Up    , "")
COALPY_ENUM(Down  , ImGuiDir_Down  , "")
COALPY_ENUM(COUNT , ImGuiDir_COUNT , "")
COALPY_ENUM_END(ImGuiDir)

//Imgui dock enum
COALPY_ENUM_BEGIN(ImGuiDockNodeFlags, "Flags for docking")
COALPY_ENUM(KeepAliveOnly         , ImGuiDockNodeFlags_KeepAliveOnly         , "Don't display the dockspace node but keep it alive. Windows docked into this dockspace node won't be undocked.")
COALPY_ENUM(NoDockingInCentralNode, ImGuiDockNodeFlags_NoDockingInCentralNode, "Disable docking inside the Central Node, which will be always kept empty.")
COALPY_ENUM(PassthruCentralNode   , ImGuiDockNodeFlags_PassthruCentralNode   , "Enable passthru dockspace: 1) DockSpace() will render a ImGuiCol_WindowBg background covering everything excepted the Central Node when empty. Meaning the host window should probably use SetNextWindowBgAlpha(0.0f) prior to Begin() when using this. 2) When Central Node is empty: let inputs pass-through + won't display a DockingEmptyBg background. See demo for details.")
COALPY_ENUM(NoSplit               , ImGuiDockNodeFlags_NoSplit               , "Disable splitting the node into smaller nodes. Useful e.g. when embedding dockspaces into a main root one (the root one may have splitting disabled to reduce confusion). Note: when turned off, existing splits will be preserved.")
COALPY_ENUM(NoResize              , ImGuiDockNodeFlags_NoResize              , "Disable resizing node using the splitter/separators. Useful with programmatically setup dockspaces.")
COALPY_ENUM(AutoHideTabBar        , ImGuiDockNodeFlags_AutoHideTabBar        , "Tab bar will automatically hide when there is a single window in the dock node.")
COALPY_ENUM_END(ImGuiDockNodeFlags)

//Imgui hovered flags
COALPY_ENUM_BEGIN(ImGuiHoveredFlags, "Flags for IsHovered")
COALPY_ENUM(ChildWindows                  ,ImGuiHoveredFlags_ChildWindows                  ,R"("IsWindowHovered() only: Return true if any children of the window is hovered)")
COALPY_ENUM(RootWindow                    ,ImGuiHoveredFlags_RootWindow                    ,R"(IsWindowHovered() only: Test from root window (top most parent of the current hierarchy))")
COALPY_ENUM(AnyWindow                     ,ImGuiHoveredFlags_AnyWindow                     ,R"(IsWindowHovered() only: Return true if any window is hovered)")
COALPY_ENUM(NoPopupHierarchy              ,ImGuiHoveredFlags_NoPopupHierarchy              ,R"(IsWindowHovered() only: Do not consider popup hierarchy (do not treat popup emitter as parent of popup) (when used with _ChildWindows or _RootWindow))")
COALPY_ENUM(DockHierarchy                 ,ImGuiHoveredFlags_DockHierarchy                 ,R"(IsWindowHovered() only: Consider docking hierarchy (treat dockspace host as parent of docked window) (when used with _ChildWindows or _RootWindow))")
COALPY_ENUM(AllowWhenBlockedByPopup       ,ImGuiHoveredFlags_AllowWhenBlockedByPopup       ,R"(Return true even if a popup window is normally blocking access to this item/window)")
COALPY_ENUM(AllowWhenBlockedByActiveItem  ,ImGuiHoveredFlags_AllowWhenBlockedByActiveItem  ,R"(Return true even if an active item is blocking access to this item/window. Useful for Drag and Drop patterns.)")
COALPY_ENUM(AllowWhenOverlapped           ,ImGuiHoveredFlags_AllowWhenOverlapped           ,R"(IsItemHovered() only: Return true even if the position is obstructed or overlapped by another window)")
COALPY_ENUM(AllowWhenDisabled             ,ImGuiHoveredFlags_AllowWhenDisabled             ,R"(IsItemHovered() only: Return true even if the item is disabled)")
COALPY_ENUM(NoNavOverride                 ,ImGuiHoveredFlags_NoNavOverride                 ,R"(Disable using gamepad/keyboard navigation state when active, always query mouse.)")
COALPY_ENUM(RectOnly                      ,ImGuiHoveredFlags_RectOnly                      ,R"(ImGuiHoveredFlags_AllowWhenBlockedByPopup | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem | ImGuiHoveredFlags_AllowWhenOverlapped)")
COALPY_ENUM(RootAndChildWindows           ,ImGuiHoveredFlags_RootAndChildWindows           ,R"(ImGuiHoveredFlags_RootWindow | ImGuiHoveredFlags_ChildWindows)")
COALPY_ENUM_END(ImGuiHoveredFlags)

#undef COALPY_ENUM_END
#undef COALPY_ENUM_BEGIN
#undef COALPY_ENUM
#undef COALPY_FN 
