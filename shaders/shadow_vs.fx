

struct CB0 {
	float4x4 viewMatrix;
	float4x4 projMatrix;
	float4x4 worldMatrix;
	float4x4 padding;
};


cbuffer MatrixBuffer : register(b0) {
	CB0 cb0;
}


StructuredBuffer<float4> positionBuffer : register(t0);
StructuredBuffer<float4> texcoordBuffer : register(t1);
StructuredBuffer<uint> indexBuffer : register(t2);

StructuredBuffer<uint> offsetBuffer : register(t3);

struct VS_OUT {
	float4 pos : SV_POSITION;
	float2 tex : TEXCOORD0;
};

VS_OUT main(uint vertexId : SV_VertexID, uint instanceId : SV_InstanceID) {
	
	VS_OUT output = (VS_OUT)0;
	
	output.pos = mul(cb0.worldMatrix, float4(positionBuffer[indexBuffer[vertexId + offsetBuffer[instanceId]]].xyz, 1.0f));
	output.pos = mul(cb0.viewMatrix, output.pos);
	output.pos = mul(cb0.projMatrix, output.pos);
	
	output.tex = texcoordBuffer[indexBuffer[vertexId + offsetBuffer[instanceId]]].xy;
	
	return output;
}