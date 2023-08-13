---
permalink: /tutorial-effect
title: "Simple Effect with _CoalPy_"
layout: "single"
toc: true
sidebar:
  nav: "tutorialbar"
---

Once the [getting started](tutorial-start) tutorial is finished, we can now add more interesting effects to our window effect application.
We will also add a simple editor user interface to begin with the Imgui integration.

# Add a color picker UI

Open the script __window_fx.py__ from the previous tutorial section, and add the following state at the top:

```python

g_color = [0.1, 0.0, 0.93, 1.0]

```

This will be the global color of the window effect that we will be implementing.
We will now create a new function that will write to this global color variable.

```python
def draw_imgui(imgui : coalpy.gpu.ImguiBuilder):
    # this is so we can write to the global g_color
    global g_color

    #begin panel
    imgui.begin("Options")

    # color picker
    col = imgui.color_edit4("effect color 0", g_color)

    # only write color if it was picked
    if col is not None:
        g_color = list(col)

    #end panel
    imgui.end()

```

This function now needs to be call at the top of the __render_fn__ as follows:

```python
def render_fn(render_args : coalpy.gpu.RenderArgs):
    global g_color # so we can use the global in this function

    # draw user interface
    draw_imgui(render_args.imgui)

    ...
```

We will now pass the color as an input constant into our shader. 
Add the __constant__ argument to the dispatch call and pass the g_color as follows:

```python

    # add a dispatch command
    cmd.dispatch(
        constants = g_color, #pass the color directly as an input
        shader = g_shader,

    ...
    
```
A version of this script can be found [here](https://github.com/kecho/coalpy/blob/master/Source/scripts/tutorial/window_fx_v2.py)

Finally, lets open our compute shader __window_fx_shader.hlsl__ and replace the hard coded color with a constant value:

```hlsl
// window_fx_shader.hlsl

cbuffer Constants : register(b0)
{
    float4 g_color;
} 

// Declare the output texture we want to write information to
RWTexture2D<float4> g_output : register(u0);

// Simple compute kernel code
[numthreads(8, 8, 1)]
void csMainWindow(int3 dispatchThreadID : SV_DispatchThreadID)
{
    g_output[dispatchThreadID.xy] = g_color;
}
```
A version of this shader can be found [here](https://github.com/kecho/coalpy/blob/master/Source/scripts/tutorial/window_fx_shader_v1.hlsl)

The results should look like this:

![image](images/step2-window.png)

We have now a simple user interface element with color picker and we are updating the color of the window in realtime.
[Documentation for the imgui API can be found here](apidocs/0.50/coalpy.gpu.html#ImguiBuilder). The __ImguiBuilder__ object is always passed down in the RenderArgs and cannot be instantiated outside a window.
Use this object as the main entry point of the imgui API.

