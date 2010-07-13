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
	static Shader* Create(const char* vertexFileName, const char* pixelFileName);
	void Bind();
	void Unbind();
	void SetBoolArray(const char* name, const BOOL* arr, unsigned numBools);
	void SetBool(const char* name, BOOL b);
	void SetIntArray(const char* name, const int* arr, unsigned numInts);
	void SetInt(const char* name, int i);
	void SetFloat(const char* name, float f);
	void SetFloatArray(const char* name, const float* arr, unsigned numFloats);
	void SetUniform(const char* name, float v1);
	void SetUniform(const char* name, float v1, float v2);
	void SetUniform(const char* name, float v1, float v2, float v3);
	void SetUniform(const char* name, float v1, float v2, float v3, float v4);
	void SetMatrix(const char* name, const float* v);
	void SetMatrix(const char* name, const double* v);
	void SetMatrixTranspose(const char* name, const float* v);
	void SetMatrixTranspose(const char* name, const double* v);
	int SetSamplerDX(const char* name, IDirect3DBaseTexture9* tex);
	int SetSamplerTex(const char* name, const Texture* tex);
	int SetSamplerGLTextureId(const char* name, int tex);
	int SetSamplerGLFixedStage(const char* name, int tex);
	~Shader();
private:
	Shader(const char* vertexFileName, const char* pixelFileName);
	LPD3DXBUFFER            m_shaderFunc[2];
	LPD3DXCONSTANTTABLE     m_constantTable[2];
	IDirect3DVertexShader9* m_vertexShader;
	IDirect3DPixelShader9*  m_pixelShader;
	DWORD                   m_fvf;
	bool                    m_Ok;
	char					m_vertexFilename[128], m_pixelFilename[128];
};

#endif

#endif
