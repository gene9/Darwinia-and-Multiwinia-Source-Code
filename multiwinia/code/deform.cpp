#include "lib/universal_include.h"

#ifdef USE_DIRECT3D

#include "app.h"
#include "camera.h"
#include "deform.h"
#include "explosion.h"
#include "location.h"
#include "renderer.h"
#include "lib/FixedPipeline.h"
#include "lib/ogl_extensions.h"
#include "lib/opengl_directx_internals.h"
#include "lib/profiler.h"
#include "lib/resource.h"
#include "lib/vector3.h"

#define STRETCH_RECT // should make FSAA possible (for small penalty with non-FSAA setups)

#define CHECK_OK if(!m_deformFullscreenShader) return

namespace OpenGLD3D {
	extern D3DPRESENT_PARAMETERS g_d3dpp;
};

enum DeformType
{
	DT_PUNCH = 0,
	DT_TEAR,
	DT_LAST
};

struct DeformParams
{
	const char* deformTable;
	float seconds;
};

DeformParams deformParams[DT_LAST] = // indexed by DeformType
{
	// new fast punch
	{"textures/deform36c.bmp",1},
	// tearing
	{"textures/deform3.bmp",1},
};

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
	AppDebugAssert(g_app);
	AppDebugAssert(g_app->m_renderer);
	m_deformFullscreenShader = Shader::Create("shaders/deform.vs","shaders/deformFullscreen.ps");
	m_deformSphereShader = Shader::Create("shaders/deform.vs","shaders/deformSphere.ps");
	m_deformDiamondShader = Shader::Create("shaders/deform.vs","shaders/deformDiamond.ps");
	m_screenTexture = Texture::Create(TextureParams(g_app->m_renderer->ScreenW(),g_app->m_renderer->ScreenH(),
		OpenGLD3D::g_d3dpp.BackBufferFormat,TF_RENDERTARGET));
	m_deformTexture = Texture::Create(TextureParams(g_app->m_renderer->ScreenW()/2,g_app->m_renderer->ScreenH()/2,D3DFMT_A8R8G8B8,TF_RENDERTARGET));
}

void DeformEffect::AddPunch(Vector3 pos, float range)
{
	CHECK_OK;
	Deformation deform;
	deform.m_pos = pos;
	deform.m_range = 1;//range;
	deform.m_extraDepth = 1;
	deform.m_birth = GetTickCount();
	deform.type = DT_PUNCH;
	m_deformationList.PutData(deform);
}

void DeformEffect::AddTearing(Vector3 pos, float range)
{
	CHECK_OK;
	Deformation deform;
	deform.m_pos = pos;
	deform.m_range = range;
#if 0
	// ripples behind centipede
	deform.m_extraDepth = 1;
	deform.type = DT_PUNCH;
#else
	// tearing
	deform.m_extraDepth = 3;
	deform.type = DT_TEAR;
#endif
	deform.m_birth = GetTickCount();
	m_deformationList.PutData(deform);
}

void DeformEffect::AddTearingPath(Vector3 pos1, Vector3 pos2, float strength)
{
	float dist = (pos2-pos1).Mag();
	unsigned points = (int)(dist); // more points -> smoother tearing

	// nearly smooth (smaller points)
	strength *= 0.8;

	// less smooth (fewer points)
	//points /= 2;

	// series of points (fewer bigger points)
	//points /= 4; strength *= 1.3;

	for(unsigned i=0;i<points;i++)
		g_deformEffect->AddTearing(pos2+(pos1-pos2)*(i/(float)points),strength);
}

void DeformEffect::Start()
{
#ifndef STRETCH_RECT
	CHECK_OK;
	OpenGLD3D::g_pd3dDevice->GetRenderTarget(0,&m_oldRenderTarget);
	OpenGLD3D::g_pd3dDevice->SetRenderTarget(0,m_screenTexture->GetRenderTarget());
	glViewport( 0, 0, m_screenTexture->GetParams().m_w, m_screenTexture->GetParams().m_h );
#endif
}

void DeformEffect::RenderSpritesBegin(Shader* shader, int deformTextureGl, Texture* deformTextureDx)
{
	glEnable(GL_TEXTURE_2D);
	g_fixedPipeline->Disable();

	shader->Bind();
	shader->SetUniform("aspect",g_app->m_renderer->ScreenW()/(float)g_app->m_renderer->ScreenH());
	int deformSampler = deformTextureDx ? shader->SetSamplerTex("deformSampler",deformTextureDx) : shader->SetSamplerGLTextureId("deformSampler",deformTextureGl);
	if(deformSampler>=0)
	{
		OpenGLD3D::g_pd3dDevice->SetSamplerState(deformSampler,D3DSAMP_MAGFILTER,D3DTEXF_LINEAR);
		OpenGLD3D::g_pd3dDevice->SetSamplerState(deformSampler,D3DSAMP_MINFILTER,D3DTEXF_LINEAR);
		OpenGLD3D::g_pd3dDevice->SetSamplerState(deformSampler,D3DSAMP_MIPFILTER,D3DTEXF_NONE);
		OpenGLD3D::g_pd3dDevice->SetSamplerState(deformSampler,D3DSAMP_ADDRESSU,D3DTADDRESS_WRAP); // time
		OpenGLD3D::g_pd3dDevice->SetSamplerState(deformSampler,D3DSAMP_ADDRESSV,D3DTADDRESS_CLAMP); // distance
	}
	int screenSampler = shader->SetSamplerTex("screenSampler",m_screenTexture);
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

	glBegin(GL_QUADS);
}

// differs from Camera::Get2DScreenPos by viewport, this one returns coords in range 0..1
static float Get2DScreenPos(Vector3 const &_vector, double *_screenX, double *_screenY)
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

// depth != 0 -> render sprite with given depth
// depth = 0 -> render fullscreen
void DeformEffect::RenderSprite(Shader* shader, Vector3 posWorld, float width, float depth, float redux)
{
	Vector2 posScreen = Vector2(0,0);
	Vector2 posScreen2 = Vector2(0,0);
	Vector3 lookDir = (posWorld-g_app->m_camera->GetPos()).Normalise();
	float size = 0.001;
	float aspect = g_app->m_renderer->ScreenW()/(float)g_app->m_renderer->ScreenH();
	if(Get2DScreenPos(posWorld,&posScreen.x,&posScreen.y)<1)
	{
		Vector3 posWorld2 = posWorld + Vector3(lookDir.z,0,-lookDir.x).Normalise();
		float z = Get2DScreenPos(posWorld2,&posScreen2.x,&posScreen2.y);
		if(!(posScreen.x<-1 || posScreen.x>2 || posScreen.y<-1 || posScreen.y>2))
		{
			Vector2 tmp = posScreen-posScreen2;
			size = 10*Vector2(tmp.x*aspect,tmp.y).Mag();
			if(size>0.4) size = 0.4;
		}
	}

	//shader->SetUniform("center",posScreen.x,1-posScreen.y);
	//shader->SetUniform("invSize",1/(size*width));
	//shader->SetUniform("height",(size*width)*depth*4);
	gglMultiTexCoord2fARB(GL_TEXTURE0_ARB,posScreen.x,1-posScreen.y);
	gglMultiTexCoord2fARB(GL_TEXTURE1_ARB,1/(size*width),(size*width)*depth*4);

	posScreen = posScreen*2-Vector2(1,1);
	Vector2 size2 = Vector2(size*width,size*width*aspect)*redux;
	if(!depth)
	{
		posScreen = Vector2(0,0);
		size2 = Vector2(1,1);
	}

	glVertex2f(posScreen.x-size2.x, posScreen.y-size2.y);
	glVertex2f(posScreen.x-size2.x, posScreen.y+size2.y);
	glVertex2f(posScreen.x+size2.x, posScreen.y+size2.y);
	glVertex2f(posScreen.x+size2.x, posScreen.y-size2.y);
}

void DeformEffect::RenderSpritesEnd(Shader* shader)
{
	glEnd();

	shader->SetSamplerTex("deformSampler",NULL);
	shader->SetSamplerTex("screenSampler",NULL);
	shader->Unbind();
	g_fixedPipeline->Enable();
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

	START_PROFILE( "Deform Pre-render");

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
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE,GL_ONE);

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
	glColor3f((GetTickCount()%1000)/1000.0,0,0);
	RenderSpritesBegin(m_deformDiamondShader,g_app->m_resource->GetTexture("textures/deform1c.bmp", false, false),NULL);
	for(unsigned i=0;i<numSouls;i++)
	{
		RenderSprite(m_deformDiamondShader,sorted[i].m_spirit->m_pos,0.32,0.12,1.3);
	}
	RenderSpritesEnd(m_deformDiamondShader);
	delete[] sorted;

	// - deformations
	DWORD now = GetTickCount();
	for(unsigned type=0;type<DT_LAST;type++)
	{
		RenderSpritesBegin(m_deformSphereShader,g_app->m_resource->GetTexture(deformParams[type].deformTable, false, false),NULL);
		//m_deformSphereShader->SetUniform("time",0);
		/*for(unsigned i=0;i<g_explosionManager.GetExplosionList().Size();i++)
		{
		if( g_explosionManager.GetExplosionList().ValidIndex(i) )
		{
		const Explosion* e = g_explosionManager.GetExplosionList().GetData(i);
		RenderSprite(m_deformSpriteShader,e->GetCenter());
		}
		}*/
		for(unsigned i=m_deformationList.Size();i--;)
		{
			if( m_deformationList.ValidIndex(i) )
			{
				const Deformation* p = m_deformationList.GetPointer(i);
				// process only punch in first pass, tearing in second pass
				if(p->type==type)
				{
					float age = (now-p->m_birth)/1000.0;
					const float maxage = deformParams[type].seconds;
					if(age>=maxage)
					{
						m_deformationList.RemoveData(i);
					}
					else
					{
						float width;
						float depth;
						float time = age/maxage;
						if(time<0.03) time=0.03; // clamp on CPU
						switch(type)
						{
							case DT_TEAR:
								width = 10*(maxage-(maxage-age)*(maxage-age)/maxage);
								depth = 0.15*(1-age/maxage);
								glColor3f(time,0,0);
								RenderSprite(m_deformSphereShader,p->m_pos,p->m_range*width,p->m_range*p->m_extraDepth*depth,0.6);
								break;
							case DT_PUNCH:
								width = 10;
								// depth fadeout curve is hardcoded here rather than painted in .bmp,
								// to avoid banding from low precision (8bit)
								depth = 0.03*(1-age/maxage)*(1-age/maxage);
								glColor3f(iv_sqrt(time),0,0);
								//AppDebugOut("%f\n",iv_sqrt(time));
								RenderSprite(m_deformSphereShader,p->m_pos,p->m_range*width,p->m_range*p->m_extraDepth*depth,0.8);
								break;
						}
					}
				}
			}
		}
		RenderSpritesEnd(m_deformSphereShader);
	}
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_BLEND);

	OpenGLD3D::g_pd3dDevice->Resolve( D3DRESOLVE_RENDERTARGET0, NULL, m_deformTexture->GetTexture(), NULL, 0, 0, NULL, 1.0, 0, NULL );

	END_PROFILE( "Deform Pre-render");
	START_PROFILE( "Deform Apply");

	// deform screen according to deform table
	glViewport( 0, 0, g_app->m_renderer->ScreenW(), g_app->m_renderer->ScreenH() );
	glClear(GL_COLOR_BUFFER_BIT);
	RenderSpritesBegin(m_deformFullscreenShader,0,m_deformTexture);
	RenderSprite(m_deformFullscreenShader,Vector3(0,0,0),1,0,1);
	RenderSpritesEnd(m_deformFullscreenShader);

	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(1);

	END_PROFILE( "Deform Apply");
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
