





Texture2D<uint2> visibilityBuffer : register(t0);


RWByteAddressBuffer materialBuffer : register(u0);

[numthreads(16,8,1)]
void main(uint3 id : SV_DispatchThreadID) {
	uint2 vis = visibilityBuffer.Load(uint3(id.xy, 0));
	
	uint threadID = id.y * 16 + id.x;
	
	materialBuffer.Store(threadID.x * 4, vis.x);
}