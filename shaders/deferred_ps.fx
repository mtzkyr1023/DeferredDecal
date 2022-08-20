
Texture2D albedoTex : register(t0);
Texture2D normalTex : register(t1);
Texture2D roughMetalTex : register(t2);
Texture2D shadowMap : register(t3);


struct PS_IN {
	float4 pos : SV_POSITION;
	float2 tex : TEXCOORD0;
};

float4 main(PS_IN input) : SV_Target {
	return albedoTex.Sample(wrapSampler, input.tex);
};