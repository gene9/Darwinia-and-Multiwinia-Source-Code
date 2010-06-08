shared float4x4 matCameraView : register( c0 );
shared float4x4 matCameraViewIT : register( c4 );

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
	// float4x4 mWV = mul(matWorldView, matCameraView);
	float4x4 mWVP = worldViewProjMatrix; // mul(mWV, matProj);	

	oPosWorld = iPos;
	oPos = mul(iPos,mWVP);
	oReflCoord = mul(iPos+float4(iNorm,0),mWVP);
	oColor = iColor;
}
