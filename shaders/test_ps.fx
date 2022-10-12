static const float PI = 3.14159265f;
static const int SIZE = 128;

Texture2D albedoTex : register(t6);
Texture2D normalTex : register(t7);
Texture2D roughMetalTex : register(t8);
TextureCube cubeMap : register(t9);

SamplerState wrapSampler : register(s0);
SamplerState clampSampler : register(s1);


struct MatrixBuffer {
	float4x4 viewMatrix;
	float4x4 projMatrix;
	float4x4 worldMatrix;
	float4x4 padding2;
};

cbuffer ViewProjBuffer : register(b0) {
	MatrixBuffer cb0;
}

struct PS_OUT {
	float4 albedo : SV_Target0;
	float4 normal : SV_Target1;
	float4 roughMetal : SV_target2;
};

struct PS_IN {
	float4 pos : SV_POSITION;
	float3 nor : NORMAL0;
	float3 tan : NORMAL1;
	float3 binor : NORMAL2;
	float2 tex : TEXCOORD0;
	float linearZ : TEXCOORD1;
};


PS_OUT main(PS_IN input){
	
	PS_OUT output = (PS_OUT)0;
		
	float4 color = albedoTex.Sample(wrapSampler, input.tex);
	
	float3 bump = normalTex.Sample(wrapSampler, input.tex).xyz * 2.0f - float3(1.0f, 1.0f, 1.0f);
	
	float3 normal = normalize(input.tan * bump.x + input.binor * bump.y + input.nor * bump.z);
	
	output.albedo = color;
	output.normal = float4(normal, input.linearZ);
	output.roughMetal = roughMetalTex.Sample(wrapSampler, input.tex);
	
	return output;
}
