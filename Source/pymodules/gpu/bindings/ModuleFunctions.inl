//Main coalpy module gpu bindings.

COALPY_FN(get_adapters, getAdapters,
    R"(
    Lists all gpu graphics cards found in the system.

    Returns:
        adapters (Array of tuple): Each element in the list is a tuple of type (i : int, name : Str)
    )"
)

COALPY_FN(get_current_adapter_info, getCurrentAdapterInfo,
    R"(
    Gets the currently active GPU adapter, as a tuple of (i : int, name : Str).

    Returns:
        adapter (tuple): Element as tuple of type (i : int, name : str)
    )"
)

COALPY_FN(set_current_adapter, setCurrentAdapter,
    R"(
    Selects an active GPU adapter. For a list of GPU adapters see coalpy.gpu.get_adapters.

    Parameters:
        index (int): the desired device index. See get_adapters for a full list.
        flags (gpu.DeviceFlags) (optional): bit mask with device flags.
        dump_shader_pdbs (bool)(optional): if True it will dump the built shader's PDBs to the .shader_pdb folder.
        shader_model (int)(optional):  Default is sm6_5. sets the current shader model used for shader compilation. See the enum coalpy.gpu.ShaderModel for the available models.
    )"
)

COALPY_FN(add_data_path, addDataPath,
    R"(
    Adds a path to load shader / texture files / shader includes from.
    
    Parameters:
        path (str or Module):  If str it will be a path to be used. If a module, it will find the source folder of the module's __init__.py file, and use this source as the path.
                               path priorities are based on the order of how they were added.
    )"
)

COALPY_FN(schedule, schedule,
    R"(
    Submits an array of CommandList objects to the GPU to run shader work on.

    Parameters:
        command_lists (array of CommandList or a single CommandList object): an array of CommandList objects or a single CommandList object to run in the GPU. CommandList can be resubmitted through more calls of schedule.
    )"
)

COALPY_FN(begin_collect_markers, beginCollectMarkers,
    R"(
    Call this surrounding all your schedules to define a 'frame'.
    Once you call end_collect_markers you can extract the results for further processing.

    gpu.begin_collect_markers()
    ...
    gpu.schedule([]) #perform all your schedules
    marker_data = gpu.end_collect_markers()

    Parameters:
        max_query_bytes (int)(optional): default value 56kb. Maximum amount of marker data to obtain.   
    )"
)

COALPY_FN(end_collect_markers, endCollectMarkers,
    R"(
    Ends marker collection. See gpu.begin_collect_markers

    Returns:
        marker_results (see g.MarkerResults)
    )"
)

COALPY_FN(run, run, "Runs window rendering callbacks. This function blocks until all the existing windows are closed. Window objects must be created and referenced prior. Use the Window object to configure / specify callbacks and this function to run all the event loops for windows.")

#undef COALPY_FN
