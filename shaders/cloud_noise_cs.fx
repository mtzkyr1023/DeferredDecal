
float random(float3 texCoord) {
	return frac(sin(dot(texCoord, float3(12.9898, 78.233, 12.9898))) * 43758.5453);
}


float hash( float n )
{
    return frac(sin(n)*43758.5453);
}

float noise( float3 x )
{
    // The noise function returns a value in the range -1.0f -> 1.0f

    float3 p = floor(x);
    float3 f = frac(x);

    f       = f*f*(3.0-2.0*f);
    float n = p.x + p.y*57.0 + 113.0*p.z;

    return lerp(lerp(lerp( hash(n+0.0), hash(n+1.0),f.x),
                   lerp( hash(n+57.0), hash(n+58.0),f.x),f.y),
               lerp(lerp( hash(n+113.0), hash(n+114.0),f.x),
                   lerp( hash(n+170.0), hash(n+171.0),f.x),f.y),f.z);
}

RWTexture3D<float> dstTex : register(u0);

[numthreads(8,8,8)]
void main(uint3 DTID : SV_DispatchThreadID, uint3 GI : SV_GroupID, uint3 GTID : SV_GroupThreadID) {
	float len = 10000.0f;
	float3 pos;
	float tmpLen;
	float tmax = 0.0f;
	
	for (int i = -3; i < 4; i++) {
		for (int j = -3; j < 4; j++) {
			for (int k = -3; k < 4; k++) {
				pos = float3(noise((float3)GI.xyz + float3(i,j,k)) * 7.5f + 7.5f + (float)i * 8.0f,
							noise((float3)GI.xyz + float3(i,j,k)) * 7.5f + 7.5f + (float)j * 8.0f,
							noise((float3)GI.xyz + float3(i,j,k)) * 7.5f + 7.5f + (float)k * 8.0f);
				tmpLen = length((float3)GTID.xyz - pos.xyz);
				len = min(tmpLen, len);
				tmax = max(tmpLen, tmax);
			}
		}
	}
	
	float dst = tmax - len;
	dst *= dst;
	
	dstTex[DTID] = 1.0f - exp(-pow(1.0f - len / pow(dst * 2, 0.5f), 8.0f));
}