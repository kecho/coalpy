#include <examples/testInclude.hlsl>

cbuffer Constants : register(b0)
{
    float gTime;
    float invWidth;
    float invHeight;
    float unused;
}

RWTexture2D<float4> output : register(u0);
[numthreads(8,4,1)]
void main(int3 dti : SV_DispatchThreadID)
{

    float2 uv = (dti.xy + 0.5) * float2(invWidth, invHeight);
    float c = sin(0.001*gTime + 200.0*uv.x + 30*cos(1.0*(sin(0.007*gTime + 20.0 * uv.y))));
    output[dti.xy] = c.xxxx;
}
