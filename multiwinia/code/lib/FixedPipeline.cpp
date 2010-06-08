#include "lib/universal_include.h"

#ifdef USE_DIRECT3D

#include "App.h"
#include "lib/debug_utils.h"
#include "FixedPipeline.h"
#include "lib/opengl_directx_internals.h"
#include "ogl_extensions.h"
#include "Renderer.h"
#include "Shader.h"
#include "Texture.h"


/////////////////////////////////////////////////////////////////////////////
//
// FixedPipeline

FixedPipeline::FixedPipeline()
{
	memset(this,0,sizeof(*this));

	m_enabled = true;

	m_fixedShader[0] = Shader::Create("shaders/FixedFunc0.vs","shaders/FixedFunc0Maps.ps");
	m_fixedShader[1] = Shader::Create("shaders/FixedFunc1.vs","shaders/FixedFunc1Maps.ps");
	m_fixedShader[2] = Shader::Create("shaders/FixedFunc2.vs","shaders/FixedFunc2Maps.ps");

	OpenGLD3D::g_pd3dDevice->SetRenderState(D3DRS_LIGHTING, false); 

	// Set the camera view (identity matrix to start)
	D3DXMATRIX mIdentity;

	AppDebugOut("Setting Camera View Matrix...\n" );
	D3DXMatrixIdentity(&mIdentity);
	OpenGLD3D::g_pd3dDevice->SetVertexShaderConstantF(0, (FLOAT *) &mIdentity, 4 );
	OpenGLD3D::g_pd3dDevice->SetVertexShaderConstantF(4, (FLOAT *) &mIdentity, 4 );
}

void FixedPipeline::Bind()
{
	if(!m_enabled) return;

	gglActiveTextureARB(GL_TEXTURE1_ARB);
	bool tex1 = glIsEnabled(GL_TEXTURE_2D);
	gglActiveTextureARB(GL_TEXTURE0_ARB);
	bool tex0 = glIsEnabled(GL_TEXTURE_2D);

	Shader* shader = m_fixedShader[ tex0?(tex1?2:1):0 ];

	// bind shader
	bool resetAll = false;
	if(shader!=m_bound)
	{
		Unbind();
		resetAll = true;
		m_bound = shader;
		m_bound->Bind();
	}

	// set camera
	const D3DXMATRIX& getWorldViewMatrix();
	const D3DXMATRIX& getProjMatrix();
	D3DXMATRIX matProj = getProjMatrix();
	D3DXMATRIX matWorld = getWorldViewMatrix();
	D3DXMATRIXA16 invMat;
	m_bound->SetMatrix("matWorldView", &matWorld._11);
	D3DXMatrixInverse(&invMat, NULL, &matWorld);
	m_bound->SetMatrix("matWorldViewIT", &invMat._11);

	// D3DXMatrixMultiply(&compMat, &matWorld, &matProj);
	// m_bound->SetMatrix("matWorldViewProj", &compMat._11);
	m_bound->SetMatrix("matProj", (FLOAT *) &matProj );

	//// Set the projection matrix
	//D3DXMATRIXA16 matProjT;
	//D3DXMatrixTranspose( &matProjT, &matProj );
	//OpenGLD3D::g_pd3dDevice->SetVertexShaderConstantF( 8, (FLOAT *) &matProjT, 4 );

	// set material
	if(resetAll || m_state.m_specular!=m_oldState.m_specular)
		m_bound->SetBool("bSpecular",m_state.m_specular);
	// samplers are always already correctly set (but it is not guaranteed)
	//if(tex0)
	//	m_bound->SetSamplerGLFixedStage("map0",0);
	//if(tex1)
	//	m_bound->SetSamplerGLFixedStage("map1",1);

	// set lights
	m_state.m_numLights = 0;
	if(m_lighting && m_lightEnabled[0])
	{
		m_state.m_numLights++;
		if(m_lightEnabled[1]) m_state.m_numLights++;
	}
	if(resetAll || m_state.m_numLights!=m_oldState.m_numLights)
		m_bound->SetInt("numLights",m_state.m_numLights);
	for(int i=0; i<m_state.m_numLights; i++)
	{
		char tmp[100];
		if(resetAll || m_oldState.m_numLights<=i || memcmp(&m_state.m_light[i].Direction,&m_oldState.m_light[i].Direction,sizeof(D3DVECTOR)))
		{
			sprintf(tmp, "lights[%i].vDir", i);
			m_bound->SetFloatArray(tmp, &m_state.m_light[i].Direction.x, 3);
		}
		if(resetAll || m_oldState.m_numLights<=i || memcmp(&m_state.m_light[i].Diffuse,&m_oldState.m_light[i].Diffuse,sizeof(D3DCOLORVALUE)))
		{
			sprintf(tmp, "lights[%i].vDiffuse", i);
			m_bound->SetFloatArray(tmp, &m_state.m_light[i].Diffuse.r, 4);
		}
		if(resetAll || m_oldState.m_numLights<=i || memcmp(&m_state.m_light[i].Specular,&m_oldState.m_light[i].Specular,sizeof(D3DCOLORVALUE)))
		{
			sprintf(tmp, "lights[%i].vSpecular", i);
			m_bound->SetFloatArray(tmp, &m_state.m_light[i].Specular.r, 4);
		}
	}

	// set fog
	m_state.m_fogRange[0] = m_fogEnabled ? m_fogEnd : 100000;
	m_state.m_fogRange[1] = m_fogEnabled ? m_fogEnd-m_fogStart : 1000;
	if(resetAll || memcmp(&m_state.m_fogRange,&m_oldState.m_fogRange,8))
		m_bound->SetFloatArray("fogRange", m_state.m_fogRange, 2);

	// set environment
	if(resetAll || m_state.m_environmentDiffuse!=m_oldState.m_environmentDiffuse || m_state.m_environmentSpecular!=m_oldState.m_environmentSpecular)
	{
		m_bound->SetSamplerTex("environmentDiffuse",m_state.m_environmentDiffuse);
		m_bound->SetSamplerTex("environmentSpecular",m_state.m_environmentSpecular);
	}

	// backup current GPU state
	m_oldState = m_state;

	// check that state is supported
	unsigned type = tex0?(tex1?2:1):0 + 3*m_state.m_numLights + (m_state.m_specular?9:0);
	//AppDebugAssert(type==0 || type==1 || type==2 || type==3 || type==6 || type==15);
	//static unsigned hist[19]={0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0};
	//hist[type]++;
}

void FixedPipeline::Unbind()
{
	if(m_bound)
	{
		m_bound->Unbind();
		m_bound = NULL;
	}
}

void FixedPipeline::Disable()
{
	Unbind();
	AppDebugAssert(m_enabled);
	m_enabled = false;
}

void FixedPipeline::Enable()
{
	AppDebugAssert(!m_enabled);
	m_enabled = true;
}

FixedPipeline::~FixedPipeline()
{
	Unbind();
	for(unsigned i=0;i<3;i++)
		SAFE_DELETE(m_fixedShader[i]);
}

FixedPipeline* g_fixedPipeline = NULL;

#endif // USE_DIRECT3D
