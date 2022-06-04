COALPY_FN(get_size, getSize, R"(
    Returns:
        returns a touple with the current size of the window (width, height)
)")

COALPY_FN(get_key_state, getKeyState, R"(
    Gets the pressed state of a queried key. For a list of keys see coalpy.gpu.Keys.

    Parameters:
        key (int enum): The key (keyboard or mouse) to query info from. Use the coalpy.gpu.Keys enumerations to query a valid key.

    Returns:
        True if the queried key is pressed. False otherwise.
)")

COALPY_FN(get_mouse_position, getMousePosition, R"(
    Gets the mouse position relative to the current window. This function will also give you the mouse coordinates even if the mouse is outside of the window.

    Returns:
        tuple with: (pixelX, pixelY, normalizedX, normalizedY)

        The pixelX and pixelY represent the pixel index (0 based to get_size()).
        The normalizedX and normalizedY are the pixel centered coordinates in x and y [0, 1] respectively, i.e.  (x + 0.5)/ (window's width).
)")

#undef COALPY_FN
