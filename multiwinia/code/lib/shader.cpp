#include "lib/universal_include.h"

#ifdef USE_DIRECT3D

#include "app.h"
#include "lib/filesys/binary_stream_readers.h"
#include "resource.h"
#include "lib/debug_utils.h"
#include "lib/opengl_directx_internals.h"
#include "lib/shader.h"

#pragma comment(lib,"d3dx9.lib")

#define CHECK_HR_FILE(hr,name) {if(FAILED(hr)) {AppDebugOut("Shader load failed: %s",name);AppDebugAssert(SUCCEEDED(hr));return;}}
#define CHECK_HR(hr) {AppDebugAssert(SUCCEEDED(hr)); if(FAILED(hr)) return;}

// general purpose whole file -> memory loader
// could be moved to public header, if anyone else wants to use it
struct MemFile
{
	MemFile(const char* filename)
	{
		m_data = NULL;
		m_size = 0;
		BinaryReader* reader = g_app->m_resource->GetBinaryReader(filename);
		if(!reader)
		{
			AppDebugOut("MemFile load failed: %s",filename); AppDebugAssert(0);
			return;
		}
		m_size = reader->GetSize();
		m_data = new unsigned char[m_size];
		m_size = reader->ReadBytes( m_size, m_data );
		delete reader;
	}
	~MemFile()
	{
		delete[] m_data;
	}
	unsigned char* m_data;
	unsigned m_size;
};

// support for #include in hlsl
class MyD3DXInclude : public ID3DXInclude
{
public:
	MyD3DXInclude()
	{
		mf = NULL;
	}
	~MyD3DXInclude()
	{
		delete mf;
	}
	STDMETHOD(Open)(D3DXINCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID *ppData, UINT *pBytes )
	{
		delete mf;
		char buf[500];
		sprintf(buf,"shaders/%s",pFileName);
		mf = new MemFile(buf);
		if(mf)
		{
			*ppData = mf->m_data;
			*pBytes = mf->m_size;
			return S_OK;
		}
		return S_FALSE;
	}
	STDMETHOD(Close)(LPCVOID pData)
	{
		delete mf;
		mf = NULL;
		return S_OK;
	}
protected:
	MemFile* mf;
};

Shader* Shader::Create(const char* vertexFileName, const char* pixelFileName)
{
	Shader* s = new Shader(vertexFileName,pixelFileName);
	if(!s->m_Ok) 
	{
		SAFE_DELETE(s);
        AppReleaseAssert(0,"Shader create error:\nvertex name:%s\npixel name:%s", vertexFileName, pixelFileName);
	}
	return s;
}

Shader::Shader(const char* vertexFileName, const char* pixelFileName)
{
	m_constantTable[0] = NULL;
	m_constantTable[1] = NULL;
	m_vertexShader = NULL;
	m_pixelShader = NULL;
	m_Ok = false;

	strcpy( m_vertexFilename, vertexFileName );
	strcpy( m_pixelFilename, pixelFileName );

	if(vertexFileName)
	{
		LPD3DXBUFFER error;
		HRESULT hr;
		MemFile mf(vertexFileName);
		MyD3DXInclude incl;

		const char *vertexShaderProfile = D3DXGetVertexShaderProfile(OpenGLD3D::g_pd3dDevice);
		hr = D3DXCompileShader((LPCSTR)mf.m_data,mf.m_size,NULL,&incl,"vs_main",vertexShaderProfile,0,&m_shaderFunc[0],&error,&m_constantTable[0]);

		CHECK_HR_FILE(hr,vertexFileName);
		if(FAILED(hr)) return;
		hr = OpenGLD3D::g_pd3dDevice->CreateVertexShader((DWORD*)m_shaderFunc[0]->GetBufferPointer(),&m_vertexShader);
		if(FAILED(hr)) return; // it's legal to fail here when gpu can't handle this shader
	}
	if(pixelFileName)
	{
		LPD3DXBUFFER error;
		HRESULT hr;
		MemFile mf(pixelFileName);
		MyD3DXInclude incl;
		hr = D3DXCompileShader((LPCSTR)mf.m_data,mf.m_size,NULL,&incl,"ps_main",D3DXGetPixelShaderProfile(OpenGLD3D::g_pd3dDevice),0,&m_shaderFunc[1],&error,&m_constantTable[1]);
		CHECK_HR_FILE(hr,pixelFileName);
		if(FAILED(hr)) return;
		hr = OpenGLD3D::g_pd3dDevice->CreatePixelShader((DWORD*)m_shaderFunc[1]->GetBufferPointer(),&m_pixelShader);
		if(FAILED(hr)) return; // it's legal to fail here when gpu can't handle this shader
	}
	m_Ok = m_vertexShader && m_pixelShader; //true;
}

void Shader::Bind()
{
	// AppDebugOut("Shader: binding vertex shader %s\n", m_vertexFilename );
	HRESULT hr;
	hr = OpenGLD3D::g_pd3dDevice->SetVertexShader(m_vertexShader);
	CHECK_HR(hr);
	hr = OpenGLD3D::g_pd3dDevice->SetPixelShader(m_pixelShader);
	CHECK_HR(hr);
}

void Shader::Unbind()
{
	HRESULT hr;
	hr = OpenGLD3D::g_pd3dDevice->SetPixelShader(NULL);
	CHECK_HR(hr);
	hr = OpenGLD3D::g_pd3dDevice->SetVertexShader(NULL);
	CHECK_HR(hr);
}

void Shader::SetBoolArray(const char* name, const BOOL* arr, unsigned numBools)
{
	for(unsigned i=0;i<2;i++)
	{
		if(m_constantTable[i])
		{
			D3DXHANDLE handle = m_constantTable[i]->GetConstantByName(NULL,name);
			if(handle)
			{
				HRESULT hr = m_constantTable[i]->SetBoolArray(OpenGLD3D::g_pd3dDevice,handle,arr,numBools);
			}
		}
	}
}

void Shader::SetBool(const char* name, BOOL b)
{
	SetBoolArray(name,&b,1);
}

void Shader::SetIntArray(const char* name, const int* arr, unsigned numInts)
{
	for(unsigned i=0;i<2;i++)
	{
		if(m_constantTable[i])
		{
			D3DXHANDLE handle = m_constantTable[i]->GetConstantByName(NULL,name);
			if(handle)
			{
				HRESULT hr = m_constantTable[i]->SetIntArray(OpenGLD3D::g_pd3dDevice,handle,arr,numInts);
			}
		}
	}
}

void Shader::SetInt(const char* name, int i)
{
	SetIntArray(name,&i,1);
}

void Shader::SetFloat(const char* name, float f)
{
	SetFloatArray(name,&f,1);
}

void Shader::SetFloatArray(const char* name, const float* arr, unsigned numFloats)
{
	for(unsigned i=0;i<2;i++)
	{
		if(m_constantTable[i])
		{
			D3DXHANDLE handle = m_constantTable[i]->GetConstantByName(NULL,name);
			if(handle)
			{
				HRESULT hr = m_constantTable[i]->SetFloatArray(OpenGLD3D::g_pd3dDevice,handle,arr,numFloats);
			}
		}
	}
}

void Shader::SetUniform(const char* name, float v1)
{
	SetFloatArray(name,&v1,1);
}

void Shader::SetUniform(const char* name, float v1, float v2)
{
	float arr[2];
	arr[0] = v1;
	arr[1] = v2;
	SetFloatArray(name,arr,2);
}

void Shader::SetUniform(const char* name, float v1, float v2, float v3)
{
	float arr[3];
	arr[0] = v1;
	arr[1] = v2;
	arr[2] = v3;
	SetFloatArray(name,arr,3);
}

void Shader::SetUniform(const char* name, float v1, float v2, float v3, float v4)
{
	float arr[4];
	arr[0] = v1;
	arr[1] = v2;
	arr[2] = v3;
	arr[3] = v4;
	SetFloatArray(name,arr,4);
}

void Shader::SetMatrix(const char* name, const float* mat)
{
	for(unsigned i=0;i<2;i++)
	{
		if(m_constantTable[i])
		{
			D3DXHANDLE handle = m_constantTable[i]->GetConstantByName(NULL,name);
			if(handle)
			{
				HRESULT hr = m_constantTable[i]->SetMatrix(OpenGLD3D::g_pd3dDevice,handle,(D3DXMATRIX*)mat);
				CHECK_HR(hr);
			}
		}
	}
}

void Shader::SetMatrixTranspose(const char* name, const float* v)
{
	D3DXMATRIX mat;
	for(unsigned i=0;i<16;i++)
		mat[i] = (float)v[(i%4)*4+i/4];
	SetMatrix(name,&mat._11);
}

void Shader::SetMatrix(const char* name, const double* v)
{
	D3DXMATRIX mat;
	for(unsigned i=0;i<16;i++)
		mat[i] = (float)v[i];
	SetMatrix(name,&mat._11);
}

void Shader::SetMatrixTranspose(const char* name, const double* v)
{
	D3DXMATRIX mat;
	for(unsigned i=0;i<16;i++)
		mat[i] = (float)v[(i%4)*4+i/4];
	SetMatrix(name,&mat._11);
}

int Shader::SetSamplerDX(const char* name, IDirect3DBaseTexture9* tex)
{
	int index = -1;
	for(unsigned i=0;i<2;i++)
	{
		if(m_constantTable[i])
		{
			D3DXHANDLE handle = m_constantTable[i]->GetConstantByName(NULL,name);
			if(handle)
			{
				index = m_constantTable[i]->GetSamplerIndex(handle);
				HRESULT hr = OpenGLD3D::g_pd3dDevice->SetTexture(index,tex);
//				hr = OpenGLD3D::g_pd3dDevice->SetSamplerState(index,D3DSAMP_ADDRESSU,D3DTADDRESS_WRAP);
//				hr = OpenGLD3D::g_pd3dDevice->SetSamplerState(index,D3DSAMP_ADDRESSV,D3DTADDRESS_WRAP);
//				hr = OpenGLD3D::g_pd3dDevice->SetSamplerState(index,D3DSAMP_MAGFILTER,D3DTEXF_LINEAR);
//				hr = OpenGLD3D::g_pd3dDevice->SetSamplerState(index,D3DSAMP_MINFILTER,D3DTEXF_LINEAR);
//				hr = OpenGLD3D::g_pd3dDevice->SetSamplerState(index,D3DSAMP_MIPFILTER,D3DTEXF_NONE);
			}
		}
	}
	return index;
}

int Shader::SetSamplerTex(const char* name, const Texture* tex)
{
	return SetSamplerDX(name,tex?tex->GetTexture():NULL);
}

Shader::~Shader()
{
	SAFE_RELEASE(m_vertexShader);
	SAFE_RELEASE(m_pixelShader);
}

#endif
