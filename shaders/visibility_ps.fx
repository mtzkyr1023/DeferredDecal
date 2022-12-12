

cbuffer MaterialId : register(b1) {
	uint materialId;
	float4x3 padding1;
	matrix padding2;
	matrix padding3;
	matrix padding4;
}


static const uint InstanceIdBits = 0;


struct PS_OUT {
	uint2 vis : SV_TARGET0;
	float4 nor : SV_TARGET1;
};

PS_OUT main(in float4 pos : SV_POSITION, in float3 nor : NORMAL0,
	in uint triId : SV_PrimitiveID) {
	PS_OUT output = (PS_OUT)0;
	output.vis = uint2(materialId, triId);
	output.nor = float4((float)(materialId % 8) / 8.0f, 0.0f, 0.0f, 0.0f);
	
	return output;
	//return triId;
}