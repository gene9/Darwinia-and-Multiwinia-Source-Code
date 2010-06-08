float4x4 worldViewProjMatrix;

void vs_main(
	in float4 iPos : POSITION,
	in float3 iNorm : NORMAL,
	in float4 iColor : COLOR,
	out float4 oPosWorld : TEXCOORD1,
	out float4 oPos : POSITION,
	out float4 oReflCoord : TEXCOORD0,
	out float4 oColor : COLOR
	)
{
	oPosWorld = iPos;
	oPos = mul(iPos,worldViewProjMatrix);
	oReflCoord = mul(iPos+float4(iNorm,0),worldViewProjMatrix);
	oColor = iColor;
}
