---
permalink: /tutorial-kernel
title: "Creating a simple Kernel with _CoalPy_"
layout: "single"
toc: true
sidebar:
  nav: "tutorialbar"
---

## Installation

There are multiple ways to install __CoalPy__. The easiest is through python PiP package manager:

```
pip install coalpy
```

Alternatively, you can download the prebuilt binaries in the release links of the [github repository here](https://github.com/kecho/coalpy/releases)

## Test it works!
 
To test it works on your machine, run the following python module, which is an example of a GPU application built using coalpy:

```
py coalpy.examples.seascape 
```

## A first kernel

Start a new script, lets call it _simple_kernel_test.py_. At the top of this script, we will write our first _hlsl_ compute kernel and store it in a string variable. It will look like this:

```python

kernel_source = """

RWBuffer<int> g_output : register(u0);

[numthreads(64,1,1)]
void csMainKernel(int3 dispatchThreadID : SV_DispatchThreadID)
{
    g_output[dispatchThreadID.x] = dispatchThreadID.x;
}

"""

```

There are a few things to unpack here. The first one is that our _hlsl_ kernel source will be embedded in a string which __CoalPy__ will later be able to execute.
Inside this kernel source the first line indicates we will write to a int buffer:

```hlsl
RWBuffer<int> g_output : register(u0);
```

At the top of the _csMainKernel_ function we see a _numthreads_ attribute. This special attribute tells us that each thread group will be composed of 64 threads per group.
The thread group can be thought as a 3d cube of threads, in this case the layout of a group is 64x1x1 cube. 

The input argument to the kernel _dispatchThreadID_ which indicates the current coordinate of the thread. Since we decided to use a group of 64x1x1 we will only be using the x coordinate.

This coordinate is used as the output and we will then just write the thread id to the output buffer.


## Setting up the shader

Now that we understand what our kernel is going to do, we will create a shader using the source code specified above.
To do this, we create a [Shader](apidocs/0.50/coalpy.gpu.html#Shader) object as follows:

```python
import coalpy.gpu

kernel_source = """
RWBuffer<int> g_output : register(u0);

[numthreads(64,1,1)]
void csMainKernel(int3 dispatchThreadID : SV_DispatchThreadID)
{
    g_output[dispatchThreadID.x] = dispatchThreadID.x;
}
"""

# Create a shader.
# Note: The shader source can also be in a separate file of extension hlsl. To do this use the file argument instead of source_code.
# This is preferable as we will see later that it allows for live editing of the code.
g_shader = coalpy.gpu.Shader(source_code=kernel_source, main_function="csMainKernel")

```

Now we have a shader that is ready to be used.

## Setting up the output buffer

Our little shader is not useful unless we have an output to write it to. For this we have to create in our little script a simple Buffer of type INT32.
The [Buffer](apidocs/0.50/coalpy.gpu.html#Buffer) type contains the necessary tools to create data that can be filled in the GPU.

```python

# Create a buffer of type int. 
g_output_buffer = coalpy.gpu.Buffer(format = coalpy.gpu.Format.R32_SINT, element_count=64)

```

This buffer object represents a GPU resource. It is of type int and it has an element count of 64.


## Execute the kernel in the GPU

Now that we have a shader, a buffer, is time to send a command to the GPU and tell it to execute our kernel on this buffer.

To do this we will create the following command:

```python

# create a command list
cmd_list = coalpy.gpu.CommandList()

# add a single dispatch
cmd_list.dispatch(shader=g_shader, outputs=g_output_buffer, x=1, y=1, z=1)

# execute
coalpy.gpu.schedule(cmd_list)

```

The code above fills in a command list. This command list only executes a single thread group (the x, y, z parameters are the dimensions of the thread group).
Because the internal shader has defined a threadgroup of size 64x1x1, and our output is just a flat buffer of 64, we just launch a single thread group.

The [CommandList](apidocs/0.50/coalpy.gpu.html#CommandList) type contains more commands that can be appended.

The function [schedule](apidocs/0.50/coalpy.gpu.html#schedule) can take an array of commands or a single command, and it will asynchroneously put them on a queue for the GPU to collect.
The GPU will pick up the command list filled in and execute anything else required.

## Read the results back

Now that we have launched the kernel and we are indeed writting to this resource, we have to download the results to the GPU.
__CoalPy__ does not try to hide this operation by convenience. The reason is performance. It is my belief that the user must be aware that a GPU readback is an expensive operation
and should only be done if necessary. In this simple example we are reading just a little bit of memory. But other examples might require doing larger readbacks.

To perform a blocking readback (that is wait until the results are ready) we add this piece of code to our script:

```python
import numpy

...

download_request = coalpy.gpu.ResourceDownloadRequest(resource=g_output_buffer)
# this blocks the CPU to wait for results. 
# Alternative the user can poll the download request using the is_ready function
download_request.resolve() 

results = numpy.frombuffer(download_request.data_as_bytearray(), dtype=int)
print(results)

```
[The final script can be found here](https://github.com/kecho/coalpy/blob/master/Source/scripts/tutorial/kernel_v0.py)

Now running this script will show an array of 64 elements computed in the GPU!

```
> py kernel_v0.py
[ 0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23
 24 25 26 27 28 29 30 31 32 33 34 35 36 37 38 39 40 41 42 43 44 45 46 47
 48 49 50 51 52 53 54 55 56 57 58 59 60 61 62 63]
```

