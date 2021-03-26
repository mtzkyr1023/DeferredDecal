

cbuffer CB3 : register(b0) {
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
	float3 nor : NORMAL0;
	float3 tex : TEXCOORD0;
	float3 wpos : TEXCOORD1;
	float3 vpos : TEXCOORD2;
};

VS_OUT main(VS_IN input) {
	VS_OUT output = (VS_OUT)0;
	
	output.pos = mul(float4(input.pos, 1.0f), world);
	output.wpos = output.pos.xyz;
	output.pos = mul(output.pos, view);
	output.vpos = output.pos.xyz;
	output.pos = mul(output.pos, proj);
	
	output.tex = float3(0.5f, 0.5f, 0.5f) + input.pos;
	
	output.nor = normalize(mul(input.nor, (float3x3)world));
	
	return output;
}