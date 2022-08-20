

cbuffer MaterialId : register(b1) {
	uint materialId;
	float4x3 padding1;
	matrix padding2;
	matrix padding3;
	matrix padding4;
}


static const uint InstanceIdBits = 0;


uint2 main(in float4 pos : SV_POSITION, in uint triId : SV_PrimitiveID) : SV_Target {
	return uint2(triId, materialId);
	//return triId;
}