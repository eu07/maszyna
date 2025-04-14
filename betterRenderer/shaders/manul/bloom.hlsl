
Texture2D<float3> g_Source : register(t0);
Texture2D<float3> g_SourceAdd : register(t1);
RWTexture2D<float4> g_Output : register(u0);
SamplerState g_SamplerLinearClamp : register(s0);

cbuffer BloomConstants : register(b0){
    float4 g_PrefilterVector;
    float g_MixFactor;
}

float3 Prefilter(float3 c) {
    float brightness = max(c.r, max(c.g, c.b));
    float soft = brightness - g_PrefilterVector.y;
    soft = clamp(soft, 0, g_PrefilterVector.z);
    soft = soft * soft * g_PrefilterVector.w;
    float contribution = max(soft, brightness - g_PrefilterVector.x);
    contribution /= max(brightness, 0.00001);
    return c * contribution;
}

[numthreads(8, 8, 1)]
void CS_BloomPrefilter(uint3 PixCoord : SV_DispatchThreadID) {
    g_Output[PixCoord.xy].xyz = Prefilter(g_Source[PixCoord.xy]);
}

[numthreads(8, 8, 1)]
void CS_BloomDownsample(uint3 PixCoord : SV_DispatchThreadID) {
    uint2 pixel = PixCoord.xy;
    uint2 source_size;
    g_Source.GetDimensions(source_size.x, source_size.y);
    uint2 output_size;
    g_Output.GetDimensions(output_size.x, output_size.y);
    float2 uv = (pixel + .5) / output_size;
    float2 offset = 1. / source_size;

    float3 a = g_Source.SampleLevel(g_SamplerLinearClamp, uv + offset * float2(-2, 2), 0.);
    float3 b = g_Source.SampleLevel(g_SamplerLinearClamp, uv + offset * float2(0, 2), 0.);
    float3 c = g_Source.SampleLevel(g_SamplerLinearClamp, uv + offset * float2(2, 2), 0.);
    
    float3 d = g_Source.SampleLevel(g_SamplerLinearClamp, uv + offset * float2(-2, 0), 0.);
    float3 e = g_Source.SampleLevel(g_SamplerLinearClamp, uv + offset * float2(0, 0), 0.);
    float3 f = g_Source.SampleLevel(g_SamplerLinearClamp, uv + offset * float2(2, 0), 0.);
    
    float3 g = g_Source.SampleLevel(g_SamplerLinearClamp, uv + offset * float2(-2, -2), 0.);
    float3 h = g_Source.SampleLevel(g_SamplerLinearClamp, uv + offset * float2(0, -2), 0.);
    float3 i = g_Source.SampleLevel(g_SamplerLinearClamp, uv + offset * float2(2, -2), 0.);
    
    float3 j = g_Source.SampleLevel(g_SamplerLinearClamp, uv + offset * float2(-1, 1), 0.);
    float3 k = g_Source.SampleLevel(g_SamplerLinearClamp, uv + offset * float2(1, 1), 0.);
    float3 l = g_Source.SampleLevel(g_SamplerLinearClamp, uv + offset * float2(-1, -1), 0.);
    float3 m = g_Source.SampleLevel(g_SamplerLinearClamp, uv + offset * float2(1, -1), 0.);
    
    g_Output[pixel].xyz = e*0.125+(a+c+g+i)*0.03125+(b+d+f+h)*0.0625+(j+k+l+m)*0.125;
}

float3 SampleTent(Texture2D<float3> source, SamplerState source_sampler, float2 uv) {
    uint2 source_size;
    source.GetDimensions(source_size.x, source_size.y);
    float2 offset = 1. / source_size;

    float3 a = g_Source.SampleLevel(g_SamplerLinearClamp, uv + offset * float2(-1, 1), 0.);
    float3 b = g_Source.SampleLevel(g_SamplerLinearClamp, uv + offset * float2(0, 1), 0.);
    float3 c = g_Source.SampleLevel(g_SamplerLinearClamp, uv + offset * float2(1, 1), 0.);
    
    float3 d = g_Source.SampleLevel(g_SamplerLinearClamp, uv + offset * float2(-1, 0), 0.);
    float3 e = g_Source.SampleLevel(g_SamplerLinearClamp, uv + offset * float2(0, 0), 0.);
    float3 f = g_Source.SampleLevel(g_SamplerLinearClamp, uv + offset * float2(1, 0), 0.);
    
    float3 g = g_Source.SampleLevel(g_SamplerLinearClamp, uv + offset * float2(-1, -1), 0.);
    float3 h = g_Source.SampleLevel(g_SamplerLinearClamp, uv + offset * float2(0, -1), 0.);
    float3 i = g_Source.SampleLevel(g_SamplerLinearClamp, uv + offset * float2(1, -1), 0.);
    return e*0.25+(a+c+g+i)*0.0625+(b+d+f+h)*0.125;
}

[numthreads(8, 8, 1)]
void CS_BloomUpsample(uint3 PixCoord : SV_DispatchThreadID) {
    uint2 output_size;
    g_Output.GetDimensions(output_size.x, output_size.y);
    float2 uv = (PixCoord.xy + .5) / output_size;
    g_Output[PixCoord.xy].xyz += SampleTent(g_Source, g_SamplerLinearClamp, uv);
}

[numthreads(8, 8, 1)]
void CS_BloomApply(uint3 PixCoord : SV_DispatchThreadID) {
    uint2 output_size;
    g_Output.GetDimensions(output_size.x, output_size.y);
    float2 uv = (PixCoord.xy + .5) / output_size;
    g_Output[PixCoord.xy].xyz = g_SourceAdd[PixCoord.xy] + SampleTent(g_Source, g_SamplerLinearClamp, uv) * g_MixFactor;
}