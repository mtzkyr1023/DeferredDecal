
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
StructuredBuffer<float4> normalBuffer : register(t1);
StructuredBuffer<float4> tangentBuffer : register(t2);
StructuredBuffer<float4> texcoordBuffer : register(t3);
StructuredBuffer<uint> indexBuffer : register(t4);
StructuredBuffer<uint> offsetBuffer : register(t5);

struct VS_OUT {
	float4 pos : SV_POSITION;
	float3 nor : NORMAL0;
	float3 tan : NORMAL1;
	float3 binor : NORMAL2;
	float2 tex : TEXCOORD0;
	float linearZ : TEXCOORD1;
};

VS_OUT main(uint vertexId : SV_VertexID, uint instanceId : SV_InstanceID) {
	
	VS_OUT output = (VS_OUT)0;

	output.pos = mul(cbMatrix.worldMatrix, float4(positionBuffer[indexBuffer[vertexId + offsetBuffer[instanceId]]].xyz, 1.0f));
	output.pos = mul(cbMatrix.viewMatrix, output.pos);
	//output.pos.z += 990.0f;
	output.linearZ = output.pos.z;
	output.pos = mul(cbMatrix.projMatrix, output.pos);

	output.nor = normalize(mul((float3x3)cbMatrix.worldMatrix, normalBuffer[indexBuffer[vertexId + offsetBuffer[instanceId]]].xyz));
	output.tan = normalize(mul((float3x3)cbMatrix.worldMatrix, tangentBuffer[indexBuffer[vertexId + offsetBuffer[instanceId]]].xyz));
	
	output.binor = normalize(cross(output.tan, output.nor));
	
	output.tex = texcoordBuffer[indexBuffer[vertexId + offsetBuffer[instanceId]]].xy;

	return output;
}