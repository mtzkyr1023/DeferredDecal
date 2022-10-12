
struct LineInfo {
	float3 pos;
	float3 color;
	float2 padding;
};

struct Matrix {
	float4x4 view;
	float4x4 proj;
	float4x4 padding[2];
};


struct VS_OUT {
	float4 pos : SV_POSITION;
	float4 col : COLOR0;
};

cbuffer MatrixBuffer : register(b0) {
	Matrix cb0;
}

StructuredBuffer<LineInfo> lineBuffer : register(t0);


VS_OUT main(uint vertexId : SV_VertexID) {
	VS_OUT output = (VS_OUT)0;
	
	output.pos = mul(cb0.view, float4(lineBuffer[vertexId].pos.xyz, 1.0f));
	//output.pos.z += 990.0f;
	output.pos = mul(cb0.proj, output.pos);
	
	output.col = float4(lineBuffer[vertexId].color, 1.0f);
	
	return output;
}