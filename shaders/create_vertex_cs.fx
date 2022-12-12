

struct Instance {
	float3 aabbmin;
	float3 aabbmax;
	uint indexOffset;
	uint indexCount;
	uint2 cbv;
};

struct Vertex {
	float4 position;
	float4 normal;
	float4 tangent;
	float4 texcoord;
};

struct DrawArg
{
	uint indexCountPerInstance;
	uint instanceCount;
	uint startIndexLocation;
	int	baseVertexLocation;
	uint startInstanceLocation;
};


struct FrustumCB {
	float4 frustumPlane[6];
	float4x4 viewMatrix;
};

struct MeshCB {
	float4x4 viewMatrix;
	float4x4 projMatrix;
	float4x4 worldMatrix;
};

struct IndirectCommand {
	uint2 cbv;
	uint indexCountPerInstance;
	uint instanceCount;
	uint startIndexLocation;
	int	baseVertexLocation;
	uint startInstanceLocation;
};


cbuffer CbFrustum : register(b0) {
	FrustumCB cbFrustum;
}
cbuffer CbMesh : register(b1) {
	MeshCB cbMesh;
};


StructuredBuffer<Instance> instanceBuffer : register(t0);


AppendStructuredBuffer<IndirectCommand> outputCommands : register(u0);
AppendStructuredBuffer<uint> outputIndexOffset : register(u1);
RWByteAddressBuffer counterBuffer : register(u2);


bool IsFrustum(Instance instance) {
	float4 points[8];
	points[0]= float4(instance.aabbmin.x, instance.aabbmin.y, instance.aabbmin.z, 1);
	points[1]= float4(instance.aabbmin.x, instance.aabbmin.y, instance.aabbmax.z, 1);
	points[2]= float4(instance.aabbmin.x, instance.aabbmax.y, instance.aabbmin.z, 1);
	points[3]= float4(instance.aabbmin.x, instance.aabbmax.y, instance.aabbmax.z, 1);
	points[4]= float4(instance.aabbmax.x, instance.aabbmin.y, instance.aabbmin.z, 1);
	points[5]= float4(instance.aabbmax.x, instance.aabbmin.y, instance.aabbmax.z, 1);
	points[6]= float4(instance.aabbmax.x, instance.aabbmax.y, instance.aabbmin.z, 1);
	points[7]= float4(instance.aabbmax.x, instance.aabbmax.y, instance.aabbmax.z, 1);

	int i = 0, j = 0;
	
	for (i = 0; i < 8; i++) {
		points[i] = mul(cbMesh.worldMatrix, points[i]);
		//points[i] = mul(cbFrustum.viewMatrix, points[i]);
	}
	
	for (i = 0; i < 6; i++) {
		float3 plane_normal = cbFrustum.frustumPlane[i].xyz;
		float plane_constant = cbFrustum.frustumPlane[i].w;
		
		bool inside = false;
	
		for (j = 0; j < 8; j++) {
			if (dot(plane_normal, points[j].xyz) + plane_constant >= 0) {
				inside = true;
				break;
			}
		}
		
		
		if (!inside) {
			return true;
		}
	}
	
	return false;
}

[numthreads(1,1,1)]
void main(uint3 dispatchID : SV_DispatchThreadID) {
	uint mesh_index = dispatchID.x;
	
	Instance instance = instanceBuffer[mesh_index];
	
	if (!IsFrustum(instance))
	{
		IndirectCommand output = (IndirectCommand)0;
		
		
		output.indexCountPerInstance = instance.indexCount;
		output.startIndexLocation = instance.indexOffset;
		
		uint arg_index = 0;
		//counterBuffer.InterlockedAdd(0, 1, arg_index);
		
		output.startInstanceLocation = arg_index;
		output.instanceCount = arg_index + 1;
		output.cbv = instance.cbv;
		
		outputCommands.Append(output);
		outputIndexOffset.Append(instance.indexOffset);
	}
}