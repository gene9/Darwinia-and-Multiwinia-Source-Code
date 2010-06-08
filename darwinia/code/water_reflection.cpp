#include "lib/universal_include.h"

#ifdef USE_DIRECT3D

#include "lib/opengl_directx_internals.h"
#include "lib/preferences.h"
#include "lib/profiler.h"
#include "lib/shader.h"
#include "lib/texture.h"
#include "app.h"
#include "camera.h"
#include "location.h"
#include "particle_system.h"
#include "renderer.h"
#include "water_reflection.h"

namespace OpenGLD3D {
	extern D3DPRESENT_PARAMETERS g_d3dpp;
};

WaterReflectionEffect* WaterReflectionEffect::Create()
{
	if(!OpenGLD3D::g_supportsHwVertexProcessing) return NULL;
	WaterReflectionEffect* w = new WaterReflectionEffect();
	if(!w->m_waterReflectionTexture || !w->m_waterMeshWavesShader)
		SAFE_DELETE(w);
	return w;
}

WaterReflectionEffect::WaterReflectionEffect()
{
	m_waterReflectionTexture = Texture::Create(TextureParams(g_app->m_renderer->ScreenW()/2,g_app->m_renderer->ScreenH()/2,OpenGLD3D::g_d3dpp.BackBufferFormat,TF_RENDERTARGET));
	LPCSTR prof = D3DXGetPixelShaderProfile(OpenGLD3D::g_pd3dDevice);
	bool hasPs3 = prof && prof[0]=='p' && prof[1]=='s' && prof[2]=='_' && prof[3]>'2';
	m_waterMeshWavesShader = hasPs3 ? Shader::Create("shaders/water-meshwaves.vs","shaders/water-meshwaves.ps3") : NULL;
	if(!m_waterMeshWavesShader)
	{
		m_waterMeshWavesShader = Shader::Create("shaders/water-meshwaves.vs","shaders/water-meshwaves.ps");
	}
}

void WaterReflectionEffect::PreRenderWaterReflection()
{
	START_PROFILE(g_app->m_profiler, "Water Pre-render");

	//
	// Prepare render target

	IDirect3DSurface9* oldRenderTarget;
	OpenGLD3D::g_pd3dDevice->GetRenderTarget(0,&oldRenderTarget);
	OpenGLD3D::g_pd3dDevice->SetRenderTarget(0,m_waterReflectionTexture->GetRenderTarget());
	glViewport( 0, 0, m_waterReflectionTexture->GetParams().m_w, m_waterReflectionTexture->GetParams().m_h );
	GLdouble plane[4] = {0,1,0,0};
	glClipPlane(GL_CLIP_PLANE2,plane); // planes 0 and 1 used by radar
	glEnable(GL_CLIP_PLANE2);

	//
	// Clipping hack start

	glMatrixMode(GL_MODELVIEW);
	void SwapToViewMatrix();
	void SwapToModelMatrix();
	SwapToViewMatrix();

	//
	// Clear

	START_PROFILE(g_app->m_profiler, "Render Clear");
	glClearColor(0.3f,0.3f,0.3f,0);
	glClear	(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	glClearColor(0,0,0,0);
	END_PROFILE(g_app->m_profiler, "Render Clear");

	//
	// Render mirrored scene without water, spiritreceiver, details

	m_prerendering = true;
	/*int buildingDetail = g_prefsManager->GetInt( "RenderBuildingDetail", 1 );
	g_prefsManager->SetInt( "RenderBuildingDetail", 3 );
	int entityDetail = g_prefsManager->GetInt( "RenderEntityDetail", 1 );
	g_prefsManager->SetInt( "RenderEntityDetail", 3 );
	int landscapeDetail = g_prefsManager->GetInt( "RenderLandscapeDetail", 1 );
	g_prefsManager->SetInt( "RenderLandscapeDetail", 3 );*/
	g_app->m_camera->WaterReflect();
	g_app->m_location->WaterReflect();
	g_app->m_location->SetupLights();
	g_app->m_renderer->SetupMatricesFor3D();
	g_app->m_renderer->UpdateTotalMatrix();
	g_app->m_location->Render( false );
	// optional: reflect particles
	//g_app->m_particleSystem->Render();
	g_app->m_camera->WaterReflect();
	g_app->m_location->WaterReflect();
	g_app->m_renderer->SetupMatricesFor3D();
	g_app->m_renderer->UpdateTotalMatrix();
	g_app->m_location->SetupLights();
	/*g_prefsManager->SetInt( "RenderLandscapeDetail", landscapeDetail );
	g_prefsManager->SetInt( "RenderEntityDetail", entityDetail );
	g_prefsManager->SetInt( "RenderBuildingDetail", buildingDetail );*/
	m_prerendering = false;

	//
	// Clipping hack end

	SwapToModelMatrix();

	//
	// Restore render target

	glEnable(GL_CULL_FACE);
	glDisable(GL_CLIP_PLANE2);
	OpenGLD3D::g_pd3dDevice->SetRenderTarget(0,oldRenderTarget);
	oldRenderTarget->Release();
	glViewport( 0, 0, g_app->m_renderer->ScreenW(), g_app->m_renderer->ScreenH() );

	CHECK_OPENGL_STATE();
	END_PROFILE(g_app->m_profiler, "Water Pre-render");
}

void WaterReflectionEffect::Start()
{
	m_waterMeshWavesShader->Bind();
	m_waterMeshWavesShader->SetUniform("time",(GetTickCount()%10000000)/1000.0f,0.001f*1024/g_app->m_renderer->ScreenW());
	m_waterMeshWavesShader->SetMatrix("worldViewProjMatrix",g_app->m_renderer->GetTotalMatrix());
	int sampler = m_waterMeshWavesShader->SetSampler("water",m_waterReflectionTexture);
	if(sampler>=0)
	{
		OpenGLD3D::g_pd3dDevice->SetSamplerState(sampler,D3DSAMP_MAGFILTER,D3DTEXF_LINEAR);	
		OpenGLD3D::g_pd3dDevice->SetSamplerState(sampler,D3DSAMP_MINFILTER,D3DTEXF_LINEAR);	
		OpenGLD3D::g_pd3dDevice->SetSamplerState(sampler,D3DSAMP_MIPFILTER,D3DTEXF_NONE);	
	}
}

void WaterReflectionEffect::Stop()
{
	m_waterMeshWavesShader->SetSampler("water",NULL);
	m_waterMeshWavesShader->Unbind();
}

WaterReflectionEffect::~WaterReflectionEffect()
{
	SAFE_DELETE(m_waterReflectionTexture);
	SAFE_DELETE(m_waterMeshWavesShader);
}

WaterReflectionEffect* g_waterReflectionEffect = NULL;

#endif
