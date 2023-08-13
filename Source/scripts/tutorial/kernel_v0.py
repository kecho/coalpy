import numpy
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

# Create a buffer of type int. 
g_output_buffer = coalpy.gpu.Buffer(format = coalpy.gpu.Format.R32_SINT, element_count=64)

# create a command list
cmd_list = coalpy.gpu.CommandList()

# add a single dispatch
cmd_list.dispatch(shader=g_shader, outputs=g_output_buffer, x=1, y=1, z=1)

# execute
coalpy.gpu.schedule(cmd_list)

# create and launch a download request on the output
download_request = coalpy.gpu.ResourceDownloadRequest(resource=g_output_buffer)

# this blocks the CPU to wait for results. 
# Alternative the user can poll the download request using the is_ready function
download_request.resolve() 

results = numpy.frombuffer(download_request.data_as_bytearray(), dtype=int)
print(results)



