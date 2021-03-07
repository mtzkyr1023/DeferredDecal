

struct PS_IN {
	float4 pos : SV_POSITION;
	float3 nor : NORMAL0;
	float2 tex : TEXCOORD0;
	float viewDistance : TEXCOORD1;
};

struct PS_OUT {
	float4 color : SV_Target0;
	float4 normal : SV_Target1;
};

PS_OUT main(PS_IN input) {
	PS_OUT output = (PS_OUT)0;
	float intensity = saturate(dot(input.nor, normalize(float3(1.0f, 1.0f, 1.0f)))) * 0.5f + 0.5f;
	intensity *= intensity;
	output.color = float4(1.0f, 1.0f, 1.0f, 1.0f) * intensity;
	output.normal = float4(input.nor, input.viewDistance);
	return output;
}