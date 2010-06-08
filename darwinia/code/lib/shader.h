#ifndef INCLUDED_SHADER_H
#define INCLUDED_SHADER_H

#ifdef USE_DIRECT3D

#include <d3dx9.h>
#include "texture.h"

//
// HLSL shader with support for
//  - setting only pixel part, staying with fixed vertex (or vice versa)
//

class Shader
{
public:
	//! \param vertexFileName
	//!  Path to HLSL vertex shader for vs_2_0 model.
	//! \param pixelFileName
	//!  Path to HLSL pixel shader for ps_2_0 or ps_3_0 model.
	//! \param shaderVersion
	//!  2 = ps_2_0, 3 = ps_3_0
	static Shader* Create(const char* vertexFileName, const char* pixelFileName);
	void Bind();
	void Unbind();
	void SetFloatArray(char* name, float* arr, unsigned numFloats);
	void SetUniform(char* name, float v1);
	void SetUniform(char* name, float v1, float v2);
	void SetUniform(char* name, float v1, float v2, float v3);
	void SetUniform(char* name, float v1, float v2, float v3, float v4);
	void SetMatrix(char* name, const double* v);
	int SetSampler(char* name, IDirect3DBaseTexture9* tex);
	int SetSampler(char* name, Texture* tex);
	int SetSampler(char* name, int tex);
	~Shader();
private:
	Shader(const char* vertexFileName, const char* pixelFileName);
	LPD3DXBUFFER            m_shaderFunc[2];
	LPD3DXCONSTANTTABLE     m_constantTable[2];
	IDirect3DVertexShader9* m_vertexShader;
	IDirect3DPixelShader9*  m_pixelShader;
	DWORD                   m_fvf;
	bool                    m_Ok;
};

#endif

#endif
