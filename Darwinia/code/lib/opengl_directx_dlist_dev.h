#ifndef OPENGL_DIRECTX_DLIST_DEV_H
#define OPENGL_DIRECTX_DLIST_DEV_H

#include "lib/opengl_directx_internals.h"

// We use  device to save the commands for display list setting.

#include <vector>

#define DLIST_DEV_NOT_IMPLEMENTED { DarwiniaDebugAssert(FALSE); return 0; }
#define DLIST_DEV_NOT_IMPLEMENTED_VOID { DarwiniaDebugAssert(FALSE); }

namespace OpenGLD3D {

	class DisplayList;
	class Command;
	class MatrixStack;

	class DisplayListDevice : public IDirect3DDevice9 {
		/*** Implemented methods ***/
		STDMETHOD(DrawPrimitiveUP)(D3DPRIMITIVETYPE PrimitiveType,UINT PrimitiveCount,CONST void* pVertexStreamZeroData,UINT VertexStreamZeroStride);

		/*** IUnknown methods ***/
		STDMETHOD(QueryInterface)(REFIID riid, void** ppvObj) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD_(ULONG,AddRef)() DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD_(ULONG,Release)() DLIST_DEV_NOT_IMPLEMENTED;

		/*** IDirect3DDevice9 methods ***/
		STDMETHOD(TestCooperativeLevel)() DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD_(UINT, GetAvailableTextureMem)() DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(EvictManagedResources)() DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(GetDirect3D)(IDirect3D9** ppD3D9) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(GetDeviceCaps)(D3DCAPS9* pCaps) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(GetDisplayMode)(UINT iSwapChain,D3DDISPLAYMODE* pMode) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(GetCreationParameters)(D3DDEVICE_CREATION_PARAMETERS *pParameters) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(SetCursorProperties)(UINT XHotSpot,UINT YHotSpot,IDirect3DSurface9* pCursorBitmap) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD_(void, SetCursorPosition)(int X,int Y,DWORD Flags) DLIST_DEV_NOT_IMPLEMENTED_VOID;
		STDMETHOD_(BOOL, ShowCursor)(BOOL bShow) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(CreateAdditionalSwapChain)(D3DPRESENT_PARAMETERS* pPresentationParameters,IDirect3DSwapChain9** pSwapChain) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(GetSwapChain)(UINT iSwapChain,IDirect3DSwapChain9** pSwapChain) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD_(UINT, GetNumberOfSwapChains)() DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(Reset)(D3DPRESENT_PARAMETERS* pPresentationParameters) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(Present)(CONST RECT* pSourceRect,CONST RECT* pDestRect,HWND hDestWindowOverride,CONST RGNDATA* pDirtyRegion) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(GetBackBuffer)(UINT iSwapChain,UINT iBackBuffer,D3DBACKBUFFER_TYPE Type,IDirect3DSurface9** ppBackBuffer) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(GetRasterStatus)(UINT iSwapChain,D3DRASTER_STATUS* pRasterStatus) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(SetDialogBoxMode)(BOOL bEnableDialogs) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD_(void, SetGammaRamp)(UINT iSwapChain,DWORD Flags,CONST D3DGAMMARAMP* pRamp) DLIST_DEV_NOT_IMPLEMENTED_VOID;
		STDMETHOD_(void, GetGammaRamp)(UINT iSwapChain,D3DGAMMARAMP* pRamp) DLIST_DEV_NOT_IMPLEMENTED_VOID;
		STDMETHOD(CreateTexture)(UINT Width,UINT Height,UINT Levels,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DTexture9** ppTexture,HANDLE* pSharedHandle) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(CreateVolumeTexture)(UINT Width,UINT Height,UINT Depth,UINT Levels,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DVolumeTexture9** ppVolumeTexture,HANDLE* pSharedHandle) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(CreateCubeTexture)(UINT EdgeLength,UINT Levels,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DCubeTexture9** ppCubeTexture,HANDLE* pSharedHandle) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(CreateVertexBuffer)(UINT Length,DWORD Usage,DWORD FVF,D3DPOOL Pool,IDirect3DVertexBuffer9** ppVertexBuffer,HANDLE* pSharedHandle) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(CreateIndexBuffer)(UINT Length,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DIndexBuffer9** ppIndexBuffer,HANDLE* pSharedHandle) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(CreateRenderTarget)(UINT Width,UINT Height,D3DFORMAT Format,D3DMULTISAMPLE_TYPE MultiSample,DWORD MultisampleQuality,BOOL Lockable,IDirect3DSurface9** ppSurface,HANDLE* pSharedHandle) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(CreateDepthStencilSurface)(UINT Width,UINT Height,D3DFORMAT Format,D3DMULTISAMPLE_TYPE MultiSample,DWORD MultisampleQuality,BOOL Discard,IDirect3DSurface9** ppSurface,HANDLE* pSharedHandle) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(UpdateSurface)(IDirect3DSurface9* pSourceSurface,CONST RECT* pSourceRect,IDirect3DSurface9* pDestinationSurface,CONST POINT* pDestPoint) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(UpdateTexture)(IDirect3DBaseTexture9* pSourceTexture,IDirect3DBaseTexture9* pDestinationTexture) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(GetRenderTargetData)(IDirect3DSurface9* pRenderTarget,IDirect3DSurface9* pDestSurface) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(GetFrontBufferData)(UINT iSwapChain,IDirect3DSurface9* pDestSurface) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(StretchRect)(IDirect3DSurface9* pSourceSurface,CONST RECT* pSourceRect,IDirect3DSurface9* pDestSurface,CONST RECT* pDestRect,D3DTEXTUREFILTERTYPE Filter) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(ColorFill)(IDirect3DSurface9* pSurface,CONST RECT* pRect,D3DCOLOR color) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(CreateOffscreenPlainSurface)(UINT Width,UINT Height,D3DFORMAT Format,D3DPOOL Pool,IDirect3DSurface9** ppSurface,HANDLE* pSharedHandle) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(SetRenderTarget)(DWORD RenderTargetIndex,IDirect3DSurface9* pRenderTarget) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(GetRenderTarget)(DWORD RenderTargetIndex,IDirect3DSurface9** ppRenderTarget) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(SetDepthStencilSurface)(IDirect3DSurface9* pNewZStencil) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(GetDepthStencilSurface)(IDirect3DSurface9** ppZStencilSurface) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(BeginScene)() DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(EndScene)() DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(Clear)(DWORD Count,CONST D3DRECT* pRects,DWORD Flags,D3DCOLOR Color,float Z,DWORD Stencil) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(GetTransform)(D3DTRANSFORMSTATETYPE State,D3DMATRIX* pMatrix) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(SetViewport)(CONST D3DVIEWPORT9* pViewport) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(GetViewport)(D3DVIEWPORT9* pViewport) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(SetMaterial)(CONST D3DMATERIAL9* pMaterial) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(GetMaterial)(D3DMATERIAL9* pMaterial) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(SetLight)(DWORD Index,CONST D3DLIGHT9*) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(GetLight)(DWORD Index,D3DLIGHT9*) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(LightEnable)(DWORD Index,BOOL Enable) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(GetLightEnable)(DWORD Index,BOOL* pEnable) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(SetClipPlane)(DWORD Index,CONST float* pPlane) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(GetClipPlane)(DWORD Index,float* pPlane) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(SetRenderState)(D3DRENDERSTATETYPE State,DWORD Value) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(GetRenderState)(D3DRENDERSTATETYPE State,DWORD* pValue) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(CreateStateBlock)(D3DSTATEBLOCKTYPE Type,IDirect3DStateBlock9** ppSB) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(BeginStateBlock)() DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(EndStateBlock)(IDirect3DStateBlock9** ppSB) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(SetClipStatus)(CONST D3DCLIPSTATUS9* pClipStatus) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(GetClipStatus)(D3DCLIPSTATUS9* pClipStatus) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(GetTexture)(DWORD Stage,IDirect3DBaseTexture9** ppTexture) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(SetTexture)(DWORD Stage,IDirect3DBaseTexture9* pTexture) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(GetTextureStageState)(DWORD Stage,D3DTEXTURESTAGESTATETYPE Type,DWORD* pValue) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(SetTextureStageState)(DWORD Stage,D3DTEXTURESTAGESTATETYPE Type,DWORD Value) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(GetSamplerState)(DWORD Sampler,D3DSAMPLERSTATETYPE Type,DWORD* pValue) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(SetSamplerState)(DWORD Sampler,D3DSAMPLERSTATETYPE Type,DWORD Value) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(ValidateDevice)(DWORD* pNumPasses) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(SetPaletteEntries)(UINT PaletteNumber,CONST PALETTEENTRY* pEntries) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(GetPaletteEntries)(UINT PaletteNumber,PALETTEENTRY* pEntries) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(SetCurrentTexturePalette)(UINT PaletteNumber) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(GetCurrentTexturePalette)(UINT *PaletteNumber) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(SetScissorRect)(CONST RECT* pRect) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(GetScissorRect)(RECT* pRect) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(SetSoftwareVertexProcessing)(BOOL bSoftware) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD_(BOOL, GetSoftwareVertexProcessing)() DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(SetNPatchMode)(float nSegments) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD_(float, GetNPatchMode)() DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(DrawPrimitive)(D3DPRIMITIVETYPE PrimitiveType,UINT StartVertex,UINT PrimitiveCount) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(DrawIndexedPrimitive)(D3DPRIMITIVETYPE,INT BaseVertexIndex,UINT MinVertexIndex,UINT NumVertices,UINT startIndex,UINT primCount) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(DrawIndexedPrimitiveUP)(D3DPRIMITIVETYPE PrimitiveType,UINT MinVertexIndex,UINT NumVertices,UINT PrimitiveCount,CONST void* pIndexData,D3DFORMAT IndexDataFormat,CONST void* pVertexStreamZeroData,UINT VertexStreamZeroStride) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(ProcessVertices)(UINT SrcStartIndex,UINT DestIndex,UINT VertexCount,IDirect3DVertexBuffer9* pDestBuffer,IDirect3DVertexDeclaration9* pVertexDecl,DWORD Flags) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(CreateVertexDeclaration)(CONST D3DVERTEXELEMENT9* pVertexElements,IDirect3DVertexDeclaration9** ppDecl) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(SetVertexDeclaration)(IDirect3DVertexDeclaration9* pDecl) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(GetVertexDeclaration)(IDirect3DVertexDeclaration9** ppDecl) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(SetFVF)(DWORD FVF) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(GetFVF)(DWORD* pFVF) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(CreateVertexShader)(CONST DWORD* pFunction,IDirect3DVertexShader9** ppShader) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(SetVertexShader)(IDirect3DVertexShader9* pShader) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(GetVertexShader)(IDirect3DVertexShader9** ppShader) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(SetVertexShaderConstantF)(UINT StartRegister,CONST float* pConstantData,UINT Vector4fCount) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(GetVertexShaderConstantF)(UINT StartRegister,float* pConstantData,UINT Vector4fCount) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(SetVertexShaderConstantI)(UINT StartRegister,CONST int* pConstantData,UINT Vector4iCount) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(GetVertexShaderConstantI)(UINT StartRegister,int* pConstantData,UINT Vector4iCount) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(SetVertexShaderConstantB)(UINT StartRegister,CONST BOOL* pConstantData,UINT  BoolCount) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(GetVertexShaderConstantB)(UINT StartRegister,BOOL* pConstantData,UINT BoolCount) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(SetStreamSource)(UINT StreamNumber,IDirect3DVertexBuffer9* pStreamData,UINT OffsetInBytes,UINT Stride) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(GetStreamSource)(UINT StreamNumber,IDirect3DVertexBuffer9** ppStreamData,UINT* pOffsetInBytes,UINT* pStride) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(SetStreamSourceFreq)(UINT StreamNumber,UINT Setting) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(GetStreamSourceFreq)(UINT StreamNumber,UINT* pSetting) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(SetIndices)(IDirect3DIndexBuffer9* pIndexData) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(GetIndices)(IDirect3DIndexBuffer9** ppIndexData) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(CreatePixelShader)(CONST DWORD* pFunction,IDirect3DPixelShader9** ppShader) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(SetPixelShader)(IDirect3DPixelShader9* pShader) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(GetPixelShader)(IDirect3DPixelShader9** ppShader) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(SetPixelShaderConstantF)(UINT StartRegister,CONST float* pConstantData,UINT Vector4fCount) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(GetPixelShaderConstantF)(UINT StartRegister,float* pConstantData,UINT Vector4fCount) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(SetPixelShaderConstantI)(UINT StartRegister,CONST int* pConstantData,UINT Vector4iCount) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(GetPixelShaderConstantI)(UINT StartRegister,int* pConstantData,UINT Vector4iCount) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(SetPixelShaderConstantB)(UINT StartRegister,CONST BOOL* pConstantData,UINT  BoolCount) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(GetPixelShaderConstantB)(UINT StartRegister,BOOL* pConstantData,UINT BoolCount) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(DrawRectPatch)(UINT Handle,CONST float* pNumSegs,CONST D3DRECTPATCH_INFO* pRectPatchInfo) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(DrawTriPatch)(UINT Handle,CONST float* pNumSegs,CONST D3DTRIPATCH_INFO* pTriPatchInfo) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(DeletePatch)(UINT Handle) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(CreateQuery)(D3DQUERYTYPE Type,IDirect3DQuery9** ppQuery) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(SetTransform)(D3DTRANSFORMSTATETYPE State,CONST D3DMATRIX* pMatrix) DLIST_DEV_NOT_IMPLEMENTED;
		STDMETHOD(MultiplyTransform)(D3DTRANSFORMSTATETYPE,CONST D3DMATRIX*) DLIST_DEV_NOT_IMPLEMENTED;

	public:

		DisplayListDevice( unsigned  _id )
			: m_id( _id )
		{
		}

		DisplayList *Compile();

		unsigned  GetId() const
		{
			return m_id;
		}

		void RecordLoadTransform( MatrixStack *_matrixStack, D3DXMATRIX const &_mat );
		void RecordPushMatrix( MatrixStack *_matrixStack );
		void RecordPopMatrix( MatrixStack *_matrixStack );
		void RecordMultMatrix( MatrixStack *_matrixStack, D3DXMATRIX const &_mat );

	private:

		unsigned m_id;
		std::vector<Command *> m_commands;
		std::vector<CustomVertex> m_vertices;
	};
}


#endif // OPENGL_DIRECTX_DLIST_DEV_H
