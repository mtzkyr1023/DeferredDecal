
static const float3 luminance = float3(0.299f, 0.587f, 0.114f);


static const float EdgeThreshold = 1.0f / 8.0f;
static const float EdgeThresholdMin = 1.0f / 32.0f;
static const float Subpix = 2;
static const float SubpixTrim = 1.0f / 4.0f;
static const float SubpixCap = 7.0f / 8.0f;
static const float SubpixTrimScale = 1.0f;
static const int SearchSteps = 8;
static const int SearchAcceleration = 1;
static const float SubpixSize = 128.0f;
static const float SubpixMul = 1.0f / 8.0f;
static const float SubpixMax = 64.0f;

cbuffer ScreenInfo : register(b0) {
	float4x4 padding1[3];
	float4x4 padding2;
	float4x4 padding3[3];
	float4x3 padding4;
	float4 screenInfo;
}

Texture2D<float4> srcTex : register(t0);
RWTexture2D<float4> destTex : register(u0);

[numthreads(16,16,1)]
void main(uint3 id : SV_DispatchThreadID) {
	float3 rgbN = srcTex[clamp(id.xy - uint2(0, -1), uint2(0, 0), uint2(screenInfo.x - 1, screenInfo.y - 1))].rgb;
	float3 rgbW = srcTex[clamp(id.xy - uint2(-1, 0), uint2(0, 0), uint2(screenInfo.x - 1, screenInfo.y - 1))].rgb;
	float3 rgbM = srcTex[id.xy].rgb;
	float3 rgbE = srcTex[clamp(id.xy - uint2(1, 0), uint2(0, 0), uint2(screenInfo.x - 1, screenInfo.y - 1))].rgb;
	float3 rgbS = srcTex[clamp(id.xy - uint2(0, 1), uint2(0, 0), uint2(screenInfo.x - 1, screenInfo.y - 1))].rgb;
	
	float lumaN = dot(luminance, rgbN);
	float lumaW = dot(luminance, rgbW);
	float lumaM = dot(luminance, rgbM);
	float lumaE = dot(luminance, rgbE);
	float lumaS = dot(luminance, rgbS);
	
	float rangeMin = min(lumaM, min(min(lumaN, lumaW), min(lumaS, lumaE)));
	float rangeMax = max(lumaM, max(max(lumaN, lumaW), max(lumaS, lumaE)));
	float range = rangeMax - rangeMin;
	
	float4 result = (float4)0;
	
	if (range < max(EdgeThresholdMin, rangeMax * EdgeThreshold)) {
		result = float4(rgbM, 0.0f);
		
		destTex[id.xy] = result;
		
		return;
	}
	
	float lumaL = (lumaN + lumaW + lumaE + lumaS) * 0.25f;
	float rangeL = abs(lumaL - lumaM);
	float blendL = max(0.0f, (rangeL / range) - SubpixTrim) * SubpixTrimScale;
	blendL = min(SubpixCap, blendL);
	
	float3 rgbL = rgbN + rgbW + rgbM + rgbE + rgbS;
	
	float3 rgbNW = srcTex[clamp(id.xy - uint2(-1, -1), uint2(0, 0), uint2(screenInfo.x - 1, screenInfo.y - 1))].rgb;
	float3 rgbNE = srcTex[clamp(id.xy - uint2(1, -1), uint2(0, 0), uint2(screenInfo.x - 1, screenInfo.y - 1))].rgb;
	float3 rgbSW = srcTex[clamp(id.xy - uint2(-1, 1), uint2(0, 0), uint2(screenInfo.x - 1, screenInfo.y - 1))].rgb;
	float3 rgbSE = srcTex[clamp(id.xy - uint2(1, 1), uint2(0, 0), uint2(screenInfo.x - 1, screenInfo.y - 1))].rgb;
	
	rgbL += (rgbNW + rgbNE + rgbSW + rgbSE);
	rgbL *= (1.0f / 9.0f);
	
	
    float lumaNW = dot(rgbNW, luminance);
    float lumaNE = dot(rgbNE, luminance);
    float lumaSW = dot(rgbSW, luminance);
    float lumaSE = dot(rgbSE, luminance);
	
	float edgeVert =
		abs((0.25 * lumaNW) + (-0.5 * lumaN) + (0.25 * lumaNE)) +
		abs((0.50 * lumaW ) + (-1.0 * lumaM) + (0.50 * lumaE )) +
		abs((0.25 * lumaSW) + (-0.5 * lumaS) + (0.25 * lumaSE));
	float edgeHorz =
		abs((0.25 * lumaNW) + (-0.5 * lumaW) + (0.25 * lumaSW)) +
		abs((0.50 * lumaN ) + (-1.0 * lumaM) + (0.50 * lumaS )) +
		abs((0.25 * lumaNE) + (-0.5 * lumaE) + (0.25 * lumaSE));
	bool horzSpan = edgeHorz >= edgeVert;
	
	float2 dir; 
	dir.x = ((lumaNW + lumaNE) - (lumaSW + lumaSE));
	dir.y = ((lumaNW + lumaSW) - (lumaNE + lumaSE));
	
	float dirReduce = max(
		(lumaNW + lumaNE + lumaSW + lumaSE) * (0.25 * EdgeThreshold),
		SubpixSize);
	float rcpDirMin = 1.0/(min(abs(dir.x), abs(dir.y)) + dirReduce);
	dir = min(float2(SubpixMax,  SubpixMax), 
		max(float2(-SubpixMax, -SubpixMax), 
		dir * rcpDirMin)) * screenInfo.xy;
	
	float3 rgbA = (1.0/2.0) * (
		srcTex[clamp(id.xy + (uint2)(dir * (1.0f / 3.0f - 0.5f)), uint2(0, 0), uint2(screenInfo.x - 1, screenInfo.y - 1))].rgb +
		srcTex[clamp(id.xy + (uint2)(dir * (2.0f / 3.0f - 0.5f)), uint2(0, 0), uint2(screenInfo.x - 1, screenInfo.y - 1))].rgb);
	float3 rgbB = rgbA * (1.0/2.0) + (1.0/4.0) * (
		srcTex[clamp(id.xy + (uint2)(dir * (0.1f / 3.0f - 0.5f)), uint2(0, 0), uint2(screenInfo.x - 1, screenInfo.y - 1))].rgb +
		srcTex[clamp(id.xy + (uint2)(dir * (0.5f / 3.0f - 0.5f)), uint2(0, 0), uint2(screenInfo.x - 1, screenInfo.y - 1))].rgb);
	float lumaB = dot(rgbB, luminance);
	if((lumaB < rangeMin) || (lumaB > rangeMax)) result = float4(rgbA, 0.0f);
	else result = float4(rgbB, 0.0f);
    
	destTex[id.xy] = result;
	//destTex[id.xy] = float4(rgbM, 1.0f); 
	
//	for(uint i = 0; i < FXAA_SEARCH_STEPS; i++) {
//		#if FXAA_SEARCH_ACCELERATION == 1
//		if(!doneN) lumaEndN = FxaaLuma(FxaaTexture(tex, posN.xy).xyz);
//		if(!doneP) lumaEndP = FxaaLuma(FxaaTexture(tex, posP.xy).xyz);
//		#else
//		if(!doneN) lumaEndN = FxaaLuma(
//		FxaaTextureGrad(tex, posN.xy, offNP).xyz);
//		if(!doneP) lumaEndP = FxaaLuma(
//		FxaaTextureGrad(tex, posP.xy, offNP).xyz);
//		#endif
//		doneN = doneN || (abs(lumaEndN - lumaN) >= gradientN);
//		doneP = doneP || (abs(lumaEndP - lumaN) >= gradientN);
//		if(doneN && doneP) break;
//		if(!doneN) posN -= offNP;
//		if(!doneP) posP += offNP;
//	}
}