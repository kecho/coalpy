
RWTexture2D<float4> g_output : register(u0);

cbuffer Constants : register(b0)
{
    float4 g_constantColor;
};

[numthreads(8,8,1)]
void csMain(int3 dti : SV_DispatchThreadID)
{
    //g_output[dti.xy] = (dti.x & 0x1) ^ (dti.y & 0x1) ? float4(1,1,1,1) : float4(0,0,0,0);
    //g_output[dti.xy] = float4(1,0,0,1);
    g_output[dti.xy] = g_constantColor;
}
