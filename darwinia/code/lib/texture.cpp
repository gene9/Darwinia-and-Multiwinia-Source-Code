#include "lib/universal_include.h"

#ifdef USE_DIRECT3D

#include "lib/texture.h"
#include "lib/debug_utils.h"
#include "lib/opengl_directx_internals.h"

#pragma comment(lib,"d3dx9.lib")

#define CHECK_HR(hr) {DarwiniaDebugAssert(SUCCEEDED(hr)); if(FAILED(hr)) return;}

Texture* Texture::Create(const TextureParams& tp)
{
	Texture* t = new Texture(tp);
	if(!t->m_D3DTexture2D && !t->m_D3DTextureCube) SAFE_DELETE(t);
	return t;
}

Texture::Texture(const TextureParams& tp) : m_textureParams(tp)
{
	// init all attributes to deterministic values
	m_D3DTexture2D = NULL;
	m_D3DTextureCube = NULL;
	for(unsigned i=0;i<6;i++) m_D3DRenderTarget[i] = NULL;

	// size must be positive
	DarwiniaDebugAssert(tp.m_w && tp.m_h);

	// rendertarget & lockable at once not possible
	DarwiniaDebugAssert(!(tp.m_flags&TF_RENDERTARGET) || !(tp.m_flags&TF_LOCKABLE));

	// set d3d textures
	unsigned flags;
	D3DPOOL pool;
	if(tp.m_flags&TF_RENDERTARGET)
	{
		flags = D3DUSAGE_RENDERTARGET;
		pool = D3DPOOL_DEFAULT;
	}
	else
	if(tp.m_flags&TF_LOCKABLE)
	{
		flags = D3DUSAGE_DYNAMIC;
		pool = D3DPOOL_DEFAULT;
	}
	else
	{
		flags = 0;
		pool = D3DPOOL_MANAGED;
	}
	if(tp.m_flags&TF_CUBE)
	{
		DarwiniaDebugAssert(tp.m_w==tp.m_h);
		HRESULT hr = OpenGLD3D::g_pd3dDevice->CreateCubeTexture(tp.m_w,1,flags,tp.m_format,pool,&m_D3DTextureCube,NULL);
		CHECK_HR(hr);
	}
	else
	{
		HRESULT hr = OpenGLD3D::g_pd3dDevice->CreateTexture(tp.m_w,tp.m_h,1,flags,tp.m_format,pool,&m_D3DTexture2D,NULL);
		CHECK_HR(hr);
	}

	// set d3d surfaces
	if(tp.m_flags&TF_RENDERTARGET)
	{
		if(tp.m_flags&TF_CUBE)
		{
			if(m_D3DTextureCube)
				for(unsigned i=0;i<6;i++)
				{
					HRESULT hr = m_D3DTextureCube->GetCubeMapSurface((D3DCUBEMAP_FACES)i, 0, &m_D3DRenderTarget[i]);
					CHECK_HR(hr);
				}
		}
		else
		{
			if(m_D3DTexture2D)
			{
				HRESULT hr = m_D3DTexture2D->GetSurfaceLevel(0, &m_D3DRenderTarget[0]);
				CHECK_HR(hr);
			}
		}
	}
}

const TextureParams& Texture::GetParams() const
{
	return m_textureParams;
}

IDirect3DBaseTexture9* Texture::GetTexture()
{
	if(m_textureParams.m_flags&TF_CUBE) return m_D3DTextureCube;
	return m_D3DTexture2D;
}

IDirect3DCubeTexture9* Texture::GetTextureCube()
{
	if(m_textureParams.m_flags&TF_CUBE) return m_D3DTextureCube;
	return NULL;
}

IDirect3DTexture9* Texture::GetTexture2D()
{
	if(!(m_textureParams.m_flags&TF_CUBE)) return m_D3DTexture2D;
	return NULL;
}

IDirect3DSurface9* Texture::GetRenderTarget(unsigned index)
{
	DarwiniaDebugAssert(index < 6);
	DarwiniaDebugAssert(index==0 || (m_textureParams.m_flags&TF_CUBE));
	DarwiniaDebugAssert(m_textureParams.m_flags&TF_RENDERTARGET);
	return m_D3DRenderTarget[index];
}

Texture::~Texture()
{
	for(unsigned i=0;i<6;i++)
	{
		if(m_D3DRenderTarget[i]) m_D3DRenderTarget[i]->Release();
	}
	if(m_D3DTexture2D) m_D3DTexture2D->Release();
	if(m_D3DTextureCube) m_D3DTextureCube->Release();
}

#endif
