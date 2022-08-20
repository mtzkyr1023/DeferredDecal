

static const uint SuperSampledAA = 100;
static const uint TriangleSampledAA = 200;
static const uint StencilAA = 440;
static const uint WireframeAA = 450;

static const uint AAMode = SuperSampledAA;

static const float MipMapLodBias = 0.5f;
static const uint SamplesPerPixel = 4;
static const float InvSpp = 0.25f;
static const uint TileSize = 128;
static const uint TileSizeX = 16;
static const uint TileSizeY = 8;
static const uint InstanceIdMask = 1023;
static const uint InstanceIdBits = 0;
static const uint MaxShadingSamplesPerTile = 512;


struct VertexInput
{
   float4 position  ;
   float4 tangent   ;
   float4 bitangent ;
   float4 texCoord  ;
};

struct VertexOutput
{
   float4 position  ;
   float4 tangent   ;
   float4 bitangent ;
   float4 texCoord  ;

   float4 clipSpacePosition : SV_POSITION;
};

cbuffer __SystemConstant : register(b0) {
	matrix InvViewProjMatrix;
	float4 WorldSpaceCameraPosition;
	uint2 ImageSize;
	uint NumLights;
	uint ShaderId;
	matrix Padding1;
	matrix Padding2;
	float4x2 Padding3;
}

Texture2D<uint2> visibilityBuffer : register(t0);
StructuredBuffer<uint4> offsets : register(t1);
StructuredBuffer<uint4> tiles : register(t2);
StructuredBuffer<float4> positionBuffer : register(t3);
StructuredBuffer<float4> normalBuffer : register(t4);
StructuredBuffer<float4> uvBuffer : register(t5);
StructuredBuffer<uint> indexBuffer : register(t6);
StructuredBuffer<uint4> instanceToIndexMap : register(t7);

RWTexture2D<float4> resultTex : register(u0);

VertexInput __loadVertex(uint bufferIndex)
{
   VertexInput vIn;
   vIn.position  = positionBuffer[bufferIndex];
   vIn.tangent   = normalBuffer[bufferIndex];
   vIn.bitangent = float4(0.0f, 0.0f, 0.0f, 0.0f);
	

	
   vIn.texCoord = uvBuffer[bufferIndex];

   return vIn;
}

float3 __intersect(float3 p0, float3 p1, float3 p2, float3 o, float3 d)
{
   float3 eo =  o - p0;
   float3 e2 = p2 - p0;
   float3 e1 = p1 - p0;
   float3 r  = cross(d, e2);
   float3 s  = cross(eo, e1);
   float iV  = 1.0f / dot(r, e1);
   float V1  = dot(r, eo);
   float V2  = dot(s,  d);
   float b   = V1 * iV;
   float c   = V2 * iV;
   float a   = 1.0f - b - c;
   return float3(a, b, c);
}

VertexInput __loadAndInterpolateVertex(uint iOffset, uint vOffset, uint tID, float2 pixelCoord, out float filterWidth)
{
   uint index0 = indexBuffer[iOffset + tID * 3 + 0];
   uint index1 = indexBuffer[iOffset + tID * 3 + 1];
   uint index2 = indexBuffer[iOffset + tID * 3 + 2];

   VertexInput v0 = __loadVertex( index0 + vOffset );
   VertexInput v1 = __loadVertex( index1 + vOffset );
   VertexInput v2 = __loadVertex( index2 + vOffset );
   
   //v0.tangent.xyz = normalize(v0.tangent.xyz + v1.tangent.xyz + v2.tangent.xyz);
   
      
   float4 d = float4(pixelCoord / ImageSize.xy * float2(2.0f, -2.0f) - float2(1.0f, -1.0f), 0.1f, 1.0f);
   d = mul(d, InvViewProjMatrix);
   d /= d.w;
   
   d.xyz = normalize(d.xyz - WorldSpaceCameraPosition.xyz);
      
   float4 p0 = v0.position;
   float4 p1 = v1.position;
   float4 p2 = v2.position;

   /// Compute the barycentric coordinates
   float3 H  = __intersect(p0.xyz, p1.xyz, p2.xyz, WorldSpaceCameraPosition.xyz, -d.xyz);

   VertexInput vIn;
   vIn.position  = mad(v0.position,  H.x, mad(v1.position,  H.y, (v2.position  * H.z)));
   vIn.tangent   = mad(v0.tangent,   H.x, mad(v1.tangent,   H.y, (v2.tangent   * H.z)));
   vIn.bitangent = mad(v0.bitangent, H.x, mad(v1.bitangent, H.y, (v2.bitangent * H.z)));
   vIn.texCoord  = mad(v0.texCoord,  H.x, mad(v1.texCoord,  H.y, (v2.texCoord  * H.z)));

   return vIn;
}




[numthreads(16,8,1)]
void main(uint3 globalThreadId : SV_DispatchThreadID,
	uint3 tileThreadId : SV_GroupThreadID,
	uint3 groupId : SV_GroupID) {
	
	uint tileId = tiles[offsets[ShaderId].x + groupId.x].x;
	const uint2 PixelCoord = uint2(globalThreadId.xy
	);
	
	const uint PixelIndex = PixelCoord.x + ImageSize.x * PixelCoord.y;
	uint groupThreadId = tileThreadId.x + tileThreadId.y * TileSizeX;
	
	uint2 vData = visibilityBuffer[PixelCoord];
	uint tId = vData.x;
	uint iId = vData.y;
	uint4 iData = instanceToIndexMap[iId];
	uint iOffset = iData.x;
	uint vOffset = iData.y;
	uint sId = iData.z;
	//if (sId > 10) return;
	
	float2 ImageCoord = float2(PixelCoord.x + 0.5f, PixelCoord.y + 0.5f) / ImageSize;
	float FilterWidth = 1.0f;
	
	VertexInput vIn = __loadAndInterpolateVertex(iOffset, vOffset, tId, PixelCoord, FilterWidth);
	
	//resultTex[PixelCoord] = float4(FilterWidth, 0.0f, 0.0f, 1.0f);
	resultTex[PixelCoord] = float4(vIn.tangent.xyz, 1.0f);
}