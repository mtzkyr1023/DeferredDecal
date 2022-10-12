
static const int LENGTH = 256;

cbuffer BlurInfo : register(b0) {
	float blurSize;
	float2 screenInfo;
	float padding1;
	float4x3 padding2;
	float4x4 padding3[3];
}

Texture2D<float4> sourceTexture : register(t0);
RWTexture2D<float4> destinationTexture : register(u0);

groupshared float4 temporaryBuffer[LENGTH * 2];

[numthreads(LENGTH,1,1)]
void main(uint3 id : SV_DispatchThreadID, uint3 gtid : SV_GroupThreadID) {
	temporaryBuffer[gtid.x] = sourceTexture[id.xy];
	temporaryBuffer[min(gtid.x+ LENGTH, LENGTH * 2 - 1)] =
		sourceTexture[min(id.xy + uint2(LENGTH, 0), (uint2)screenInfo.xy - uint2(1, 1))];
	
	GroupMemoryBarrierWithGroupSync();
	
	float4 color = (float4)0;
	
	int i = 0;
	int j = 0;
	
	for (i = gtid.x; i < min(gtid.x + 8, LENGTH * 2 - 1); i++) {
		color += temporaryBuffer[i];
		j++;
	}
	
	color /= (float)j;
	
	
	destinationTexture[id.xy] = color;
}