// window_fx_shader.hlsl

// Declare the output texture we want to write information to
RWTexture2D<float4> g_output : register(u0);

// Simple compute kernel code
[numthreads(8, 8, 1)]
void csMainWindow(int3 dispatchThreadID : SV_DispatchThreadID)
{
    g_output[dispatchThreadID.xy] = float4(0.5, 0, 0.2, 1.0);
}
