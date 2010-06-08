#ifndef _included_deform_h
#define _included_deform_h

#ifdef USE_DIRECT3D

#include "lib/tosser/llist.h"
#include "lib/shader.h"
#include "lib/texture.h"
#include "lib/vector3.h"

class DeformEffect
{
public:
	static DeformEffect* Create();
	void AddPunch(Vector3 pos, float range); ///< Creates punch effect, adds it to internal list of active deformations.
	void AddTearing(Vector3 pos, float range); ///< Creates tearing effect, adds it to internal list of active deformations.
	void AddTearingPath(Vector3 pos1, Vector3 pos2, float strength); ///< Creates tearing effect between pos1 and pos2.
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
	struct Deformation
	{
		Deformation() {};
		Vector3 m_pos;
		float m_range;
		float m_extraDepth;
		DWORD m_birth;
		enum DeformType type;
	};
	LList<Deformation> m_deformationList;
};

extern DeformEffect* g_deformEffect;

#endif

#endif

