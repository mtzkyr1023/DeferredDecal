

cbuffer ViewProjBuffer : register(b0) {
	matrix viewMatrix;
	matrix projMatrix;
	
	matrix padding1;
	matrix padding2;
}

struct VS_IN {
	float4 pos : POSITION0;
	float4 nor : NORMAL0;
	float4 tan : TANGENT0;
	float4 tex : TEXCOORD0;
};

struct VS_OUT {
	float4 pos : SV_POSITION;
	float3 nor : NORMAL0;
};

VS_OUT main(VS_IN input) {
	VS_OUT output = (VS_OUT)0;
	output.pos = mul(float4(input.pos.xyz, 1.0f), padding1);
	output.pos = mul(output.pos, viewMatrix);
	output.pos = mul(output.pos, projMatrix);
	
	
	output.nor = input.nor.xyz;
	
	return output;
}