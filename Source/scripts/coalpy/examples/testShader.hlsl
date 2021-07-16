#include <examples/testInclude.hlsl>

RWTexture2D<float4> output : register(u0);
[numthreads(8,4,1)]
void main(int3 dti : SV_DispatchThreadID)
{
    output[dti.xy] = float4((float)dti.x/480.0,(float)dti.y/480.0,0,0);
}
