#pragma pack_matrix(row_major)

struct PS_IN
{
	float4 pos : SV_POSITION;
	float3 norm : TEXCOORD0;
	float2 uv : TEXCOORD1;
	float3 worldpos : TEXCOORD2;
};

struct PS_OUT
{
	float4 PositionU : SV_Target0;
	float4 NormalV : SV_Target1;
};

PS_OUT main(PS_IN input)
{
	PS_OUT result = (PS_OUT)0;
	result.PositionU = float4(input.worldpos, input.uv.x);
	result.NormalV = float4(input.norm, input.uv.y);
	return result;
}