#include "lib/universal_include.h"

#ifdef USE_DIRECT3D

#include "app.h"
#include "camera.h"
#include "deform.h"
#include "explosion.h"
#include "location.h"
#include "renderer.h"
#include "lib/opengl_directx_internals.h"
#include "lib/profiler.h"
#include "lib/resource.h"
#include "lib/vector3.h"

#define STRETCH_RECT // should make FSAA possible (for small penalty with non-FSAA setups)

#define CHECK_OK if(!m_deformFullscreenShader) return

namespace OpenGLD3D {
	extern D3DPRESENT_PARAMETERS g_d3dpp;
};

// conserved for future testing several deformation types
//#define SOUL_TYPES 10
//float soulSize [SOUL_TYPES] = {0.37f,0.37f,0.37f,0.32f,0.6f,0.4f,0.60f,0.50f,0.40f};
//float soulDepth[SOUL_TYPES] = {0.10f,0.05f,0.15f,0.12f,0.1f,0.1f,0.05f,0.15f,0.15f};
//int soulType = 0;

struct PunchParams
{
	const char* punchTable;
	float seconds;
	int fadeType;
};

enum PunchFade
{
	PF_OLD,
	PF_NEW,
};

// sucks inwards and returns back
//PunchParams punchParams = {"textures/deform3.bmp",1,PF_OLD};
// ripples
PunchParams punchParams = {"textures/deform36c.bmp",1,PF_NEW};

DeformEffect* DeformEffect::Create()
{
	DeformEffect* d = new DeformEffect();
	if(!d->m_deformFullscreenShader
		|| !d->m_deformSphereShader
		|| !d->m_deformDiamondShader
		|| !d->m_screenTexture 
		|| !d->m_deformTexture
		) SAFE_DELETE(d);
	return d;
}

DeformEffect::DeformEffect()
{
	m_deformFullscreenShader = Shader::Create("shaders/deform.vs","shaders/deformFullscreen.ps");
	m_deformSphereShader = Shader::Create("shaders/deform.vs","shaders/deformSphere.ps");
	m_deformDiamondShader = Shader::Create("shaders/deform.vs","shaders/deformDiamond.ps");
	m_screenTexture = Texture::Create(TextureParams(g_app->m_renderer->ScreenW(),g_app->m_renderer->ScreenH(),OpenGLD3D::g_d3dpp.BackBufferFormat,TF_RENDERTARGET));
	m_deformTexture = Texture::Create(TextureParams(g_app->m_renderer->ScreenW()/2,g_app->m_renderer->ScreenW()/2,D3DFMT_A8R8G8B8,TF_RENDERTARGET));
}

//#define PUNCH_FIRST 10
//#define PUNCH_LAST 36
//static unsigned punchtest = PUNCH_LAST;

void DeformEffect::AddPunch(Vector3 pos, float range)
{
	CHECK_OK;
	Punch punch;
	punch.m_pos = pos;
	punch.m_range = range;
	punch.m_birth = GetTickCount();
	m_punchList.PutData(punch);

//	punchtest++;
//	if(punchtest>PUNCH_LAST) punchtest=PUNCH_FIRST;
//	DebugOut("Doing punch %d.\n",punchtest);
}

void DeformEffect::Start()
{
#ifndef STRETCH_RECT
	CHECK_OK;
	OpenGLD3D::g_pd3dDevice->GetRenderTarget(0,&m_oldRenderTarget);
	OpenGLD3D::g_pd3dDevice->SetRenderTarget(0,m_screenTexture->GetRenderTarget());
#endif
}

void DeformEffect::RenderSpritesBegin(Shader* shader, int deformTextureGl, Texture* deformTextureDx)
{
	glEnable(GL_TEXTURE_2D);
	shader->Bind();
	shader->SetUniform("aspect",g_app->m_renderer->ScreenW()/(float)g_app->m_renderer->ScreenH());
	int deformSampler = deformTextureDx ? shader->SetSampler("deformSampler",deformTextureDx) : shader->SetSampler("deformSampler",deformTextureGl);
	if(deformSampler>=0)
	{
		OpenGLD3D::g_pd3dDevice->SetSamplerState(deformSampler,D3DSAMP_MAGFILTER,D3DTEXF_LINEAR);
		OpenGLD3D::g_pd3dDevice->SetSamplerState(deformSampler,D3DSAMP_MINFILTER,D3DTEXF_LINEAR);
		OpenGLD3D::g_pd3dDevice->SetSamplerState(deformSampler,D3DSAMP_MIPFILTER,D3DTEXF_NONE);
		OpenGLD3D::g_pd3dDevice->SetSamplerState(deformSampler,D3DSAMP_ADDRESSU,D3DTADDRESS_WRAP); // time
		OpenGLD3D::g_pd3dDevice->SetSamplerState(deformSampler,D3DSAMP_ADDRESSV,D3DTADDRESS_CLAMP); // distance
	}
	int screenSampler = shader->SetSampler("screenSampler",m_screenTexture);
	if(screenSampler>=0)
	{
		OpenGLD3D::g_pd3dDevice->SetSamplerState(screenSampler,D3DSAMP_MAGFILTER,D3DTEXF_LINEAR);
		OpenGLD3D::g_pd3dDevice->SetSamplerState(screenSampler,D3DSAMP_MINFILTER,D3DTEXF_LINEAR);
		OpenGLD3D::g_pd3dDevice->SetSamplerState(screenSampler,D3DSAMP_MIPFILTER,D3DTEXF_NONE);
		OpenGLD3D::g_pd3dDevice->SetSamplerState(screenSampler,D3DSAMP_ADDRESSU,D3DTADDRESS_CLAMP);
		OpenGLD3D::g_pd3dDevice->SetSamplerState(screenSampler,D3DSAMP_ADDRESSV,D3DTADDRESS_CLAMP);
		// seems to be partially ignored on my card,
		//  verify that it's my card's bug or fix it
	}
}

// differs from Camera::Get2DScreenPos by viewport, this one returns coords in range 0..1
static float Get2DScreenPos(Vector3 const &_vector, float *_screenX, float *_screenY)
{
	double outX, outY, outZ;

	int viewport[4] = {0,0,1,1};
	double viewMatrix[16];
	double projMatrix[16];
	glGetDoublev(GL_MODELVIEW_MATRIX, viewMatrix);
	glGetDoublev(GL_PROJECTION_MATRIX, projMatrix);
	gluProject(_vector.x, _vector.y, _vector.z, 
		viewMatrix,
		projMatrix,
		viewport,
		&outX,
		&outY,
		&outZ);

	*_screenX = outX;
	*_screenY = outY;
	return outZ;
}

// depth != 0 -> render sprite with given width/depth
// depth == 0 -> render fullscreen
void DeformEffect::RenderSprite(Shader* shader, Vector3 posWorld, float width, float depth, float redux)
{
	Vector2 posScreen = Vector2(0,0);
	Vector2 posScreen2 = Vector2(0,0);
	Vector3 lookDir = (posWorld-g_app->m_camera->GetPos()).Normalise();
	float size = 0.001f;
	float aspect = g_app->m_renderer->ScreenW()/(float)g_app->m_renderer->ScreenH();
	if(Get2DScreenPos(posWorld,&posScreen.x,&posScreen.y)<1)
	{
		Vector3 posWorld2 = posWorld + Vector3(lookDir.z,0,-lookDir.x).Normalise();
		float z = Get2DScreenPos(posWorld2,&posScreen2.x,&posScreen2.y);
		if(!(posScreen.x<-1 || posScreen.x>2 || posScreen.y<-1 || posScreen.y>2))
		{
			Vector2 tmp = posScreen-posScreen2;
			size = 10*Vector2(tmp.x*aspect,tmp.y).Mag();
			if(size>0.4f) size = 0.4f;
		}
	}
	shader->SetUniform("center",posScreen.x,1-posScreen.y);
	shader->SetUniform("invSize",1/(size*width));
	shader->SetUniform("height",(size*width)*depth*4);

	posScreen = posScreen*2-Vector2(1,1);
	Vector2 size2 = Vector2(size*width,size*width*aspect)*redux;
	if(!depth)
	{
		posScreen = Vector2(0,0);
		size2 = Vector2(1,1);
	}
	glBegin(GL_QUADS);
		glVertex2f(posScreen.x-size2.x, posScreen.y-size2.y);
		glVertex2f(posScreen.x-size2.x, posScreen.y+size2.y);
		glVertex2f(posScreen.x+size2.x, posScreen.y+size2.y);
		glVertex2f(posScreen.x+size2.x, posScreen.y-size2.y);
	glEnd();
}

void DeformEffect::RenderSpritesEnd(Shader* shader)
{
	shader->SetSampler("deformSampler",NULL);
	shader->SetSampler("screenSampler",NULL);
	shader->Unbind();
	glDisable(GL_TEXTURE_2D);
}

struct SoulDistance
{
	Spirit* m_spirit;
	float   m_distance;
};

int CompareSoulDistance(const void * elem1, const void * elem2 )
{
	SoulDistance *s1 = (SoulDistance*) elem1;
	SoulDistance *s2 = (SoulDistance*) elem2;

	if      ( s1->m_distance < s2->m_distance )     return +1;
	else if ( s1->m_distance > s2->m_distance )     return -1;
	else                                            return 0;
}

void DeformEffect::Stop()
{
	// possible optimization: immediately return when deformations are not visible

	START_PROFILE(g_app->m_profiler, "Deform Pre-render");

	// conserved for future testing several deformation types
	//extern signed char g_keyDeltas[256];
	//if(g_keyDeltas[' ']==1)
	//{
	//	extern int soulType;
	//	++soulType %= SOUL_TYPES;
	//	DebugOut("Soul effect=%d.\n",soulType);
	//}

	CHECK_OK;
	glDepthMask(0);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

#ifdef STRETCH_RECT
	// copy screen
	OpenGLD3D::g_pd3dDevice->GetRenderTarget(0,&m_oldRenderTarget);
	RECT rect = {0,0,g_app->m_renderer->ScreenW(),g_app->m_renderer->ScreenH()};
	OpenGLD3D::g_pd3dDevice->StretchRect(m_oldRenderTarget,&rect,m_screenTexture->GetRenderTarget(),&rect,D3DTEXF_NONE);
#endif

	// build deforem table
	OpenGLD3D::g_pd3dDevice->SetRenderTarget(0,m_deformTexture->GetRenderTarget());
	glViewport( 0, 0, m_deformTexture->GetParams().m_w, m_deformTexture->GetParams().m_h );
	glClear(GL_COLOR_BUFFER_BIT);

	// - spirits
	//   sort them
	SoulDistance* sorted = new SoulDistance[g_app->m_location->m_spirits.Size()];
	unsigned numSouls = 0;
	for(unsigned i=0;i<g_app->m_location->m_spirits.Size();i++)
	{
		if( g_app->m_location->m_spirits.ValidIndex(i) )
		{
			Spirit *r = g_app->m_location->m_spirits.GetPointer(i);
			if(r->m_state!=Spirit::StateAttached)
			{
				sorted[numSouls].m_spirit = r;
				sorted[numSouls].m_distance = (r->m_pos-g_app->m_camera->GetPos()).MagSquared();
				numSouls++;
			}
		}
	}
	qsort(sorted,numSouls,sizeof(SoulDistance),CompareSoulDistance);
	//   render them sorted
	RenderSpritesBegin(m_deformDiamondShader,g_app->m_resource->GetTexture("textures/deform1c.bmp", false, false),NULL);
	m_deformDiamondShader->SetUniform("time",(GetTickCount()%10000)/1000.0f);
	for(unsigned i=0;i<numSouls;i++)
	{
		RenderSprite(m_deformDiamondShader,sorted[i].m_spirit->m_pos,0.32f,0.12f,1.3f);
	}
	RenderSpritesEnd(m_deformDiamondShader);
	delete[] sorted;

	// - explosions
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE,GL_ONE);
	//char buf[100];sprintf(buf,"textures/deform%d.bmp",punchtest);
	RenderSpritesBegin(m_deformSphereShader,g_app->m_resource->GetTexture(punchParams.punchTable, false, false),NULL);
	DWORD now = GetTickCount();
	for(unsigned i=m_punchList.Size();i--;)
	{
		if( m_punchList.ValidIndex(i) )
		{
			const Punch* p = m_punchList.GetPointer(i);
			float age = (now-p->m_birth)/1000.0f;
			const float maxage = punchParams.seconds;
			if(age>=maxage)
			{
				m_punchList.RemoveData(i);
			}
			else
			{
				float width;
				float depth;
				switch(punchParams.fadeType)
				{
					case PF_OLD:
						width = 10*(maxage-(maxage-age)*(maxage-age)/maxage);
						depth = 0.15f*(1-age/maxage);
						break;
					case PF_NEW:
						width = 10;
						// depth fadeout curve is hardcoded here rather than painted in .bmp,
						// to avoid banding from low precision (8bit)
						depth = 0.03*(1-age/maxage)*(1-age/maxage);
						break;
				}
				float time = age/maxage;
				if(time<0.03f) time=0.03f; // clamp on CPU
				m_deformSphereShader->SetUniform("time",sqrtf(time));
				RenderSprite(m_deformSphereShader,p->m_pos,width,depth,0.8f);
			}
		}
	}
	RenderSpritesEnd(m_deformSphereShader);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_BLEND);

	END_PROFILE(g_app->m_profiler, "Deform Pre-render");
	START_PROFILE(g_app->m_profiler, "Deform Apply");

	// deform screen according to deform table
	OpenGLD3D::g_pd3dDevice->SetRenderTarget(0,m_oldRenderTarget);
	m_oldRenderTarget->Release();
	glViewport( 0, 0, g_app->m_renderer->ScreenW(), g_app->m_renderer->ScreenH() );
	RenderSpritesBegin(m_deformFullscreenShader,0,m_deformTexture);
	RenderSprite(m_deformFullscreenShader,Vector3(0,0,0),1,0,1);
	RenderSpritesEnd(m_deformFullscreenShader);

	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(1);

	END_PROFILE(g_app->m_profiler, "Deform Apply");
}

DeformEffect::~DeformEffect()
{
	delete m_deformFullscreenShader;
	delete m_deformDiamondShader;
	delete m_deformSphereShader;
	delete m_deformTexture;
	delete m_screenTexture;
}

DeformEffect* g_deformEffect = NULL;

#endif
