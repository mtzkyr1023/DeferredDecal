
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

struct VS_IN {
	float3 pos : POSITION0;
	float3 nor : NORMAL0;
	float3 tan : TANGENT0;
	float2 tex : TEXCOORD0;
};

struct VS_OUT {
	float4 pos : SV_POSITION;
	float3 zVec : TEXCOORD0;
	uint instanceID : TEXCOORD1;
};

VS_OUT main(VS_IN input, uint instanceID : SV_InstanceID) {
	VS_OUT output = (VS_OUT)0;
	
	output.pos = mul(float4(input.pos, 1.0f), SB0[instanceID].world);
	output.pos = mul(output.pos, view);
	output.pos = mul(output.pos, proj);
	
	output.instanceID = instanceID;
	
	output.zVec =
		normalize(float3(SB0[instanceID].world[2][0],
				SB0[instanceID].world[2][1],
				SB0[instanceID].world[2][2]));
	
	return output;
}