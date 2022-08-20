static const int CUBE_SIZE = 256;


RWTexture2DArray<float4> srcTex : register(u0);
RWTexture2DArray<float4> destTex : register(u1);

[numthreads(16, 16, 1)]
void main(uint3 id : SV_DispatchThreadID, uint3 gid : SV_GroupThreadID) {
	
	float4 color = (float4)0;
	
	float inv = 1.0f / (16.0f * 16.0f);
	
	for (int i = 0; i < 16; i++) {
		for(int j = 0; j < 16; j++) {
			color += srcTex[id.xyz * uint3(16, 16, 1) + uint3(j, i, 0)];
		}
	}
	
	destTex[id.xyz] = color * inv;
}