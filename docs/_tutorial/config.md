---
permalink: /tutorial-config
title: "Configuring _CoalPy_"
layout: "single"
toc: true
sidebar:
  nav: "tutorialbar"
---

This section will show more advanced configuration of __CoalPy__. We will see how to list and pick a graphics card, how to configure the backend, how to enable debug information and more interesting things.

## Initialization

__CoalPy__ has two initialization steps. Soft and Hard.
* **Soft**: when the library is imported. The only operations allowed after soft initialization are querying the GPUs and modifying the module settings.
* **Hard**: when the library instantiates the internal device of the graphics APIs. This can be done by calling init() explicitely or by just doing it implicitely on the first usage of the API, such as scheduling a command list, creating a shader or texture.

**Soft** initialization executes immediately after importing __CoalPy__

```python
>>> import coalpy.gpu
```

A hard initialization of CoalPy can be done explicitely by calling the [_init()_](apidocs/0.50/coalpy.gpu.html#init) function directly

```python
>>> coalpy.gpu.init()
```

**Hard** initialization can also occur implicitly if we use the API in any way that requries it (lazy initialization). 
Use an explicit hard initialization if you'd like to configure CoalPy further before starting your app (for example pick a different backend like _Vulkan_ instead of _DirectX12_)

A good usage for **Hard** initialization is to first query the GPUs, and based on some setting in the APP, maybe prefer a certain vendor. This code can run in a module's \_\_init\_\_ function.
After this, the application can call init() or simply just let the first execution of an API function to implicitely call init() internally.

## Querying Available GPUs

In CoalPy we can query all the available GPUs by calling the [_get_adapters()_](apidocs/0.50/coalpy.gpu.html#get_adapters) function.

```python
>>> coalpy.gpu.get_adapters()
[(0, 'NVIDIA GeForce RTX 2070 SUPER'), (1, 'Intel(R) UHD Graphics 630'), (2, 'Microsoft Basic Render Driver')]
```
**NOTE: This function won't cause a hard initialization.**

This function will return you a list of tuples. Each tuple has the index of the GPU and a name describing it.
You can store this list on an array during your module initialization, and use the index to explicitely pick a GPU.

## Configuring CoalPy

Before a hard initialization, you can get a coalpy settings object via the [get_settings](apidocs/0.50/coalpy.gpu.html#get_settings) function.

```python
>>> settings_obj = coalpy.gpu.get_settings()
>>> settings_obj.enable_debug_device = True # enables debug device for error messages
>>> settings_obj.save("my_settings.json") # saves the settings to a file
>>> settings_obj.load("my_settings.json") # loads the settings from a file
>>> coalpy.gpu.init() # Hard initializes! 
```
The coalpy settings object cannot be instantiated, is a singleton object and it can be serialized / deserialized.
When the application does **Hard** initialization, this object is read and the necessary settings will apply.

Settings that can be set through this object include:
* adapter_index: the graphics card to use
* graphics_api: Either "Dx12" or "Vulkan". This will be the internal backed used by __CoalPy__
* shader_model: The _hlsl_ shader model feature set.

For a full list of the settings available please see the [coalpy.gpu.Settings](apidocs/0.50/coalpy.gpu.html#Settings) type.
