#define ID_NULL (-1)
#define RESULT_QUANTIZATION_FACTOR (255.0f)

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

struct Intersection
{
	int shapeid;
	int primid;
	int padding0;
	int padding1;
	float4 uvwt;
};

struct Options
{
	int SampleCount;
	int TextureWidth;
	int UseAnyHit;
	float FalloffDistance;
};

struct ResultValue
{
	int QuantizedTotal;
	int QuantizedTotalWeight;
};

StructuredBuffer<TexelRasterization> Texels : register(t0);
StructuredBuffer<Intersection> Intersections : register(t1);
StructuredBuffer<ray> Rays : register(t2);
RWStructuredBuffer<ResultValue> Results : register(u0);

Options Constants : register(b0);

float calcWeight(float3 texelNormal, float3 rayDirection)
{
	return dot(texelNormal, rayDirection);
}

float DistanceToRadiance(float distance) {
	return distance / Constants.FalloffDistance;
}

[numthreads(1, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	int texelIndex = DTid.x;
	for (int i = 0; i < Constants.SampleCount; i++)
	{
		int rayIndex = texelIndex * Constants.SampleCount + i;

		float weight = calcWeight(Texels[texelIndex].Normal, Rays[rayIndex].d.xyz);
		float result = 0.0f;
		if (Constants.UseAnyHit != 0)
		{
			result = (Intersections[rayIndex].shapeid != ID_NULL) ? 0.0f : 1.0f;
		}
		else
		{
			if (Intersections[rayIndex].shapeid != ID_NULL && Intersections[rayIndex].uvwt.w < Constants.FalloffDistance)
			{
				result = DistanceToRadiance(Intersections[rayIndex].uvwt.w);
			}
			else
			{
				result = 1.0f;
			}
		}

		int quantizedResult = (int)(result * weight * RESULT_QUANTIZATION_FACTOR);
		int quantizedWeight = (int)(weight * RESULT_QUANTIZATION_FACTOR);
		InterlockedAdd(Results[Texels[texelIndex].TexelY * Constants.TextureWidth + Texels[texelIndex].TexelX].QuantizedTotal, quantizedResult);
		InterlockedAdd(Results[Texels[texelIndex].TexelY * Constants.TextureWidth + Texels[texelIndex].TexelX].QuantizedTotalWeight, quantizedWeight);
	}
}