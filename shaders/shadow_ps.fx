
Texture2D albedoTex : register(t4);
SamplerState wrapSampler : register(s0);


struct PS_IN {
	float4 pos : SV_POSITION;
	float2 tex : TEXCOORD0;
};

struct PS_OUT {
	float2 shadow : SV_Target0;
	float4 albedo : SV_Target1;
};

PS_OUT main(PS_IN input) {
	PS_OUT output = (PS_OUT)0;
	
	output.shadow = float2(input.pos.z, input.pos.z * input.pos.z);
	
	output.albedo = albedoTex.Sample(wrapSampler, input.tex);
	
	return output;
}