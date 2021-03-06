
Texture2D srcTex : register(t0);
SamplerState wrapSampler : register(s0);

struct PS_IN {
	float4 pos : SV_POSITION;
	float2 tex : TEXCOORD0;
};

float4 main(PS_IN input) : SV_Target {
	return srcTex.Sample(wrapSampler, input.tex);
};