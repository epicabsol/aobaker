#pragma pack_matrix(row_major)

struct VS_IN
{
	float3 Position : POSITION;
	float2 TexCoord : TEXCOORD0;
};

struct VS_OUT
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

PostprocessConstants Constants : register(b0);

VS_OUT main(VS_IN input)
{
	VS_OUT result;
	result.Position = float4(input.Position, 1.0f);
	result.TexCoord = input.TexCoord;

	return result;
}