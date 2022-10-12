

struct ViewProjStruct {
	float4x4 view;
	float4x4 proj;
	float4x4 padding;
	float4x4 invVP;
};


struct ShadowViewProjStruct {
	float4x4 view;
	float4x4 proj;
	float4x4 padding;
	float4x4 invVP;
	float4x4 invV;
	float3 lightVec;
};

struct ScreenInfoStruct {
	float4 screenInfo;
	float4x4 invV;
	float4x4 padding1[2];
	float4x3 padding2;
};

cbuffer CB0 : register(b0) {;
	ViewProjStruct cameraVP;
}

cbuffer CB1 : register(b1) {;
	ShadowViewProjStruct shadowVP;
}

Texture2D albedoTex : register(t0);
Texture2D normalTex : register(t1);
Texture2D roughMetalTex : register(t2);
Texture2D<float> depthBuffer : register(t3);
Texture2D<float2> shadowMap : register(t4);
Texture2D reflectMap : register(t5);


struct PointLight {
	float3 pos;
	float radius;
	float3 color;
	float atten;
};

StructuredBuffer<PointLight> pointLightBuffer : register(t6);
StructuredBuffer<uint> lightIndexOffsetBuffer : register(t7);
StructuredBuffer<uint> lightIndexBuffer : register(t8);

SamplerState wrapSampler : register(s0);
SamplerState clampSampler : register(s1);


struct PS_IN {
	float4 pos : SV_POSITION;
	float2 tex : TEXCOORD0;
};

float shadow_check(float2 shadow, float depth) {
	float depth_sq = shadow.x * shadow.x;
	float md = shadow.x - depth;
	float variance = shadow.y - depth_sq;
	variance = min(1.0f, max(0.0f, variance + 0.0001f));
	float p = variance / (variance + (md * md));
	
	return p;
} 

float linstep(float min, float max, float v) {
	return saturate((v - min) / (max - min));
}

float4 main(PS_IN input) : SV_Target {
	float4 normal = normalTex.Sample(wrapSampler, input.tex);
	float depth = depthBuffer.Sample(wrapSampler, input.tex).r;
	
	float4 screenPosition = float4(input.tex.x * 2.0f - 1.0f, input.tex.y * -2.0f + 1.0f, depth, 1.0f);
	//worldPosition.xy +=  float2(1.0f,1.0f);
	float4 worldPosition = mul(cameraVP.invVP, screenPosition);
	worldPosition = worldPosition / worldPosition.w;
	float4 shadowPosition = mul(shadowVP.view, worldPosition);
	shadowPosition = mul(shadowVP.proj, shadowPosition);
	//shadowPosition = shadowPosition / shadowPosition.w;
	//shadowPosition.x = shadowPosition.x * 1000.0f;
	//shadowPosition.y = shadowPosition.y * 1000.0f;
	//shadowPosition.z = -shadowPosition.z * 1000.0f;
	//shadowPosition = mul(shadowVP.invV, shadowPosition);
	//shadowPosition = mul(shadowVP.view, shadowPosition);
	//shadowPosition = mul(shadowVP.proj, shadowPosition);
	//shadowPosition /= 1000.0f;
	shadowPosition = shadowPosition / shadowPosition.w;
	
	float4 viewPosition = mul(cameraVP.view, worldPosition);
	
	float2 shadowCoord = float2(shadowPosition.x * 0.5f + 0.5f, shadowPosition.y * -0.5f + 0.5f);
	
	float4 color = albedoTex.Sample(wrapSampler, input.tex);
	
	float2 shadowDepth1 = shadowMap.Sample(clampSampler, shadowCoord).rg;
	float scl = 0.0001f;
	float2 shadowDepth2 = shadowMap.Sample(clampSampler, shadowCoord + float2(scl, scl)).rg;
	float2 shadowDepth3 = shadowMap.Sample(clampSampler, shadowCoord + float2(-scl, scl)).rg;
	float2 shadowDepth4 = shadowMap.Sample(clampSampler, shadowCoord + float2(scl, -scl)).rg;
	float2 shadowDepth5 = shadowMap.Sample(clampSampler, shadowCoord + float2(-scl, -scl)).rg;
	
	float4 reflectColor = reflectMap.Sample(clampSampler, shadowCoord);
	
	float intensity = saturate(dot(normal.xyz, -shadowVP.lightVec)) * 0.5f + 0.5f;
	intensity *= intensity;
	
	float p = 0.0f;
	p += shadow_check(shadowDepth1, shadowPosition.z);
	//p += shadow_check(shadowDepth2, worldPosition.z);
	//p += shadow_check(shadowDepth3, worldPosition.z);
	//p += shadow_check(shadowDepth4, worldPosition.z);
	//p += shadow_check(shadowDepth5, worldPosition.z);
	
	//p /= 5.0f;
	
	//p = max(0.25f, p);
	float variance = 1.0f;
	
	variance *= max(shadowDepth1.x >= shadowPosition.z, p);
	variance *= max(shadowDepth2.x >= shadowPosition.z, p);
	variance *= max(shadowDepth3.x >= shadowPosition.z, p);
	variance *= max(shadowDepth4.x >= shadowPosition.z, p);
	variance *= max(shadowDepth5.x >= shadowPosition.z, p);
	variance = max(0.25f, variance);
	
	//return color;
	
	color = color * variance;
	
	
	screenPosition.x = (screenPosition.x * 0.5f + 0.5f) * 16.0f;
	screenPosition.y = (screenPosition.y * 0.5f + 0.5f) * 16.0f;
	screenPosition.z = -normal.w / (2000.0f - 0.1f) * 16.0f;
	uint frustumIndex = clamp((uint)screenPosition.x + (uint)screenPosition.y * 16 + (uint)screenPosition.z * 256, 0, 4095);
	uint lightCount = lightIndexOffsetBuffer[frustumIndex];
	
	for (int i = 0; i < lightCount; i++) {
		PointLight light = pointLightBuffer[lightIndexBuffer[i + frustumIndex * 256]];
		float atten = length(worldPosition.xyz - light.pos);
		intensity = saturate(dot(-normal.xyz, normalize(worldPosition.xyz - light.pos)));
		color.rgb += light.color * linstep(light.radius, 0.0f, atten) * intensity;
	}
	
	//return normal;
	//depth = screenPosition.z / 16.0f;
	//return float4(depth, depth,depth ,0.0f);
	return color;
	//return shadowDepth1.rrrr;
	
	//return float4(screenPosition.xyz / 16.0f, 0.0f);
};