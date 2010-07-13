shared float4x4 matCameraView : register( c0 );
shared float4x4 matCameraViewIT : register( c4 );
float4x4 matProj;

struct CLight
{
   float3 vDir;
   float4 vDiffuse;
   float4 vSpecular;
};

struct COLOR_PAIR
{
   float4 Color         : COLOR0;
   float4 ColorSpec     : COLOR1;
};

bool     bSpecular;
float2   fogRange;
int      numLights;
CLight   lights[2];

float4x4 matWorldView;
float4x4 matWorldViewIT;


COLOR_PAIR DoDirLight(float3 N, float3 V, int i)
{
   COLOR_PAIR Out;
   float3 L = -lights[i].vDir;
   float NdotL = dot(N, L);
   Out.Color = 0;
   Out.ColorSpec = 0;
   if(NdotL > 0.f)
   {
      //compute diffuse color
      Out.Color += NdotL * lights[i].vDiffuse;

      //add specular component
      if(bSpecular)
      {
         float3 H = normalize(L + V);
         Out.ColorSpec = pow(max(0, dot(H, N)), 16) * lights[i].vSpecular;
      }
   }
   return Out;
}

void vs_main(
	in  float4 iPosition  : POSITION0,
	in  float3 iNormal    : NORMAL0,
	in  float4 iColor     : COLOR0,
	out float4 oPos       : POSITION,
	out float4 oColor     : TEXCOORD0
	)
{
	float4x4 mWV = mul(matWorldView, matCameraView);
	float4 P0 = mul(iPosition, mWV);
	oPos = mul(P0, matProj);

	float4x4 mWVIT = mul(matCameraViewIT, matWorldViewIT);		

	float3 P = P0;           //position in view space	

	//directional lights
	if(numLights<=0)
	{
		oColor = iColor;
	}
	else
	{
		float3 N = normalize(mul((float3x3)mWVIT, normalize(iNormal))+float3(0,0.0001,0)); //normal in view space
		float3 V = normalize(P);                           //viewer
		oColor = 0;
		for(int i = 0; i < numLights; i++)
		{
			COLOR_PAIR ColOut = DoDirLight(N, V, i);
			oColor += ColOut.Color * iColor + ColOut.ColorSpec;
		}

		oColor.a = 1; // fixes world map
	}

	float d = oPos.z; 
	float fog = saturate((fogRange.x - d)/fogRange.y);
	
	oColor *= fog;
}
