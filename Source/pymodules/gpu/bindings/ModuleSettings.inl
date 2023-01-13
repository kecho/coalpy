COALPY_FN(save, saveSettings, R"(
    Saves settings to a file.

    Parameters:
        filename (optional): optional name of the device.
            If not used, default settings are saved in ".coalpy_settings.json"
            which is the default settings read at module boot.
    Returns:
        True if successful, False if it failed.
)")

#undef COALPY_FN
