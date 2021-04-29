
cbuffer CB0 : register(b0) {
	matrix view;
	matrix proj;
	matrix padding1;
	matrix padding2;
}


struct VS_IN {
	float3 pos : POSITION0;
	float3 nor : NORMAL0;
	float3 tan : TANGENT0;
	float2 tex : TEXCOORD0;
};

struct VS_OUT {
	float4 pos : SV_POSITION;
	float3 nor : NORMAL0;
	float2 tex : TEXCOORD0;
	float viewDistance : TEXCOORD1;
};

VS_OUT main(VS_IN input, uint vertexId : SV_VertexID) {
	VS_OUT output = (VS_OUT)0;
	
	output.pos = float4(input.pos, 1.0f);
	output.pos = mul(float4(input.pos, 1.0f), view);
	output.viewDistance = output.pos.z;
	output.pos = mul(output.pos, proj);
	
	const float2 vId = float2((float)(vertexId / 2), (float)(vertexId % 2));
	
//	output.nor = mul(input.nor, (float3x3)world);
	
	output.tex = input.tex;
	
	return output;
}