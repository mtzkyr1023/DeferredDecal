
cbuffer CB0 : register(b0) {
	matrix view;
	matrix proj;
	matrix padding1;
	matrix padding2;
}

cbuffer CB1 : register(b1) {
	matrix world;
	matrix padding3;
	matrix padding4;
	matrix padding5;
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

VS_OUT main(VS_IN input) {
	VS_OUT output = (VS_OUT)0;
	
	output.pos = mul(float4(input.pos, 1.0f), world);
	output.pos = mul(output.pos, view);
	output.viewDistance = output.pos.z;
	output.pos = mul(output.pos, proj);
	
	output.nor = mul(input.nor, (float3x3)world);
	
	output.tex = input.tex;
	
	return output;
}