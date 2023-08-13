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
