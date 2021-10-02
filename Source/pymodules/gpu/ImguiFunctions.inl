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

IMGUI_FN(begin_main_menu_bar, beginMainMenuBar, "")
IMGUI_FN(end_main_menu_bar, endMainMenuBar, "")
IMGUI_FN(begin_menu, beginMenu, "")
IMGUI_FN(menu_item, menuItem, "")
IMGUI_FN(end_menu, endMenu, "")
IMGUI_FN(collapsing_header, collapsingHeader, "")
IMGUI_FN(show_demo_window, showDemoWindow, "")
IMGUI_FN(slider_float, sliderFloat, "")
IMGUI_FN(input_float, inputFloat, "")
IMGUI_FN(input_text, inputText, "")
IMGUI_FN(text, text, "")
IMGUI_FN(button, button, "")
IMGUI_FN(checkbox, checkbox, "")
