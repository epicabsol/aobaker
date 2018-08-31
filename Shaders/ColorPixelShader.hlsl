#pragma pack_matrix(row_major)

struct PS_IN
{
	float4 pos : SV_POSITION;
	float3 norm : TEXCOORD0;
	float2 uv : TEXCOORD1;
	float4 color : COLOR0;
};

float4 main(PS_IN input) : SV_TARGET
{
	return input.color;
}