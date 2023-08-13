COALPY_FN(save, saveSettings, R"(
    Saves settings to a json file.

    Parameters:
        filename (optional): optional name of the file. Ensure to include the .json extension if specified.
            If not used, default settings are saved in ".coalpy_settings.json"
            which is the default settings read at module boot.
    Returns:
        True if successful, False if it failed.
)")

COALPY_FN(load, loadSettings, R"(
    Load settings from a json file.

    Parameters:
        filename: name of the file to load.
    Returns:
        True if successful, False if it failed.
)")

#undef COALPY_FN
