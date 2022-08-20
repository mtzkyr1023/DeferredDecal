
static const float PI = 3.14159265f;
static const int SIZE = 128;

struct MatrixBuffer {
	float4x4 viewMatrix;
	float4x4 projMatrix;
	float4x4 worldMatrix;
	float4 viewPos;
	float4x3 padding2;
};

cbuffer ViewProjBuffer : register(b0) {
	MatrixBuffer cbMatrix;
}

struct Vertex {
	float3 position;
	float3 normal;
	float3 tangent;
	float2 texcoord;
	float padding;
};

StructuredBuffer<float4> positionBuffer : register(t0);
StructuredBuffer<float4> uvBuffer : register(t1);
StructuredBuffer<uint> indexBuffer : register(t2);

StructuredBuffer<uint> offsetBuffer : register(t3);

struct VS_OUT {
	float4 pos : SV_POSITION;
	float2 tex : TEXCOORD0;
};

VS_OUT main(uint vertexId : SV_VertexID, uint instanceId : SV_InstanceID) {
	
	VS_OUT output = (VS_OUT)0;

	output.pos = mul(cbMatrix.worldMatrix, float4(positionBuffer[indexBuffer[vertexId + offsetBuffer[instanceId]]].xyz, 1.0f));
	output.pos = mul(cbMatrix.viewMatrix, output.pos);
	output.pos = mul(cbMatrix.projMatrix, output.pos);

	output.tex = uvBuffer[indexBuffer[vertexId + offsetBuffer[instanceId]]].xy;


	return output;
}