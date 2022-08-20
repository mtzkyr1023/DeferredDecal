

struct Instance {
	float3 aabbmin;
	float3 aabbmax;
	uint indexOffset;
	uint material;
};


struct DrawArg{
	uint indexCount;
	uint instanceCount;
	uint startIndexLocation;
	int BaseVertexLocation;
	uint startInstanceLocation;
};

struct Vertex {
	float3 pos;
	float3 nor;
	float3 tan;
	float2 tex;
	uint material;
};


struct FrustumCB {
	float4 plane[6];
	float4 padding[10]
};

struct MeshCB {
	float4x4 worldMatrix;
	float4x4 prevWorldMatrix;
	float4x4 padding[2];
};

struct SceneCB {
	float4x4 viewMatrix;
	float4x4 projMatrix;
	float4x4 prevViewMatrix;
	float4x4 padding;
};

ConstantBuffer<FrustumCB> cbFrustum : register(b0);
ConstantBuffer<SceneCB> cbScene : register(b1);
ConstantBuffer<MeshCB> cbMesh: register(b2);

StructuredBuffer<Instance> instanceBuffer : register(t0);
StructuredBuffer<float4> positionBuffer : register(t1);
StructuredBuffer<float4> normalBuffer : register(t2);
StructuredBuffer<float4> tangentBuffer : register(t3);
StructuredBuffer<float4> uvBuffer : register(t4);

RWByteAdressBuffer drawArg : register(u0);
RWStructuredBuffer<Vertex> vertexUberBuffer : register(u1);


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
	
	for (int i = 0; i < 8; i++)
		points[i] = mul(cbMesh.worldMatrix, points[i]);
	
	
	for (int i = 0; i < 6; i++) {
		float3 plane_normal = cbFrustum[i].xyz;
		float plane_constant = cbFrustum[i].w;
		
		bool inside = false;
		for (int j = 0; j < 8; j++) {
			if (dot(plane_normal, points[j].xyz) + plane_constant >= 0) {
				inside = false;
				break;
			}
		}
	}
	
	if (!inside)
		return true;
	
	return false;
}


[numthreads(1,1,1)]
void main(uint3 DTID : SV_DispatchThreadID) {
	uint instanceIndex = DTID.x;
	Instance instance = instanceBuffer[instanceIndex];
	
	if (!IsFrustum(instance)) {
		drawArg.InterlockedAdd(0, 1, arg_index);
	}
}