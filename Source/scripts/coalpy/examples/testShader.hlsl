#include <examples/testInclude.hlsl>

cbuffer Constants : register(b0)
{
    int4 vals;
    float4 vals2;
}

RWTexture2D<float4> output : register(u0);
[numthreads(8,4,1)]
void main(int3 dti : SV_DispatchThreadID)
{
    output[dti.xy] = vals2.x < 0.7 ? float4(1,0,0,1) : float4(0,0,1,1);
}
