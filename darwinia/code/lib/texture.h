#ifndef INCLUDED_TEXTURE_H
#define INCLUDED_TEXTURE_H

#ifdef USE_DIRECT3D

#include <d3dx9.h>

//
// D3D texture with support for
//  - becoming render target
//  - cube maps
//

//! Flags specifying texture type or features.
enum TextureFlags
{
	TF_RENDERTARGET   = 1<<0, ///< necessary if you plan to set texture as a render target
	TF_CUBE           = 1<<1, ///< texture is cube texture (opposed to 2d texture)
	TF_LOCKABLE       = 1<<2, ///< necessary if you plan to lock texture. can't be combined with TF_RENDERTARGET
};

//! Params specifying texture size, format etc.
struct TextureParams
{
	TextureParams(unsigned w, unsigned h, D3DFORMAT format, unsigned flags)
		: m_w(w), m_h(h), m_format(format), m_flags(flags)
	{
	}
	unsigned  m_w;
	unsigned  m_h;
	D3DFORMAT m_format;
	unsigned  m_flags;
};

//! Texture object
class Texture
{
public:
	static Texture* Create(const TextureParams& tp);
	const TextureParams& GetParams() const;
	IDirect3DBaseTexture9* GetTexture();
	IDirect3DTexture9* GetTexture2D();
	IDirect3DCubeTexture9* GetTextureCube();
	IDirect3DSurface9* GetRenderTarget(unsigned index = 0);
	~Texture();

private:
	Texture(const TextureParams& tp);
	TextureParams              m_textureParams;
	IDirect3DTexture9*         m_D3DTexture2D;
	IDirect3DCubeTexture9*     m_D3DTextureCube;
	IDirect3DSurface9*         m_D3DRenderTarget[6];
};

#endif

#endif
