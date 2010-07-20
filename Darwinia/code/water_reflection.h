#ifndef INCLUDED_WATER_REFLECTION_H
#define INCLUDED_WATER_REFLECTION_H

#ifdef USE_DIRECT3D

class WaterReflectionEffect
{
public:
	static WaterReflectionEffect* Create();
	void PreRenderWaterReflection();
	bool IsPrerendering() {return m_prerendering;}
	void Start();
	void Stop();
	~WaterReflectionEffect();
private:
	WaterReflectionEffect();
	class Texture   *m_waterReflectionTexture;
	class Shader    *m_waterMeshWavesShader;
	bool             m_prerendering;
};

extern WaterReflectionEffect* g_waterReflectionEffect;

#endif

#endif
