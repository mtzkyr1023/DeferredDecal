static const float PI = 3.14159265f;
static const int SIZE = 128;

TextureCube cubeMap : register(t3);
SamplerState wrapSampler : register(s0);


struct PS_IN {
	float4 pos : SV_POSITION;
	float3 tex : TEXCOORD0;
};

float4 main(PS_IN input) : SV_Target0 {
	

	float4 color = cubeMap.Sample(wrapSampler, input.tex);
	
		
	return color;
}
