RWTexture2D<float4> g_output : register(u0);
//RWTexture2D<float4> g_output1 : register(u1);
//RWTexture2D<float4> g_output2 : register(u2);

cbuffer Constants : register(b0)
{
    float4 g_constantColor;
};

[numthreads(8,8,1)]
void csMain(int3 dti : SV_DispatchThreadID)
{
    g_output[dti.xy] = g_constantColor;
    //g_output2[dti.xy] = float4(1,0,0,0);
}
