shared float4x4 matCameraView : register( c0 );
shared float4x4 matCameraViewIT : register( c4 );
float4x4 matProj;

float2 fogRange;

float4x4 matWorldView;
float4x4 matWorldViewIT;

void vs_main(
	in  float4 iPosition  : POSITION0,
	in  float4 iColor     : COLOR0,
	in  float2 iTex0      : TEXCOORD0,
	out float4 oPos       : POSITION,
	out float4 oColor     : TEXCOORD1,
	out float2 oTex0      : TEXCOORD0
	)
{
	float4x4 mWV = mul(matWorldView, matCameraView);
	float4 P0 = mul(iPosition, mWV);
	oPos = mul(P0, matProj);

	oTex0 = iTex0;

	float d = oPos.z;
	float fog = saturate((fogRange.x - d)/fogRange.y);	
	oColor = iColor * fog;
}
