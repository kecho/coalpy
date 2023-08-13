---
permalink: /tutorial-window
title: "Creating a simple Window with _CoalPy_"
layout: "single"
toc: true
sidebar:
  nav: "tutorialbar"
---

To visualize any realtime graphics work in _CoalPy_, we have to create a window.
The following script creates a window, and is the minimal application to display something to the screen:

```python
# import the library
import coalpy.gpu

# define a simple rendering function (do nothing)
def render_fn(render_args : coalpy.gpu.RenderArgs):
    return

# create a window
w = coalpy.gpu.Window(title = "Tutorial", width = 400, height = 400, on_render = render_fn)

# do the window loop. This blocks, and unless w is set to None or the user hits the close button, it will continue
coalpy.gpu.run()

```

You can extract many properties of the window and the state of the application via [render_args](apidocs/0.50/coalpy.gpu.html#RenderArgs)

The final result of the window should look like this:

![image](images/step0-window.png)

Save this script as __window_fx.py__

A version of this script can be found [here](https://github.com/kecho/coalpy/blob/master/Source/scripts/tutorial/window_fx_v0.py)

## Create a Simple Shader

Create a new text file and call it __window_fx_shader.hlsl__. This will be our first compute shader file containing some kernels that will draw to the window.
First, lets write an empty compute shader:

```hlsl
// window_fx_shader.hlsl

// Declare the output texture we want to write information to
RWTexture<float4> g_output : register(u0);

// Simple compute kernel code
// we dispatch on thread groups of 8 by 8
[numthreads(8, 8, 1)]
void csMainWindow(int3 dispatchThreadID : SV_DispatchThreadID)
{
    g_output[dispatchThreadID.xy] = float4(0.5, 0, 0.2, 1.0);
}

```
A version of this shader can be found [here](https://github.com/kecho/coalpy/blob/master/Source/scripts/tutorial/window_fx_shader_v0.hlsl)

This shader will write uniformly for every pixel (accessed by the dispatchThreadID) the color __(0.5, 0, 0.2, 1.0)__
The next step is to load this shader in our tutorial. At the top of the __window_fx.py__ script, add the following lines:

```python
g_shader = coalpy.gpu.Shader(
    file="window_fx_shader.hlsl",
    name="WindowFx",
    main_function="csMainWindow")
```

## Create a command list and submit it

Go back to the script __window_fx.py__ and change the render_fn to look like this:

```python
import math # make sure to import math for ceil function

def render_fn(render_args : coalpy.gpu.RenderArgs):
    # create a new command list object
    cmd = coalpy.gpu.CommandList()

    # add a dispatch command
    cmd.dispatch(
        # the shader
        shader = g_shader, 

        # the output is the window's display texture
        outputs = render_args.window.display_texture,

        # we dispatch on thread groups of 8 by 8.
        x = math.ceil(render_args.width/8),
        y = math.ceil(render_args.height/8),
        z = 1) # z = 1 since there are no threads on z axis

    # submit command list to the GPU for execution
    coalpy.gpu.schedule(cmd)

```

A version of this script can be found [here](https://github.com/kecho/coalpy/blob/master/Source/scripts/tutorial/window_fx_v1.py)

More information about 

Now if you run your script, you should expect some color in your image!

![image](images/step1-window.png)
