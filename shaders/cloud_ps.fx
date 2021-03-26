
Texture3D cloudTex : register(t0);
Texture2D depthTex : register(t1);
SamplerState wrapSampler : register(s0);
SamplerState clampSampler : register(s1);

cbuffer CB3 : register(b0) {
	matrix world;
	matrix view;
	matrix proj;
	matrix padding;
}

cbuffer CB4 : register(b1) {
	float4 eyePos;
	float4 offset;
	float4 boundMin;
	float4 boundMax;
	
	float4 cloudState;
	float4 cloudScale;
	float4 paddinf1_1;
	float4 paddinf1_2;
	matrix padding2;
	matrix padding3;
}


float2 rayBoxDst(float3 boundsMin, float3 boundsMax, float3 rayOrigin, float3 rayDir) {
	float3 t0 = (boundsMin - rayOrigin) / rayDir;
	float3 t1 = (boundsMax - rayOrigin) / rayDir;
	float3 tmin = min(t0, t1);
	float3 tmax = max(t0, t1);
	
	float dstA = max(max(tmin.x, tmin.y), tmin.z);
	float dstB = min(min(tmax.x, tmax.y), tmax.z);
	
	float dstToBox = max(0, dstA);
	float dstInsideBox = max(0, dstB - dstToBox);
	
	return float2(dstToBox, dstInsideBox);
}

float sampleDensity(float3 pos) {
	float3 uvw = pos / cloudScale.xyz * 0.001f + offset.xyz * 0.01f;
	float shape = cloudTex.Sample(wrapSampler, uvw).r;
	float density = max(0.0f, shape - cloudState.w);
	return density;
}

float sampleIntensity(float3 pos) {
//	pos -= boundMin.xyz;
//	pos /= cloudScale.xyz;
//	pos = saturate(pos) + offset.xyz * 0.1f;
	float3 uvw = (pos / cloudScale.xyz * 0.001f + offset.xyz * 0.01f);
	float intensity = cloudTex.Sample(wrapSampler, uvw).r;
	intensity = max(cloudState.w, intensity);
	return intensity;
}

struct PS_IN {
	float4 pos : SV_POSITION;
	float3 nor : NORMAL0;
	float3 tex : TEXCOORD0;
	float3 wpos : TEXCOORD1;
	float3 vpos : TEXCOORD2;
};

float4 main(PS_IN input) : SV_Target {
	float4 color = (float4)0;
	
	float depth = depthTex[int2(input.pos.xy)].r;
	float2 spos = input.pos.xy / float2(1280.0f, 720.0f);
	float3 vpos;
	vpos.z = proj._m32 / (depth - proj._m22);
	vpos.xy = (spos.xy * vpos.z) / float2(proj._m00, proj._m11);
	
	float3 viewDir = normalize(input.wpos - eyePos.xyz);
	
	float3 origin = eyePos.xyz;
	
	float2 rayBoxInfo = rayBoxDst(boundMin.xyz, boundMax.xyz, origin, viewDir);
	float dstToBox = rayBoxInfo.x;
	float dstInsideBox = rayBoxInfo.y;
	
	float dstTravelled = 0.0f;
	float stepSize = dstInsideBox / 64.0f;
	float step = stepSize / dstInsideBox;
	float dstLimit = min(vpos.z - dstToBox, dstInsideBox);
	dstLimit = dstInsideBox;
	
	float3 lightVec = normalize(-float3(0.0f, 1.0f, 0.0f));
	lightVec = normalize(lightVec + viewDir);
	float intensity = 1.0f;
	float3 rayPos = origin + viewDir * dstLimit;
	
	float totalDensity = 0.0f;
	while (dstTravelled < dstLimit) {
		rayPos = origin + viewDir * (dstToBox + dstTravelled);
		float density = sampleDensity(rayPos);
		totalDensity += density * stepSize;
		dstTravelled += stepSize;
		
		rayPos = rayPos + lightVec;
		intensity = sampleDensity(rayPos);
//		intensity = pow(intensity, 1.0f);
		color.rgb += lerp(float3(1.0f, 1.0f, 1.0f), float3(0.01f, 0.01f, 0.08f), (density - intensity) * 128.0f) * step;
//		color.rgb += float3(intensity, intensity, intensity) * step * 0.01f;
	}
	
	color.rgb = saturate(pow(color.rgb, 2.2f));
	
	float t = 1.0f - exp(-totalDensity);
	float i = exp(-intensity);
	color.a = 1.0f * t;
	return color;
}