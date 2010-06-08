#ifndef _included_deform_h
#define _included_deform_h

#ifdef USE_DIRECT3D

#include "lib/llist.h"
#include "lib/shader.h"
#include "lib/texture.h"
#include "lib/vector3.h"

class DeformEffect
{
public:
	static DeformEffect* Create();
	void AddPunch(Vector3 pos, float range); ///< Creates punch effect, adds it to internal list of active deformations.
	void Start(); ///< Step 1: call before rendering scene with deform effects.
	void Stop(); ///< Step 2: call after rendering scene with deform effects.
	~DeformEffect();
private:
	DeformEffect();
	void RenderSpritesBegin(Shader* shader, int deformTextureGl, Texture* deformTextureDx);
	void RenderSprite(Shader* shader, Vector3 posWorld, float width, float depth, float redux);
	void RenderSpritesEnd(Shader* shader);
	Texture* m_screenTexture;
	Texture* m_deformTexture;
	Shader* m_deformDiamondShader;
	Shader* m_deformSphereShader;
	Shader* m_deformFullscreenShader;
	IDirect3DSurface9* m_oldRenderTarget;
	struct Punch
	{
		Punch() {};
		Vector3 m_pos;
		float m_range;
		DWORD m_birth;
	};
	LList<Punch> m_punchList;
};

extern DeformEffect* g_deformEffect;

#endif

#endif

