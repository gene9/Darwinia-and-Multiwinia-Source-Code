#ifndef __OPENGL_DIRECTX_INTERNALS_H
#define __OPENGL_DIRECTX_INTERNALS_H

namespace OpenGLD3D {

	struct CustomVertex {
		float x, y, z;
		float nx, ny, nz;
		union
		{
			D3DCOLOR diffuse;
			struct
			{
				unsigned char b8,g8,r8,a8;
			};
		};
		float u, v;
		float u2, v2;
	};

	extern LPDIRECT3DDEVICE9 g_pd3dDeviceActual, g_pd3dDevice;
	extern IDirect3DVertexBuffer9 **g_currentVertexBuffer;
	extern bool g_supportsHwVertexProcessing;

	void __stdcall glActiveTextureD3D(int _target);
	void __stdcall glMultiTexCoord2fD3D(int _target, float _x, float _y);
}

#endif // __OPENGL_DIRECTX_INTERNALS_H