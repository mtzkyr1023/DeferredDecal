
Texture2D albedoTex : register(t0);
SamplerState wrapSampler : register(s0);

struct PS_IN {
	float4 pos : SV_POSITION;
	float3 nor : NORMAL0;
	float2 tex : TEXCOORD0;
	float viewDistance : TEXCOORD1;
};

struct PS_OUT {
	float4 color : SV_Target0;
	float4 normal : SV_Target1;
};

PS_OUT main(PS_IN input) {
	PS_OUT output = (PS_OUT)0;
	output.color = albedoTex.Sample(wrapSampler, input.tex);
	output.normal = float4(input.nor, input.viewDistance);
	return output;
}