void vs_main(
	in float4 iPos : POSITION,
	out float4 oPos : POSITION,
	out float2 oUv : TEXCOORD0
	)
{
	oPos = iPos;
	oUv.x = 0.5*(1+iPos.x)+0.0003;
	oUv.y = 0.5*(1-iPos.y)+0.0003;
	// +0.0003 protects against ugly interference on Radeon X300
}
