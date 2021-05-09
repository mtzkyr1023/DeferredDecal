struct PS_IN {
	float4 pos : SV_POSITION;
	float3 nor : NORMAL0;
	float2 tex : TEXCOORD0;
	float viewDistance : TEXCOORD1;
};

struct PS_OUT {
	float4 color : SV_Target0;
	float4 normal : SV_Target1;
	float2 pbr : SV_Target2;
};

PS_OUT main(PS_IN input) {
	PS_OUT output = (PS_OUT)0;
	output.color = float4(1.0f, 1.0f, 1.0f, 1.0f);
	
	output.normal = float4(input.nor, input.viewDistance);
	
	output.pbr = float2(1.0f, 1.0f);
	
	return output;
}