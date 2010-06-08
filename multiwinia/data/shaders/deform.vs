void vs_main(
	in float4 iPos : POSITION,
	in float2 iUv0 : TEXCOORD0, // center
	in float2 iUv1 : TEXCOORD1, // invSize, height
	in float2 iColor : COLOR, // time
	out float4 oPos : POSITION,
	out float4 oColor : COLOR,
	out float3 oUv : TEXCOORD0
	)
{
	oPos = iPos;
	oColor = float4(iUv0.x,iUv0.y,iUv1.x,iUv1.y);
	oUv.x = 0.5*(1+iPos.x)+0.0003;
	oUv.y = 0.5*(1-iPos.y)+0.0003;
	// +0.0003 protects against ugly interference on Radeon X300
	oUv.z = iColor.x;
}
