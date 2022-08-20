

struct Instance {
	float3 aabbmin;
	float3 aabbmax;
	uint indexOffset;
	uint indexCount;
	uint material;
	uint3 padding;
};

struct Vertex {
	float3 position;
	float3 normal;
	float3 tangent;
	float2 texcoord;
	float padding;
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
	float4 padding[10];
};

struct MeshCB {
	float4x4 worldMatrix;
	float4x4 padding[3];
};


cbuffer CbFrustum : register(b0) {
	FrustumCB cbFrustum;
};
cbuffer CbMesh : register(b1) {
	MeshCB cbMesh;
};


StructuredBuffer<Instance> instanceBuffer : register(t0);

StructuredBuffer<float4> srcPositionBuffer : register(t1);
StructuredBuffer<float4> srcNormalBuffer : register(t2);
StructuredBuffer<float4> srcTangentBuffer : register(t3);
StructuredBuffer<float4> srcUvBuffer : register(t4);

StructuredBuffer<uint> indexBuffer : register(t5);

RWByteAddressBuffer drawArgBuffer : register(u0);

RWStructuredBuffer<Vertex> destVertexBuffer : register(u1);

RWByteAddressBuffer countBuffer : register(u2);

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
	
	for (i = 0; i < 8; i++)
		points[i] = mul(cbMesh.worldMatrix, points[i]);
	
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
		
		if (!inside)
			return true;
	}
	
	return false;
}

[numthreads(1,1,1)]
void main(uint3 dispatchID : SV_DispatchThreadID) {
	uint mesh_index = dispatchID.x;
	
	if (mesh_index == 0) {
		drawArgBuffer.Store(0, 0);
		drawArgBuffer.Store(4, 1);
		drawArgBuffer.Store(8, 0);
		drawArgBuffer.Store(12, 0);
		countBuffer.Store(0, 0);
		countBuffer.Store(4, 0);
		countBuffer.Store(8, 0);
		countBuffer.Store(12, 0);
	}
	
	AllMemoryBarrierWithGroupSync();
	
	Instance instance = instanceBuffer[mesh_index];
	
	//if (!IsFrustum(instance)) {
	if (mesh_index == 0) {
		uint baseIndex = 0;
		uint maxVertexCount = 0;
		uint vertexStride = 0;

		destVertexBuffer.GetDimensions(maxVertexCount, vertexStride);

		//countBuffer.InterlockedAdd(0, instance.indexCount, baseIndex);

		if (maxVertexCount <= baseIndex + instance.indexCount) return;

		uint i = 0;

		for (i = 0; i < (uint)instance.indexCount && baseIndex + i < maxVertexCount; i++) {
		//for (i = 0; i < 0; i++) {
			Vertex vertex = (Vertex)0;
			uint index = indexBuffer[i + instance.indexOffset];

			float4 position = mul(cbMesh.worldMatrix, srcPositionBuffer[index]);
			float3 normal = normalize(mul((float3x3)cbMesh.worldMatrix, srcNormalBuffer[index].xyz));
			float3 tangent = normalize(mul((float3x3)cbMesh.worldMatrix, srcTangentBuffer[index].xyz));
			float2 texcoord = srcUvBuffer[index].xy;

			vertex.position = position.xyz;
			vertex.normal = normal;
			vertex.tangent = tangent;
			vertex.texcoord = texcoord;

			destVertexBuffer[baseIndex + i] = vertex;
		}

		uint vertexCount = 0;
		//drawArgBuffer.InterlockedAdd(0, i, vertexCount);
	}
}