
Texture2D portalTex : register(t0);
SamplerState wrapSampler : register(s0);


struct PS_IN {
	float4 pos : SV_POSITION;
	float2 tex : TEXCOORD0;
	float3 zVec : TEXCOORD1;
	float3 nor : NORMAL0;
};

float4 main(PS_IN input) : SV_Target {
	float2 texcoord = input.pos.xy / float2(1280.0f, 720.0f);
	
	float2 spos = input.tex * 2.0f - 1.0f;
	
	float4 color = portalTex.Sample(wrapSampler, texcoord);
	
	float visibility = step(dot(-input.zVec, input.nor), 0.5f);
	
	color.rgb *= pow(1.0f - length(spos), 0.5f) * visibility;
	
	return color;
}