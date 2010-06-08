#ifndef INCLUDED_FIXEDPIPELINE_H
#define INCLUDED_FIXEDPIPELINE_H

#ifdef USE_DIRECT3D

class FixedPipeline
{
public:
	FixedPipeline();
	~FixedPipeline();
	
	//! Binds correct shader, called automatically by glEnd().
	void Bind();
	//! Unbinds our shader.
	void Unbind();
	//! Disables all our activities, use before binding non-fixed shader.
	void Disable();
	//! Reenables our activities when non-fixed shader is no longer needed.
	void Enable();


	// fog state
	bool m_fogEnabled;
	float m_fogStart;
	float m_fogEnd;
	// light state
	bool m_lighting;
	bool m_lightEnabled[2];
	// material state
	float m_materialDiffuse[4];
	float m_materialSpecular[4];
	float m_materialShininess;

	struct State
	{
		D3DLIGHT9 m_light[2];
		bool m_specular;
		const class Texture* m_environmentDiffuse;
		const class Texture* m_environmentSpecular;
		// derived state
		int m_numLights;
		float m_fogRange[2]; // fogEnd, fogEnd-fogStart
	};
	State m_state; // setup for future shader
	State m_oldState; // what is in GPU now

protected:
	class Shader* m_fixedShader[3];
	class Shader* m_bound;
	bool m_enabled;
};

extern class FixedPipeline* g_fixedPipeline;


#endif // USE_DIRECT3D

#endif
