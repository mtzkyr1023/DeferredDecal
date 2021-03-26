
cbuffer CB2 : register(b0) {
	matrix view;
	matrix proj;
	matrix invViewProj;
	float4 screenParam;
	float4 viewPos;
	float4 padding3;
	float4 padding4;
}

struct PerInstance {
	matrix world;
	matrix invWorld;
};

StructuredBuffer<PerInstance> SB0 : register(t0);
Texture2D depthTex : register(t1);
Texture3D decalTex : register(t2);
SamplerState clampSampler : register(s0);

struct PS_IN {
	float4 pos : SV_POSITION;
	float3 zVec : TEXCOORD0;
	uint instanceID : TEXCOORD1;
};

struct PS_OUT {
	float4 color : SV_Target0;
};

PS_OUT main(PS_IN input) {
	PS_OUT output = (PS_OUT)0;
	
	float2 texcoord = input.pos.xy / screenParam.xy;
	
	float depth = depthTex.Sample(clampSampler, texcoord).x;
	
	float4 screenPos = float4(texcoord.x * 2.0f - 1.0f, texcoord.y * -2.0f + 1.0f, depth, 1.0f);
	float4 worldPos = mul(screenPos, invViewProj);
	
	worldPos /= worldPos.w;
	
	float3 viewDir = normalize(worldPos.xyz - viewPos.xyz);
	
	float3 ddxWp = ddx(worldPos.xyz);
	float3 ddyWp = ddy(worldPos.xyz);
	
	float3 normal = normalize(cross(ddxWp, ddyWp));
	
	worldPos = mul(worldPos, SB0[input.instanceID].invWorld);
	
	clip(float3(0.5f, 0.5f, 0.5f) - abs(worldPos.xyz));
	
	float2 tex = float2(0.5f, 0.5f) + worldPos.xy;
	
	float3 tpos = float3(0.5f, 0.5f, 0.5f) + worldPos.xyz;
	
	output.color = float4(decalTex.Sample(clampSampler, tpos).r, 0.0f, 0.0f, 1.0f);
	
	float mask = saturate(dot(-normal, input.zVec));
	float2 shadow = tex * float2(0.5f, -0.5f) - float2(0.25f, -0.25f);
	float vigent = 1.0f - length(shadow);
	shadow.y = 0.25f - abs(shadow.y);
	
	output.color.a = 1.0f;
	
	return output;
}