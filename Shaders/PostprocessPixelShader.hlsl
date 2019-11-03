#pragma pack_matrix(row_major)

struct PS_IN
{
	float4 Position : SV_POSITION;
	float2 TexCoord : TEXCOORD0;
};

struct PostprocessConstants
{
	uint ResolutionX;
	uint ResolutionY;
	uint BleedDistance;
	uint DenoiseDistance;
};

struct ResultValue
{
	int QuantizedTotal;
	int QuantizedTotalWeight;
};

#define DENOISE_DOT_THRESHOLD (0.3f)

PostprocessConstants Constants : register(b0);
StructuredBuffer<ResultValue> ResultBuffer : register(t0);
Texture2D NormalVTexture : register(t1);

SamplerState Sampler {

};

float SampleValue(int2 px)
{
	ResultValue bakeResult = ResultBuffer[px.y * Constants.ResolutionX + px.x];
	return ((bakeResult.QuantizedTotal * 255) / bakeResult.QuantizedTotalWeight) / 255.0f;
}

float CalculateValue(int2 px, float3 normal)
{
	float totalWeight = 0.0f;
	float totalValue = 0.0f;

	for (uint y = px.y - Constants.DenoiseDistance; y <= px.y + Constants.DenoiseDistance; y++)
	{
		for (uint x = px.x - Constants.DenoiseDistance; x <= px.x + Constants.DenoiseDistance; x++)
		{
			float4 normalV = NormalVTexture.SampleLevel(Sampler, float2(x / (float)Constants.ResolutionX, /*1.0f -*/ (y / (float)Constants.ResolutionY)), 0);
			if (dot(normalV.xyz, normal) >= 1.0f - DENOISE_DOT_THRESHOLD)
			{
				ResultValue bakeResult = ResultBuffer[y * Constants.ResolutionX + x];
				//return ((bakeResult.QuantizedTotal * 255) / bakeResult.QuantizedTotalWeight) / 255.0f;
				if (bakeResult.QuantizedTotalWeight > 0)
				{
					totalWeight += bakeResult.QuantizedTotalWeight / 255.0f;
					totalValue += bakeResult.QuantizedTotal / 255.0f;
				}

			}
		}
	}

	if (totalWeight > 0.00001f)
	{
		return totalValue / totalWeight;
	}
	else
	{
		return 0.0f;
	}
}

float4 main(PS_IN input) : SV_TARGET
{

	int2 px = float2(input.TexCoord.x * Constants.ResolutionX, (1.0f - input.TexCoord.y) * Constants.ResolutionY);
	ResultValue bakeResult = ResultBuffer[px.y * Constants.ResolutionX + px.x];
	float result = 0.0f;
	if (bakeResult.QuantizedTotalWeight == 0)
	{
		// This pixel wasn't covered - bleed in the closest value by checking successively larger rings
		//float closestValue = 0.0f;
		uint closestDistanceSquared = (Constants.BleedDistance + 1) * (Constants.BleedDistance + 1); // Higher than any actual distance
		/*for (int halfRadius = 1; halfRadius <= Constants.BleedDistance; halfRadius++)
		{
			for (int x = max(0, px.x - halfRadius))
		}*/

		for (int x = px.x - Constants.BleedDistance; x <= px.x + (int)Constants.BleedDistance; x++)
		{
			for (int y = px.y - Constants.BleedDistance; y <= px.y + (int)Constants.BleedDistance; y++)
			{
				// Calculate well-wrapped pixel coordinates. Wouldn't it be nice if '%' worked like this?
				int realx = x % Constants.ResolutionX;
				int realy = y % Constants.ResolutionY;
				if (realx < 0)
					realx += Constants.ResolutionX;
				if (realy < 0)
					realy += Constants.ResolutionY;

				ResultValue neighborResult = ResultBuffer[realy * Constants.ResolutionX + realx];
				if (neighborResult.QuantizedTotalWeight > 0)
				{
					uint distanceSquared = ((int)x - (int)px.x) * ((int)x - (int)px.x) + ((int)y - (int)px.y) * ((int)y - (int)px.y);
					if (distanceSquared < closestDistanceSquared && distanceSquared < Constants.BleedDistance * Constants.BleedDistance)
					{
						closestDistanceSquared = distanceSquared;
						//closestValue = 
						float4 normalV = NormalVTexture.SampleLevel(Sampler, float2(realx / (float)Constants.ResolutionX, realy / (float)Constants.ResolutionY), 0);
						result = CalculateValue(int2(x, y), normalV.xyz);
					}
				}
			}
		}
	}
	else
	{
		//result = ((bakeResult.QuantizedTotal * 255) / bakeResult.QuantizedTotalWeight) / 255.0f;
		float4 normalV = NormalVTexture.SampleLevel(Sampler, float2(input.TexCoord.x, 1.0f - input.TexCoord.y), 0);
		result = CalculateValue(px, normalV.xyz);
	}
	//float4 normalV = NormalVTexture.Sample(Sampler, input.TexCoord);
	//float result = ((bakeResult.QuantizedTotal * 255) / bakeResult.QuantizedTotalWeight) / 255.0f;
	return float4(result, 0.0f, 0.0f, 1.0f);
}