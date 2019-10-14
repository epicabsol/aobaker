struct TexelRasterization
{
	int TexelX;
	int TexelY;
	float3 Position;
	float2 TexCoord;
	float3 Normal;
};

struct ray
{
	float4 o;
	float4 d;
	int2 extra;
	int doBackfaceCulling;
	int padding;
};

struct Options
{
	int SampleCount;
	float3 Unused;
};

StructuredBuffer<TexelRasterization> Texels : register(t0);
StructuredBuffer<float3> Hemisphere : register(t1);
RWStructuredBuffer<ray> Rays : register(u0);

Options Constants : register(b0);

[numthreads(1, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	float3 hemiY = Texels[DTid.x].Normal;
	float3 hemiX = float3(1.0f, 0.0f, 0.0f);
	float3 hemiZ = float3(0.0f, 0.0f, 1.0f);
	if (hemiY.y < 0.9999 && hemiY.y > -0.9999)
	{
		hemiX = normalize(cross(hemiY, float3(0.0f, 1.0f, 0.0f)));
		hemiZ = normalize(cross(hemiX, hemiY));
	}

	for (int i = 0; i < Constants.SampleCount; i++)
	{
		float3 hemi = Hemisphere[DTid.x * Constants.SampleCount + i];
		float3 dir = float3(hemi.x * hemiX.x + hemi.y * hemiY.x + hemi.z * hemiZ.x, hemi.x * hemiX.y + hemi.y * hemiY.y + hemi.z * hemiZ.y, hemi.x * hemiX.z + hemi.y * hemiY.z + hemi.z * hemiZ.z);
		float3 pos = Texels[DTid.x].Position + /*hemiY*/dir * 0.001f;

		Rays[DTid.x * Constants.SampleCount + i].o = float4(pos, 1000000.0f); // Start position, max t
		Rays[DTid.x * Constants.SampleCount + i].d = float4(dir, 0.0f); // Direction, motionblur time
		Rays[DTid.x * Constants.SampleCount + i].extra = int2(0, -842150451); // No idea what this thing is, but it's necessary.
		Rays[DTid.x * Constants.SampleCount + i].doBackfaceCulling = 0;
		Rays[DTid.x * Constants.SampleCount + i].padding = 0;
	}
}