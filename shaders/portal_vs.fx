cbuffer CB0 : register(b0) {
	matrix world;
	matrix view;
	matrix proj;
	matrix padding;
}


struct VS_IN {
	float3 pos : POSITION0;
	float3 nor : NORMAL0;
	float3 tan : TANGENT0;
	float2 tex : TEXCOORD0;
};

struct VS_OUT {
	float4 pos : SV_POSITION;
	float2 tex : TEXCOORD0;
	float3 zVec : TEXCOORD1;
	float3 nor : NORMAL0;
};

VS_OUT main(VS_IN input, uint vertexId : SV_VertexID) {
	VS_OUT output = (VS_OUT)0;
	
	output.pos = mul(float4(input.pos, 1.0f), world);
	output.pos = mul(output.pos, view);
	output.pos = mul(output.pos, proj);
	
	output.zVec = normalize(float3(world[2][0], world[2][1], world[2][2]));
	
	output.nor = normalize(mul(input.nor, (float3x3)world));
	
	output.tex = input.tex;
	
	return output;
}