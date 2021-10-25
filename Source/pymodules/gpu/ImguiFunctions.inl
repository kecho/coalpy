//Add here all your imgui forwarding functions. Add implementation in ImguiBuilder.cpp
//IMGUI_FN(python name, c++ name, documentation)
IMGUI_FN(begin, begin,R"(
    Begin method, starts the scope of a ImGUI interface. This method must have a corresponding end at some point denoting 
    denoting the scope of all functions required.

    Parameters:
        name (str): Mandatory name of this interface panel.
        is_open (bool)(optional): Specifies the previous state of this panel. True for open, false otherwise.
                                  Update this state with the return value of this function.

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
