
cbuffer CB0 : register(b0) {
	matrix view;
	matrix proj;
	matrix padding1;
	matrix padding2;
}

StructuredBuffer<float4> positionBuffer : register(t0);
StructuredBuffer<float4> normalBuffer : register(t1);
StructuredBuffer<float4> tangentBuffer : register(t2);
StructuredBuffer<float4> uvBuffer : register(t3);
StructuredBuffer<uint> indexBuffer : register(t4);

struct VS_OUT {
	float4 pos : SV_POSITION;
	float3 nor : NORMAL0;
	float3 tan : TANGENT0;
	float3 binor : BINORMAL0;
	float2 tex : TEXCOORD0;
};

VS_OUT main(uint vertexId : SV_VertexID) {
	VS_OUT output = (VS_OUT)0;
	
	output.pos = mul(float4(positionBuffer[indexBuffer[vertexId]].xyz * 1.0f, 1), view);
	//output.viewDistance = output.pos.z;
	output.pos = mul(output.pos, proj);
	
	output.nor = normalBuffer[indexBuffer[vertexId]].xyz;
	output.tan = tangentBuffer[indexBuffer[vertexId]].xyz;
	
	output.binor = cross(output.nor,output.tan);
	
	output.tex = uvBuffer[indexBuffer[vertexId]].xy;
	
	return output;
}