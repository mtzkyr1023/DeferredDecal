

cbuffer DirectionalLight : register(b0) {
	float4 g_lightDirection;
	float4 g_padding1;
	float4 g_padding2;
	float4 g_padding3;
	matrix g_padding4;
	matrix g_padding5;
	matrix g_padding6;
}

Texture2D<uint> visibilityBuffer : register(t0);
SamplerState wrapSampler : register(s0);

struct PS_IN {
	float4 pos : SV_POSITION;
	float2 tex : TEXCOORD0;
};

float4 main(PS_IN input) : SV_Target {
	uint id = visibilityBuffer[(uint2)input.pos.xy];

	float4 color = float4(1.0f, 0.0f, 0.0f, 1.0f);

	color.r = (float)(id % 256) / 255.0f;
	
	return color;
}