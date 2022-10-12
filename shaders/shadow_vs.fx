

struct CB0 {
	float4x4 viewMatrix;
	float4x4 projMatrix;
	float4x4 worldMatrix;
	float4x4 padding;
	float4x4 invView;
};

struct CB1 {
	float4x4 viewMatrix;
	float4x4 projMatrix;
	float4x4 worldMatrix;
	float4x4 padding;
};


cbuffer MatrixBuffer : register(b0) {
	CB0 cb0;
}

cbuffer ShadowMatrixBuffer : register(b1) {
	CB1 cb1;
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
	
	output.pos = mul(cb1.worldMatrix, float4(positionBuffer[indexBuffer[vertexId + offsetBuffer[instanceId]]].xyz, 1.0f));
	output.pos = mul(cb1.viewMatrix, output.pos);
	//output.pos.z -= 1000.0f;
	output.pos = mul(cb1.projMatrix, output.pos);
	output.pos = mul(cb0.invView, output.pos);
	//output.pos /= output.pos.w;
	output.pos.x = output.pos.x * 0.1f;
	output.pos.y = output.pos.y * 0.1f;
	output.pos.z = output.pos.z * 1.0f;
	//output.pos = mul(cb0.invView, output.pos);
	//output.pos = mul(cb0.viewMatrix, output.pos);
	//output.pos = mul(cb0.projMatrix, output.pos);
	
	//output.pos.z = 0.5f;

	
	output.tex = texcoordBuffer[indexBuffer[vertexId + offsetBuffer[instanceId]]].xy;
	
	return output;
}