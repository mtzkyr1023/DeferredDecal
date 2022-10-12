
struct PointLight {
	float3 pos;
	float radius;
	float3 color;
	float atten;
};


struct MatrixBuffer {
	float4x4 view;
	float4x4 proj;
	float4x4 padding;
	float4x4 invVP;
};

struct Frustum {
	float4 Min;
	float4 Max;
};

cbuffer ViewProjBuffer : register(b0) {
	MatrixBuffer Matrix;
}

StructuredBuffer<PointLight> PointLightBuffer : register(t0);
StructuredBuffer<Frustum> FrustumBuffer : register(t1);
RWStructuredBuffer<uint> IndexOffsetBuffer : register(u0);
RWStructuredBuffer<uint> LightIndexBuffer : register(u1);


[numthreads(16,16,1)]
void main(uint3 id : SV_DispatchThreadID) {
	
	
	uint threadIndex = id.z * 256 + id.y * 16 + id.x;
	
	float clipSizeX = 1.0f / 16.0f;
	float clipSizeY = 1.0f / 16.0f;
	float clipSizeZ = 1.0f / 16.0f;
	
	float3 minClip;
	float3 maxClip;
	minClip.xyz = FrustumBuffer[threadIndex].Min.xyz;
	maxClip.xyz = FrustumBuffer[threadIndex].Max.xyz;
	
	
	
	uint lightNum = 0;
	uint stride = 0;
	PointLightBuffer.GetDimensions(lightNum, stride);
	
	uint lightCount = 0;
	
	float4 screenPosition = float4((float)id.x / 16.0f - 8.0f, (float)id.y / 16.0f - 8.0f, (float)id.z / 16.0f, 1.0f);
	
	screenPosition.x = screenPosition.x * 0.5f + 0.5f;
	screenPosition.y = screenPosition.y * -0.5f - 0.5f;
	
	for (int i = 0; i < lightNum; i++) {
		PointLight light = PointLightBuffer[i];
		float radius = light.radius * 1.0f;
		float4 pos = float4(light.pos, 1.0f);
		pos = mul(Matrix.view, pos);
		pos.z = -pos.z;
		
		float3 aabb = (maxClip + minClip) * 0.5f;
		
		float sqLen = 0.0f;
		
		for (int j = 0; j < 3; j++) {
			if (pos[j] < minClip[j])
				sqLen += (pos[j] - minClip[j]) * (pos[j] - minClip[j]);
			if (pos[j] > maxClip[j])
				sqLen += (pos[j] - maxClip[j]) * (pos[j] - maxClip[j]);
		}
		if (sqLen < radius * radius) {
			
			LightIndexBuffer[lightCount + threadIndex * 256] = i;
			
			lightCount++;
		}
	}
	
	IndexOffsetBuffer[threadIndex] = lightCount;
}