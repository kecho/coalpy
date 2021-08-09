#include <examples/testInclude.hlsl>

cbuffer Constants : register(b0)
{
    float gTime;
    float invWidth;
    float invHeight;
    float unused;
}

float3 rgb2sRgb(float3 RGB)
{
    float3 S1 = sqrt(RGB);
    float3 S2 = sqrt(S1);
    float3 S3 = sqrt(S2);
    float3 sRGB = 0.662002687 * S1 + 0.684122060 * S2 - 0.323583601 * S3 - 0.0225411470 * RGB;
    return sRGB;
}

Texture2D<float4> input : register(t0);
RWTexture2D<float4> output : register(u0);
[numthreads(8,4,1)]
void main(int3 dti : SV_DispatchThreadID)
{
    float2 uv = (dti.xy + 0.5) * float2(invWidth, invHeight);
    float c = sin(0.001*gTime + 200.0*uv.x + 30*cos(1.0*(sin(0.007*gTime + 20.0 * uv.y))));
    output[dti.xy] = c.xxxx;
    //output[dti.xy] = float4(uv.y < 0.5 ? uv.xxx : (uv.x < 0.5 ? float3(1,0,0) : float3(0,0,1)), 1.0);//float4(rgb2sRgb(input[dti.xy].rgb), 1.0);//c.xxxx;
    //output[dti.xy] = float4(input[dti.xy].rgb, 1.0);
}
