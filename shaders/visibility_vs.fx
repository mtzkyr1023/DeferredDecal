

cbuffer ViewProjBuffer : register(b0) {
	matrix viewMatrix;
	matrix projMatrix;
	
	matrix padding1;
	matrix padding2;
}


StructuredBuffer<float4>  positionBuffer : register(t0);
StructuredBuffer<uint> indexBuffer : register(t1);


float4 main(uint vertexId : SV_VertexID, uint instanceId : SV_InstanceID) : SV_POSITION {
	float4 output = mul(float4(positionBuffer[indexBuffer[vertexId] + instanceId].xyz * 1.0f, 1), viewMatrix);
	output = mul(output, projMatrix);
	
	return output;
}