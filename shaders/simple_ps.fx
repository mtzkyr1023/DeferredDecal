static const float PI = 3.14159265f;
static const int SIZE = 128;

Texture2D albedoTex : register(t4);
SamplerState wrapSampler : register(s0);


struct PS_IN {
	float4 pos : SV_POSITION;
	float2 tex : TEXCOORD0;
};

float4 main(PS_IN input) : SV_Target0{
	

	float4 color = albedoTex.Sample(wrapSampler, input.tex);
	
		
	return color;
}
