##Append consume buffer example.
import coalpy.gpu as g
import numpy as np

shader_obj = g.Shader(
    source_code = """
        AppendStructuredBuffer<int> g_appendBuffer : register(u0);
    
        [numthreads(64, 1, 1)]
        void csMain(int3 dti : SV_DispatchThreadID)
        {
            g_appendBuffer.Append(dti.x);
        }
    """,
    main_function = "csMain")

dest_buffer = g.Buffer(
    type=g.BufferType.Structured,
    format=g.Format.R32_UINT,
    stride=4, element_count=64, is_append_consume = True)

cmdlist = g.CommandList()
cmdlist.clear_append_consume_counter(dest_buffer)
cmdlist.dispatch(
    x = 1, y = 1, z = 1,
    outputs = dest_buffer,
    shader = shader_obj)

print ("Appending valus 1...64")
g.schedule(cmdlist)

print ("Downloading to CPU")
download_request = g.ResourceDownloadRequest(dest_buffer)
download_request.resolve()
result_array = np.frombuffer(download_request.data_as_bytearray(), dtype='i')
print ("Result (sorted): " + str(np.sort(result_array)))


