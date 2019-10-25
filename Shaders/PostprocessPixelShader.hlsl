#pragma pack_matrix(row_major)

struct PS_IN
{
	float4 Position : SV_POSITION;
	float2 TexCoord : TEXCOORD0;
};

struct PostprocessConstants
{
	int ResolutionX;
	int ResolutionY;
	int BleedDistance;
	int _Padding;
};

struct ResultValue
{
	int QuantizedTotal;
	int QuantizedTotalWeight;
};

PostprocessConstants Constants : register(b0);
StructuredBuffer<ResultValue> ResultBuffer : register(t0);
Texture2D NormalVTexture : register(t1);

SamplerState Sampler {

};

float CalculateValue(int2 px)
{
	ResultValue bakeResult = ResultBuffer[px.y * Constants.ResolutionX + px.x];
	return ((bakeResult.QuantizedTotal * 255) / bakeResult.QuantizedTotalWeight) / 255.0f;
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
		int closestDistanceSquared = (Constants.BleedDistance + 1) * (Constants.BleedDistance + 1); // Higher than any actual distance
		/*for (int halfRadius = 1; halfRadius <= Constants.BleedDistance; halfRadius++)
		{
			for (int x = max(0, px.x - halfRadius))
		}*/

		for (int x = px.x - Constants.BleedDistance; x <= px.x + Constants.BleedDistance; x++)
		{
			for (int y = px.y - Constants.BleedDistance; y <= px.y + Constants.BleedDistance; y++)
			{
				// Calculate well-wrapped pixel coordinates. Wouldn't it be nice if '%' worked like this?
				int realx = x % Constants.ResolutionX;
				int realy = y % Constants.ResolutionY;
				if (realx < 0)
					realx += Constants.ResolutionX;
				if (realy < 0)
					realy += Constants.ResolutionY;

				ResultValue neighborResult = ResultBuffer[y * Constants.ResolutionX + x];
				if (neighborResult.QuantizedTotalWeight > 0)
				{
					int distanceSquared = (x - px.x) * (x - px.x) + (y - px.y) * (y - px.y);
					if (distanceSquared < closestDistanceSquared && distanceSquared < Constants.BleedDistance * Constants.BleedDistance)
					{
						closestDistanceSquared = distanceSquared;
						//closestValue = 
						result = CalculateValue(int2(x, y));
					}
				}
			}
		}
	}
	else
	{
		result = ((bakeResult.QuantizedTotal * 255) / bakeResult.QuantizedTotalWeight) / 255.0f;
	}
	//float4 normalV = NormalVTexture.Sample(Sampler, input.TexCoord);
	//float result = ((bakeResult.QuantizedTotal * 255) / bakeResult.QuantizedTotalWeight) / 255.0f;
	return float4(result, 0.0f, 0.0f, 1.0f);
}