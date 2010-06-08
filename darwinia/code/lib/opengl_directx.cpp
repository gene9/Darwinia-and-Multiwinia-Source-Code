#include "lib/universal_include.h"

#ifdef USE_DIRECT3D
#include "lib/opengl_trace.h"
#include "lib/debug_utils.h"

#include "lib/opengl_directx_internals.h"
#include "lib/opengl_directx_dlist_dev.h"
#include "lib/opengl_directx_dlist.h"
#include "lib/opengl_directx_matrix_stack.h"
#include "lib/ogl_extensions.h"
#include "lib/shader.h"
#include "lib/hi_res_time.h"

#include "app.h"
#include "deform.h"
#include "landscape.h"
#include "landscape_renderer.h"
#include "location.h"
#include "renderer.h"
#include "taskmanager_interface_gestures.h"
#include "water.h"
#include "water_reflection.h"
#include "window_manager.h"
#include "window_manager_win32.h"
#include "lib/input/input.h"
#include "lib/resource.h"

#include <limits>
#include <vector>
#include <map>
#include <stack>
#include <algorithm>

using namespace OpenGLD3D;

#define FRAMES_PER_SECOND_COUNTER
//#define D3DTS_WORLD D3DTS_VIEW

// Notes:
// OpenGL uses a RH coordinate system (+ve Z goes into the monitor),
// Direct3D uses LH coordinate system (-ve Z goes into the monitor)
// Matrix format is different in the two systems (One is the transpose of the other)
// gluLookAt -> D3DXMatrixLookAtRH (LH) ?
// gluOrtho, gluOrtho2D -> D3DXMatrixOrthoRH (LH) ?

/* TODO:

	- Implement GL_FOG_HINT and GL_POLYGON_HINT properly.
	- Proper error reporting in glGetError, gluErrorString
	- Line width emulation


	. Create matrix stack class, with modified flag, transform type (View / Projection)
		- constructor to get the current device transform
			- initialise with D3DTRANSFORMSTATETYPE
		- destructor so that it releases the internal matrix stack
		- Load
		- Multiply
		- Modified			(need to set the transform to sync up)
		- SetTransform		(set the device)
		- GetTransform

		- Look in the code for all ->SetTransform and ->GetTransform and recode as appropriate
*/

namespace OpenGLD3D {

	LPDIRECT3D9             g_pD3D       = NULL; // Used to create the D3DDevice
    D3DPRESENT_PARAMETERS	g_d3dpp;			 // The device parameters

	LPDIRECT3DDEVICE9       g_pd3dDevice = NULL;
	LPDIRECT3DDEVICE9		g_pd3dDeviceActual = NULL; // The device corresponding to the
	bool                    g_supportsHwVertexProcessing = true;
	bool					g_supportsAutoMipmapping = true;

	struct TextureState {
	public:
		GLuint target;
		GLuint lastTarget;
		D3DCOLORVALUE envColour;
		GLenum envMode;
		GLenum lastEnvMode;
		GLenum rgbCombineMode;

		TextureState()
			: target(0),
			  lastTarget(-1),
			  envMode(GL_MODULATE),
			  lastEnvMode(0),
			  rgbCombineMode(0)
		{
			ZeroMemory(&envColour, sizeof(envColour));
		}
	};
}

// Misc
static D3DCULL s_cullMode = D3DCULL_CW;
static bool s_ccwFrontFace = true;
static bool s_cullBackFace = true;

// Hints
static GLenum s_fogHint = GL_DONT_CARE;
static GLenum s_polygonSmoothHint = GL_DONT_CARE;

// Matrix and Projection
static GLenum s_matrixMode = GL_MODELVIEW;
static D3DTRANSFORMSTATETYPE s_targetMatrixTransformType = D3DTS_WORLD;
static MatrixStack *s_pTargetMatrixStack = NULL;

static MatrixStack *s_pModelViewMatrixStack = NULL;
static MatrixStack *s_pProjectionMatrixStack = NULL;

static MatrixStack *s_pModelViewMatrixStackActual = NULL;
static MatrixStack *s_pProjectionMatrixStackActual = NULL;

// Colours
static D3DCOLOR s_clearColor;

// Primitives
static GLenum s_primitiveMode = GL_QUADS;

// Vertices

static D3DVERTEXELEMENT9 s_customVertexDesc[] = {
	{0,  0, D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
	{0, 12, D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0},
	{0, 24, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0},
	{0, 28, D3DDECLTYPE_FLOAT2,    D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
	{0, 36, D3DDECLTYPE_FLOAT2,    D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1},
	{0xFF,0,D3DDECLTYPE_UNUSED, 0,0,0}// D3DDECL_END
};

LPDIRECT3DVERTEXDECLARATION9 s_pCustomVertexDecl;

static CustomVertex *s_vertices = NULL;
static CustomVertex *s_currentVertex = NULL;
static unsigned s_allocatedVertices = 0;
static unsigned s_currentVertexNumber = 0;

// Textures
static std::vector<LPDIRECT3DTEXTURE9> s_textureIds;

// Texture Samplers
const int MAX_ACTIVE_TEXTURES = 2;
unsigned s_activeTexture = 0;
static TextureState s_textureStates[MAX_ACTIVE_TEXTURES];
static TextureState *s_activeTextureState = &s_textureStates[0];

// Display lists
static std::map<unsigned, DisplayList *> s_displayLists;
static unsigned s_lastDisplayId = 0;
static DisplayListDevice *s_pDisplayListDevice = NULL;

// Saved Attributes
class CurrentAttributes : public CustomVertex {
public:
	CurrentAttributes();

public:
									// Current raster position
									// GL_CURRENT_RASTER_POSITION_VALID flag
									// RGBA color associated with current raster position
									// Color index associated with current raster position
									// Texture coordinates associated with current raster position
									// GL_EDGE_FLAG flag

	float	alphaRef;				// Store precise value of alpha ref

	bool	texturingEnabled[MAX_ACTIVE_TEXTURES];		//			(GL_ENABLE_BIT)
	bool	backCullingEnabled;		//								(GL_CULL_FACE)

	bool	colorMaterialEnabled;	// Material properties			(GL_ENABLE_BIT, GL_LIGHTING_BIT)
	GLenum	colorMaterialMode;		//								(GL_LIGHTING_BIT)
};

static CurrentAttributes s_currentAttribs;

// Attribute Stack
class Attributes {
public:
	Attributes( const CurrentAttributes &_currentAttribs, IDirect3DStateBlock9 *_state );

public:
	CurrentAttributes m_currentAttribs;
	IDirect3DStateBlock9 *m_d3dState;
};

static std::stack<Attributes> s_attributeStack;

// --- Attributes -----------------------------------------------------------------

Attributes::Attributes( const CurrentAttributes &_currentAttribs, IDirect3DStateBlock9 *_state )
	: m_currentAttribs( _currentAttribs ),
	  m_d3dState( _state )
{
}

// --- CurrentAttributes ----------------------------------------------------------

CurrentAttributes::CurrentAttributes()
{
	ZeroMemory(this, sizeof(*this));
	colorMaterialMode = GL_DIFFUSE;
	a8 = 255;
}

// --- The rest -------------------------------------------------------------------

static void InitialiseData()
{
	D3DXMATRIX m;

	// Matrix Data

	if (s_pModelViewMatrixStackActual != NULL) {
		delete s_pModelViewMatrixStackActual;
		delete s_pProjectionMatrixStackActual;
	}

	s_pModelViewMatrixStackActual = new MatrixStackActual( D3DTS_WORLD );
	s_pProjectionMatrixStackActual = new MatrixStackActual( D3DTS_PROJECTION );

	s_matrixMode = GL_MODELVIEW;
	s_targetMatrixTransformType = D3DTS_WORLD;
	s_pModelViewMatrixStack = s_pModelViewMatrixStackActual;
	s_pProjectionMatrixStack = s_pProjectionMatrixStackActual;
	s_pTargetMatrixStack = s_pModelViewMatrixStack;

	// Colours
	s_clearColor = D3DCOLOR_RGBA(0, 0, 0, 255);

	// Vertex data

	g_pd3dDevice->CreateVertexDeclaration( s_customVertexDesc, &s_pCustomVertexDecl );

	if (s_vertices == NULL) {
		s_allocatedVertices = 1024;
		s_vertices = new CustomVertex[s_allocatedVertices];
	}
	s_currentVertexNumber = 0;
	s_currentVertex = s_vertices;

	// Textures
	s_textureIds.clear();
	s_activeTexture = 0;
	s_activeTextureState = &s_textureStates[s_activeTexture];
	for (int i = 0; i < MAX_ACTIVE_TEXTURES; i++)
		s_textureStates[i] = TextureState();

	// Misc
	s_ccwFrontFace = true;
	s_cullBackFace = true;
	s_cullMode = D3DCULL_CW;

	// Hints
	s_fogHint = GL_DONT_CARE;
	s_polygonSmoothHint = GL_DONT_CARE;

	// Current
	s_currentAttribs = CurrentAttributes();
}

// d3d pool default resources are mostly automatically created
//  (on misc places) first time they are needed
void CreateD3dPoolDefaultResources()
{
	DarwiniaDebugAssert(!g_deformEffect);
	if(!g_deformEffect)
	{
		g_deformEffect = DeformEffect::Create();
	}
	//DarwiniaDebugAssert(!g_waterReflectionEffect);
	if(!g_waterReflectionEffect)
	{
		g_waterReflectionEffect = WaterReflectionEffect::Create();
	}
}

// to be called before d3d reset
void ReleaseD3DPoolDefaultResources()
{
	SAFE_DELETE(g_deformEffect);
	SAFE_DELETE(g_waterReflectionEffect);
	if(g_app && g_app->m_location && g_app->m_location->m_water)
	{
		g_app->m_location->m_water->ReleaseD3DPoolDefaultResources();
	}
	if(g_app && g_app->m_location && g_app->m_location->m_landscape.m_renderer)
	{
		g_app->m_location->m_landscape.m_renderer->ReleaseD3DPoolDefaultResources();
	}
	for(int i=0;i<s_textureIds.size();i++)
	{
		SAFE_RELEASE(s_textureIds[i]);
	}
}

// d3d resources are mostly automatically created
//  (on misc places) first time they are needed
void CreateD3DResources()
{
	CreateD3dPoolDefaultResources();
}

// to be called before d3d shutdown
void ReleaseD3DResources()
{
	ReleaseD3DPoolDefaultResources();
	if(g_app && g_app->m_location && g_app->m_location->m_water)
	{
		g_app->m_location->m_water->ReleaseD3DResources();
	}
	if(g_app && g_app->m_location && g_app->m_location->m_landscape.m_renderer)
	{
		g_app->m_location->m_landscape.m_renderer->ReleaseD3DResources();
	}
	TaskManagerInterfaceGestures::ReleaseD3dResources();
}

bool Direct3DInit(HWND _hWnd, bool _windowed, int _width, int _height, int _colourDepth, int _zDepth, bool _waitVRT)
{
    // Create the D3D object.
	if (!g_pD3D)
	{
		if( NULL == ( g_pD3D = Direct3DCreate9( D3D_SDK_VERSION ) ) )
		{
			// fatal end, no res change can help
			DarwiniaReleaseAssert(0,"Direct3D 9 not available.");
		}
	}

	// Detect hw vertex processing support.
	D3DCAPS9 caps;
	if(SUCCEEDED(g_pD3D->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,&caps)))
	{
		g_supportsHwVertexProcessing = caps.DevCaps&D3DDEVCAPS_HWTRANSFORMANDLIGHT;
		g_supportsAutoMipmapping = caps.Caps2&D3DCAPS2_CANAUTOGENMIPMAP;
	}
	// disable it for testing
	//g_supportsHwVertexProcessing = 0;

    // Set up the structure used to create the D3DDevice. Since we are now
    // using more complex geometry, we will create a device with a zbuffer.
    ZeroMemory( &g_d3dpp, sizeof(g_d3dpp) );

	// Default setting for the back buffer format
	g_d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
	D3DFORMAT displayFormat = D3DFMT_UNKNOWN;

	/*/ Triple buffering, turn off in safe mode
	if(_waitVRT && // makes sense only when waiting for retrace
		(_width>640 || _height>480 || !_windowed) // let's try it off in safe mode to save memory
		)
	{
		// - on slow computer, could make latency higher
		// + could prevent some jerkiness (but not all)
		// + on slow computer, could make fps higher
		g_d3dpp.BackBufferCount = 2;
	}
	*/

	if (_windowed) {
		g_d3dpp.Windowed = TRUE;
	}
	else {
		g_d3dpp.Windowed = FALSE;
		g_d3dpp.BackBufferWidth = _width;
		g_d3dpp.BackBufferHeight = _height;
		switch (_colourDepth) {
			case 24:
			case 32:
				g_d3dpp.BackBufferFormat = D3DFMT_X8R8G8B8;
				displayFormat = D3DFMT_X8R8G8B8;
				break;
			case 15:
			case 16:
				g_d3dpp.BackBufferFormat = D3DFMT_X1R5G5B5;
				displayFormat = D3DFMT_X1R5G5B5;
				break;
			default:
				DarwiniaReleaseAssert(0,"Unexpected colourDepth.");
		}
	}

	g_d3dpp.PresentationInterval = _waitVRT ? D3DPRESENT_INTERVAL_ONE : D3DPRESENT_INTERVAL_IMMEDIATE;
	g_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;

	if (_zDepth) {
		g_d3dpp.EnableAutoDepthStencil = TRUE;
		switch (_zDepth) {
			case 16:
				g_d3dpp.AutoDepthStencilFormat = D3DFMT_D16;//D3DFMT_D15S1
				break;
			case 24:
				g_d3dpp.AutoDepthStencilFormat = D3DFMT_D24X8;//D3DFMT_D24S8;D3DFMT_D24FS8;D3DFMT_D24X4S4;
				break;
			case 32:
				g_d3dpp.AutoDepthStencilFormat = D3DFMT_D32;
				break;
			default:
				DarwiniaReleaseAssert(0,"Unexpected zDepth.");
		}
	}
	g_d3dpp.hDeviceWindow = _hWnd;

	// Adjust format.
	if(!_windowed)
	{
		HRESULT hr = g_pD3D->CheckDeviceType(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, displayFormat, g_d3dpp.BackBufferFormat, g_d3dpp.Windowed);
		if(FAILED(hr))
		{
			if(_colourDepth==15 || _colourDepth==16)
			{
				// Fixes 16bit modes on Radeon X300
				g_d3dpp.BackBufferFormat = D3DFMT_R5G6B5;
				displayFormat = D3DFMT_R5G6B5;
				HRESULT hr = g_pD3D->CheckDeviceType(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, displayFormat, g_d3dpp.BackBufferFormat, g_d3dpp.Windowed);
				if(FAILED(hr))
				{
					// last existing case
					g_d3dpp.BackBufferFormat = D3DFMT_A1R5G5B5;
					displayFormat = D3DFMT_X1R5G5B5;
				}
			}
			else
			{
				// last existing case
				g_d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;
				displayFormat = D3DFMT_X8R8G8B8;
			}
		}
		hr = g_pD3D->CheckDeviceType(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, displayFormat, g_d3dpp.BackBufferFormat, g_d3dpp.Windowed);
		DarwiniaDebugAssert(SUCCEEDED(hr));
		if(!SUCCEEDED(hr))
		{
			// when all checks failed, return back to default mode and hope that something is wrong _only with checks_
			g_d3dpp.BackBufferFormat = (_colourDepth>=24) ? D3DFMT_X8R8G8B8 : D3DFMT_X1R5G5B5;
		}
	}

    // Create the D3DDevice
    if( FAILED( g_pD3D->CreateDevice(
		// NVPerfHud adapter
		//g_pD3D->GetAdapterCount()-1, D3DDEVTYPE_REF,
		// standard adapter
		D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
		_hWnd, g_supportsHwVertexProcessing?D3DCREATE_HARDWARE_VERTEXPROCESSING:D3DCREATE_SOFTWARE_VERTEXPROCESSING,&g_d3dpp, &g_pd3dDevice ) ) )
    {
		// non fatal end, res change can help
		//DarwiniaReleaseAssert( 0, "Failed to set screen mode" );
        return false;
    }


	g_pd3dDeviceActual = g_pd3dDevice;

	InitialiseData();

	g_pd3dDevice->BeginScene();
	g_pd3dDevice->SetVertexDeclaration( s_pCustomVertexDecl );
	// g_pd3dDevice->SetFVF(CustomVertexFVF);

	CreateD3DResources();

	// setup for specular
	g_pd3dDevice->SetRenderState( D3DRS_LOCALVIEWER, FALSE);

	return true;
}

void Direct3DShutdown()
{
	ReleaseD3DResources();
	SAFE_RELEASE(g_pd3dDevice);
	SAFE_RELEASE(g_pD3D);
}

void WaitAndResetDevice()
{
	ReleaseD3DPoolDefaultResources();

	// wait until device can be reset and reset device
	while (true)
	{
		Sleep( 100 );

		HRESULT hr;
		if (FAILED(hr = g_pd3dDevice->TestCooperativeLevel()))
		{
			if (hr == D3DERR_DEVICENOTRESET)
			{
				hr = g_pd3dDeviceActual->Reset( &g_d3dpp );
				if (FAILED(hr))
					continue;
				break;
			}
		}

		g_inputManager->PollForEvents();
	}

	//delete g_app->m_renderer;
	//g_app->m_renderer = new Renderer();
	//g_app->m_renderer->Restart();
	g_app->m_resource->FlushOpenGlState();
	CreateD3dPoolDefaultResources();
	g_app->m_resource->RegenerateOpenGlState();
	g_pd3dDevice->SetVertexDeclaration( s_pCustomVertexDecl );
}

void Direct3DSwapBuffers()
{
	g_pd3dDevice->EndScene();
	if (g_pd3dDevice->Present(NULL, NULL, NULL, NULL) == D3DERR_DEVICELOST)
	{
		WaitAndResetDevice();
	}
	g_pd3dDevice->BeginScene();

#ifdef FRAMES_PER_SECOND_COUNTER
	static DWORD s_startTime = GetTickCount();
	static unsigned s_numFrames = 0;

	s_numFrames++;

	if (GetTickCount() - s_startTime > 5000) {
		//DebugOut("%6.2f FPS\n", 1000.0f * s_numFrames / (GetTickCount() - s_startTime));
		s_numFrames = 0;
		s_startTime = GetTickCount();
	}
#endif // FRAMES_PER_SECOND_COUNTER
}

void glClearColor (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
	GL_TRACE_IMP(" glClearColor(%10.4g, %10.4g, %10.4g, %10.4g)", (double) red, (double) green, (double) blue, (double) alpha)

	s_clearColor = D3DCOLOR_COLORVALUE(red, green, blue, alpha);
}

void glClear (GLbitfield mask)
{
	GL_TRACE_IMP(" glClear(%s)", glBitfieldToString(mask))

	DWORD flags = 0;

	if (mask & GL_COLOR_BUFFER_BIT)
		flags |= D3DCLEAR_TARGET;

	if (mask & GL_DEPTH_BUFFER_BIT)
		flags |= D3DCLEAR_ZBUFFER;

	// Not implemented:
	// OpenGL supports specifying which buffers to clear
	// (Front or Back buffers, left or right stereoscopic)
	// by means of glDrawBuffers

	g_pd3dDevice->Clear( 0, NULL, flags, s_clearColor, 1.0f, 0 );

	// Ensure that mask does not include any flags that we may not
	// have handled.
	DarwiniaDebugAssert( (mask & ~(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT)) == 0 );
}

void glMatrixMode (GLenum mode)
{
	GL_TRACE_IMP(" glMatrixMode(%s)", glEnumToString(mode))

	DarwiniaDebugAssert( mode == GL_MODELVIEW || mode == GL_PROJECTION );

	// Nothing to be done if we're already in this matrix mode
	if (s_matrixMode == mode)
		return;

	s_pTargetMatrixStack->FastSetTransform();

	s_matrixMode = mode;
	switch (mode) {
		case GL_MODELVIEW:
			s_pTargetMatrixStack = s_pModelViewMatrixStack;
			s_targetMatrixTransformType = D3DTS_WORLD;
			break;

		case GL_PROJECTION:
			s_pTargetMatrixStack = s_pProjectionMatrixStack;
			s_targetMatrixTransformType = D3DTS_PROJECTION;
			break;
	}
}

void glLoadIdentity ()
{
	GL_TRACE_IMP(" glLoadIdentity()")

	D3DXMATRIX m;
	D3DXMatrixIdentity(&m);
	s_pTargetMatrixStack->Load(m);
}

void gluOrtho2D (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top)
{
	GL_TRACE_IMP(" gluOrtho2D(%10.4g, %10.4g, %10.4g, %10.4g)", left, right, bottom, top)

	D3DXMATRIX m;
	D3DXMatrixOrthoOffCenterRH( &m, left, right, bottom, top, -1.0, +1.0 );
	s_pTargetMatrixStack->Load(m);
}

void gluLookAt (GLdouble eyex, GLdouble eyey, GLdouble eyez, GLdouble centerx, GLdouble centery, GLdouble centerz, GLdouble upx, GLdouble upy, GLdouble upz)
{
	GL_TRACE_IMP(" gluLookAt(%10.4g, %10.4g, %10.4g, %10.4g, %10.4g, %10.4g, %10.4g, %10.4g, %10.4g)", eyex, eyey, eyez, centerx, centery, centerz, upx, upy, upz)

	D3DXVECTOR3
		eye( (FLOAT) eyex,    (FLOAT) eyey,    (FLOAT) eyez ),
		 at( (FLOAT) centerx, (FLOAT) centery, (FLOAT) centerz ),
		 up( (FLOAT) upx,     (FLOAT) upy,     (FLOAT) upz );

	D3DXMATRIX m;
	D3DXMatrixLookAtRH( &m, &eye, &at, &up );
	s_pTargetMatrixStack->Load(m);
}

void gluPerspective (GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar)
{
	GL_TRACE_IMP(" gluPerspective(%10.4g, %10.4g, %10.4g, %10.4g)", fovy, aspect, zNear, zFar)

	// zNear The distance from the viewer to the near clipping plane (always positive).
	// zFar  The distance from the viewer to the far clipping plane (always positive).

	// zn [in] Z-value of the near view-plane.
	// zf [in] Z-value of the far view-plane.

	D3DXMATRIX m;
	D3DXMatrixPerspectiveFovRH( &m, D3DXToRadian((FLOAT) fovy), (FLOAT) aspect, (FLOAT) zNear, (FLOAT) zFar );
	// Preserve OpenGL compatibility
	m._43 *= 2.0f;
	s_pTargetMatrixStack->Load(m);
}


static void setupTexturing()
{
	// JAK: I think that the glPushAttrib stuff might be interfering with
	//      the commented optimisation below:

	//if (s.envMode != s.lastEnvMode ||
	//	s.target != s.lastTarget) {

		TextureState &s = *s_activeTextureState;

		s.lastEnvMode = s.envMode;
		s.lastTarget = s.target;

		g_pd3dDevice->SetTexture( s_activeTexture, s_textureIds[s.target] );

		// See table 9-15 in Red Book (page 418) for description of texture env modes.
		// We assume that all textures are in RGBA format.

		switch (s.envMode) {
			case GL_MODULATE:
				g_pd3dDevice->SetTextureStageState( s_activeTexture, D3DTSS_COLOROP,   D3DTOP_MODULATE );
				g_pd3dDevice->SetTextureStageState( s_activeTexture, D3DTSS_COLORARG1, D3DTA_TEXTURE );
				g_pd3dDevice->SetTextureStageState( s_activeTexture, D3DTSS_COLORARG2, D3DTA_DIFFUSE );

				g_pd3dDevice->SetTextureStageState( s_activeTexture, D3DTSS_ALPHAOP,   D3DTOP_MODULATE );
				g_pd3dDevice->SetTextureStageState( s_activeTexture, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
				g_pd3dDevice->SetTextureStageState( s_activeTexture, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );
				break;

			case GL_REPLACE:
				g_pd3dDevice->SetTextureStageState( s_activeTexture, D3DTSS_COLOROP, D3DTOP_SELECTARG1 );
				g_pd3dDevice->SetTextureStageState( s_activeTexture, D3DTSS_COLOROP, D3DTA_TEXTURE );

				g_pd3dDevice->SetTextureStageState( s_activeTexture, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG1 );
				g_pd3dDevice->SetTextureStageState( s_activeTexture, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
				break;

			case GL_DECAL:
				g_pd3dDevice->SetTextureStageState( s_activeTexture, D3DTSS_COLOROP,   D3DTOP_MODULATE );
				g_pd3dDevice->SetTextureStageState( s_activeTexture, D3DTSS_COLORARG1, D3DTA_TEXTURE );
				g_pd3dDevice->SetTextureStageState( s_activeTexture, D3DTSS_COLORARG2, D3DTA_DIFFUSE );

				g_pd3dDevice->SetTextureStageState( s_activeTexture, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG1 );
				g_pd3dDevice->SetTextureStageState( s_activeTexture, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE );
				break;

			case GL_COMBINE_EXT:
				switch (s.rgbCombineMode) {
					case GL_REPLACE:
						g_pd3dDevice->SetTextureStageState( s_activeTexture, D3DTSS_COLOROP, D3DTOP_SELECTARG1 );
						g_pd3dDevice->SetTextureStageState( s_activeTexture, D3DTSS_COLOROP, D3DTA_TEXTURE );


                        g_pd3dDevice->SetTextureStageState( s_activeTexture, D3DTSS_ALPHAOP,   D3DTOP_MODULATE );
                        g_pd3dDevice->SetTextureStageState( s_activeTexture, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
                        g_pd3dDevice->SetTextureStageState( s_activeTexture, D3DTSS_ALPHAARG2, D3DTA_CURRENT );

						//g_pd3dDevice->SetTextureStageState( s_activeTexture, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG1 );
						//g_pd3dDevice->SetTextureStageState( s_activeTexture, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
						break;

					case GL_MULT:
						g_pd3dDevice->SetTextureStageState( s_activeTexture, D3DTSS_COLOROP,   D3DTOP_MODULATE );
						g_pd3dDevice->SetTextureStageState( s_activeTexture, D3DTSS_COLORARG1, D3DTA_TEXTURE );
						g_pd3dDevice->SetTextureStageState( s_activeTexture, D3DTSS_COLORARG2, D3DTA_CURRENT );

						g_pd3dDevice->SetTextureStageState( s_activeTexture, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG1 );
						g_pd3dDevice->SetTextureStageState( s_activeTexture, D3DTSS_ALPHAARG1, D3DTA_CURRENT );
						break;

					case GL_MODULATE:
						g_pd3dDevice->SetTextureStageState( s_activeTexture, D3DTSS_COLOROP,   D3DTOP_MODULATE );
						g_pd3dDevice->SetTextureStageState( s_activeTexture, D3DTSS_COLORARG1, D3DTA_TEXTURE );
						g_pd3dDevice->SetTextureStageState( s_activeTexture, D3DTSS_COLORARG2, D3DTA_CURRENT );

						g_pd3dDevice->SetTextureStageState( s_activeTexture, D3DTSS_ALPHAOP,   D3DTOP_MODULATE );
						g_pd3dDevice->SetTextureStageState( s_activeTexture, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
						g_pd3dDevice->SetTextureStageState( s_activeTexture, D3DTSS_ALPHAARG2, D3DTA_CURRENT );
						break;
				}

		}
	//}
}


void __stdcall OpenGLD3D::glActiveTextureD3D(int _target)
{
	GL_TRACE_IMP(" glActiveTextureD3D(%d)", _target);

	// Multi-texturing

	_target -= GL_TEXTURE0_ARB;
	DarwiniaDebugAssert(_target >= 0 && _target < MAX_ACTIVE_TEXTURES);

	s_activeTexture = _target;
	s_activeTextureState = &s_textureStates[s_activeTexture];
}

void __stdcall OpenGLD3D::glMultiTexCoord2fD3D(int _target, float _x, float _y)
{
	_target -= GL_TEXTURE0_ARB;
	DarwiniaDebugAssert(_target >= 0 && _target < MAX_ACTIVE_TEXTURES);

	if (_target == 0) {
		s_currentAttribs.u = _x;
		s_currentAttribs.v = _y;
	}
	else {
		s_currentAttribs.u2 = _x;
		s_currentAttribs.v2 = _y;
	}
}


static void setupColorMaterial()
{
	if (s_currentAttribs.colorMaterialEnabled) {
		switch (s_currentAttribs.colorMaterialMode) {
			case GL_AMBIENT_AND_DIFFUSE:
				g_pd3dDevice->SetRenderState( D3DRS_AMBIENTMATERIALSOURCE, D3DMCS_COLOR1);
				g_pd3dDevice->SetRenderState( D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_COLOR1);
				g_pd3dDevice->SetRenderState( D3DRS_SPECULARMATERIALSOURCE, D3DMCS_MATERIAL);
				break;

			case GL_DIFFUSE:
				g_pd3dDevice->SetRenderState( D3DRS_AMBIENTMATERIALSOURCE, D3DMCS_MATERIAL);
				g_pd3dDevice->SetRenderState( D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_COLOR1);
				g_pd3dDevice->SetRenderState( D3DRS_SPECULARMATERIALSOURCE, D3DMCS_MATERIAL);
				break;

			default:
				DarwiniaDebugAssert( FALSE );
		}
	}
	else {
		g_pd3dDevice->SetRenderState( D3DRS_AMBIENTMATERIALSOURCE, D3DMCS_MATERIAL);
		g_pd3dDevice->SetRenderState( D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_MATERIAL);
		g_pd3dDevice->SetRenderState( D3DRS_SPECULARMATERIALSOURCE, D3DMCS_MATERIAL);
	}
}


void glDisable ( GLenum cap )
{
	GL_TRACE_IMP(" glDisable(%s)", glEnumToString(cap))

	switch (cap) {
		case GL_ALPHA_TEST:
			g_pd3dDevice->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
			break;

		case GL_BLEND:
			g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
			break;

        case GL_CLIP_PLANE0:
        case GL_CLIP_PLANE1:
		case GL_CLIP_PLANE2:
			{
				DWORD clipPlanes;
				g_pd3dDevice->GetRenderState(D3DRS_CLIPPLANEENABLE, &clipPlanes);
				g_pd3dDevice->SetRenderState(D3DRS_CLIPPLANEENABLE, clipPlanes&~(1<<(cap-GL_CLIP_PLANE0)));
				break;
			}

		case GL_COLOR_MATERIAL:
			s_currentAttribs.colorMaterialEnabled = false;
			setupColorMaterial();
			break;

		case GL_CULL_FACE:
			s_currentAttribs.backCullingEnabled = false;
			g_pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
			break;

		case GL_DEPTH_TEST:
			g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
			break;

		case GL_FOG:
			g_pd3dDevice->SetRenderState(D3DRS_FOGENABLE, FALSE);
			break;

		case GL_LIGHTING:
			g_pd3dDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
			break;

		case GL_LIGHT0:
		case GL_LIGHT1:
		case GL_LIGHT2:
		case GL_LIGHT3:
		case GL_LIGHT4:
		case GL_LIGHT5:
		case GL_LIGHT6:
		case GL_LIGHT7:
			g_pd3dDevice->LightEnable( cap - GL_LIGHT0, FALSE );
			break;

		case GL_LINE_SMOOTH:
			g_pd3dDevice->SetRenderState( D3DRS_ANTIALIASEDLINEENABLE, FALSE );
			break;

		case GL_NORMALIZE:
			g_pd3dDevice->SetRenderState( D3DRS_NORMALIZENORMALS, FALSE );
			break;

		case GL_SCISSOR_TEST:
			g_pd3dDevice->SetRenderState( D3DRS_SCISSORTESTENABLE, FALSE );
			break;

		case GL_POINT_SMOOTH:
			// Don't think Direct3D supports smooth (circular) points anyway
			break;

		case GL_TEXTURE_2D:
			s_currentAttribs.texturingEnabled[s_activeTexture] = false;
			g_pd3dDevice->SetTextureStageState( s_activeTexture, D3DTSS_COLOROP, D3DTOP_DISABLE );
			break;

		default:
			GL_TRACE("-glDisable(%s) not implemented", glEnumToString(cap))
	}
}

void glEnable ( GLenum cap )
{
	GL_TRACE_IMP(" glEnable(%s)", glEnumToString(cap))

	switch (cap) {
		case GL_ALPHA_TEST:
			g_pd3dDevice->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
			break;

		case GL_BLEND:
			g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
			break;

        case GL_CLIP_PLANE0:
        case GL_CLIP_PLANE1:
		case GL_CLIP_PLANE2:
			{
				DWORD clipPlanes;
				g_pd3dDevice->GetRenderState(D3DRS_CLIPPLANEENABLE, &clipPlanes);
				g_pd3dDevice->SetRenderState(D3DRS_CLIPPLANEENABLE, clipPlanes|(1<<(cap-GL_CLIP_PLANE0)));
				break;
			}

		case GL_COLOR_MATERIAL:
			s_currentAttribs.colorMaterialEnabled = true;
			setupColorMaterial();
			break;

		case GL_CULL_FACE:
			s_currentAttribs.backCullingEnabled = true;
			g_pd3dDevice->SetRenderState(D3DRS_CULLMODE, s_cullMode);
			break;

		case GL_DEPTH_TEST:
			g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, D3DZB_TRUE);
			break;

		case GL_FOG:
			g_pd3dDevice->SetRenderState(D3DRS_FOGENABLE, TRUE);
			break;

		case GL_LIGHTING:
			g_pd3dDevice->SetRenderState(D3DRS_LIGHTING, TRUE);
			break;

		case GL_LIGHT0:
		case GL_LIGHT1:
		case GL_LIGHT2:
		case GL_LIGHT3:
		case GL_LIGHT4:
		case GL_LIGHT5:
		case GL_LIGHT6:
		case GL_LIGHT7:
			g_pd3dDevice->LightEnable( cap - GL_LIGHT0, TRUE );
			break;

		case GL_LINE_SMOOTH:
			g_pd3dDevice->SetRenderState( D3DRS_ANTIALIASEDLINEENABLE, TRUE );
			break;

		case GL_NORMALIZE:
			g_pd3dDevice->SetRenderState( D3DRS_NORMALIZENORMALS, TRUE );
			break;

		case GL_SCISSOR_TEST:
			g_pd3dDevice->SetRenderState( D3DRS_SCISSORTESTENABLE, TRUE );
			break;

		case GL_TEXTURE_2D:
			if (s_textureIds[s_activeTextureState->target])
				setupTexturing();

			s_currentAttribs.texturingEnabled[s_activeTexture] = true;
			break;

		default:
			GL_TRACE("-glEnable(%s) not implemented", glEnumToString(cap))

	}
}

void glColor4ub( GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha )
{
	s_currentAttribs.r8 = red;
	s_currentAttribs.g8 = green;
	s_currentAttribs.b8 = blue;
	s_currentAttribs.a8 = alpha;
}

void glNormal3f_impl (GLfloat nx, GLfloat ny, GLfloat nz)
{
	// Direct3D Normals have to be inverted due to the left handed coordinate system.
	s_currentAttribs.nx = -nx;
	s_currentAttribs.ny = -ny;
	s_currentAttribs.nz = -nz;
}

static void reallocateVertexList()
{
	// Expand the vertex buffer
	unsigned allocatedVertices = s_allocatedVertices * 2;
	CustomVertex *buffer = new CustomVertex[allocatedVertices];

	memcpy(buffer, s_vertices, sizeof(CustomVertex) * s_currentVertexNumber);
	delete[] s_vertices;

	s_vertices = buffer;
	s_allocatedVertices = allocatedVertices;
	s_currentVertex = s_vertices + s_currentVertexNumber;
}

void glBegin ( GLenum mode )
{
	GL_TRACE_IMP(" glBegin(%s)", glEnumToString(mode))

	// glBegin is used to draw primitives
	// Note: opportunity for object orientation here
	//		 (Refactoring switch statements in glBegin, glEnd)

	s_currentVertex = s_vertices;
	s_currentVertexNumber = 0;
	s_primitiveMode = mode;
}

void glEnd ()
{
	GL_TRACE_IMP(" glEnd()")

	// Update the matrix if it has been modified (also updated when we switch matrix modes)
	s_pTargetMatrixStack->FastSetTransform();

	switch (s_primitiveMode) {
		case GL_POINTS:
			DarwiniaDebugAssert(s_currentVertexNumber>0);
			g_pd3dDevice->DrawPrimitiveUP(D3DPT_POINTLIST, s_currentVertexNumber, s_vertices, sizeof(CustomVertex));
			break;

		case GL_LINES:
			DarwiniaDebugAssert(s_currentVertexNumber%2==0);
			DarwiniaDebugAssert(s_currentVertexNumber>1);
			g_pd3dDevice->DrawPrimitiveUP(D3DPT_LINELIST, s_currentVertexNumber / 2, s_vertices, sizeof(CustomVertex));
			break;

		case GL_LINE_LOOP:
			DarwiniaDebugAssert(s_currentVertexNumber>1);
			if(s_currentVertexNumber+2 >= s_allocatedVertices)
				reallocateVertexList();
			s_vertices[s_currentVertexNumber] = s_vertices[0];
			g_pd3dDevice->DrawPrimitiveUP(D3DPT_LINESTRIP, s_currentVertexNumber, s_vertices, sizeof(CustomVertex));
			break;

		case GL_TRIANGLES:
			// Reverse the triangle vertices
			DarwiniaDebugAssert(s_currentVertexNumber%3==0);
			DarwiniaDebugAssert(s_currentVertexNumber>2);
			g_pd3dDevice->DrawPrimitiveUP(D3DPT_TRIANGLELIST, s_currentVertexNumber / 3, s_vertices, sizeof(CustomVertex));
			break;

		case GL_TRIANGLE_STRIP:
		case GL_QUAD_STRIP:
			DarwiniaDebugAssert(s_currentVertexNumber>2);
			if(s_currentVertexNumber>2) // invalid value here _could be_ cause of xander's problem
				g_pd3dDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, s_currentVertexNumber-2, s_vertices, sizeof(CustomVertex));
			break;

		case GL_TRIANGLE_FAN:
			DarwiniaDebugAssert(s_currentVertexNumber>2);
			g_pd3dDevice->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, s_currentVertexNumber-2, s_vertices, sizeof(CustomVertex));
			break;

		case GL_QUADS:
			// Inefficient, perhaps we should have two buffers, a vertex list, and an index list
			// When adding a vertex using glVertex, we could add just the vertex to the vertex list
			// and then add the appropriate indexes depending on the primitive mode.
			// Would require using proper vertex buffers though.
			{
				DarwiniaDebugAssert(s_currentVertexNumber%3==0);
				int numTris = s_currentVertexNumber / 3;
				// could be 0 in PaintPixels, and it's difficult to filter it out sooner
				if(numTris)
				{
					g_pd3dDevice->DrawPrimitiveUP(D3DPT_TRIANGLELIST, numTris, s_vertices, sizeof(CustomVertex));
				}
			}
			break;

		default:
			GL_TRACE("-glEnd() not implemented for %s", glEnumToString(s_primitiveMode));
			break;
	}

}

void glVertex3f_impl( GLfloat x, GLfloat y, GLfloat z )
{
	// Check to see if we need to expand the list
	if (s_currentVertexNumber+2 >= s_allocatedVertices)
		reallocateVertexList();

	*s_currentVertex = s_currentAttribs;
	s_currentVertex->x = x;
	s_currentVertex->y = y;
	s_currentVertex->z = z;

	s_currentVertex++;
	s_currentVertexNumber++;

    if (s_primitiveMode == GL_QUADS) {
        static int vertexCount = 0;

        if( vertexCount == 2 ) {
            // Add in some more vertices to make up the quad

            *s_currentVertex = s_vertices[s_currentVertexNumber - 3];
            s_currentVertex++;
            s_currentVertexNumber++;

            *s_currentVertex = s_vertices[s_currentVertexNumber - 2];
            s_currentVertex++;
            s_currentVertexNumber++;
        }

        vertexCount = (vertexCount + 1) & 3;
    }
}


void glGenTextures (GLsizei n, GLuint *textures)
{
	GL_TRACE_IMP(" glGenTextures(%d, (float *)%p)", n, textures)

	// We start off with a NULL pointer for the texture
	// This will be replaced with an actual pointer
	// by a call to glTexImage2D (or gluBuild2DMipmaps)

	for (int i = 0; i < n; i++) {
		textures[i] = s_textureIds.size();
		s_textureIds.push_back( 0 );
	}
}

void glBindTexture ( GLenum target, GLuint texture )
{
	GL_TRACE_IMP(" glBindTexture(%s, %u)", glEnumToString(target), texture)

	switch (target) {
		case GL_TEXTURE_2D:
			s_activeTextureState->target = texture;
			if (s_currentAttribs.texturingEnabled[s_activeTexture])
				setupTexturing();
			break;

		default:
			DarwiniaDebugAssert( target == GL_TEXTURE_2D );
			break;
	}
}

void glCopyTexImage2D (GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border)
{
	GL_TRACE_IMP(" glCopyTexImage2D(%s, %d, %s, %d, %d, %d, %d, %d)", glEnumToString(target), level, glEnumToString(internalFormat), x, y, width, height, border);

	LPDIRECT3DTEXTURE9 &pTexture = s_textureIds[s_activeTextureState->target];

	if (pTexture == NULL) {
		HRESULT hr = g_pd3dDeviceActual->CreateTexture( width, height, 1, D3DUSAGE_RENDERTARGET, g_d3dpp.BackBufferFormat, D3DPOOL_DEFAULT, &pTexture, NULL );
		DarwiniaDebugAssert( hr != D3DERR_INVALIDCALL );
		DarwiniaDebugAssert( hr != D3DXERR_INVALIDDATA );
	}

	// Get the backbuffer surface
	IDirect3DSurface9 *backbuffer = NULL;
	RECT backbufferRect = { x, y, x + width, y + height };
	HRESULT hr = g_pd3dDeviceActual->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &backbuffer );
	DarwiniaDebugAssert( hr == D3D_OK );

	// Get the texture surface
	IDirect3DSurface9 *textureSurface = NULL;
	hr = pTexture->GetSurfaceLevel(0,&textureSurface);
	DarwiniaDebugAssert( hr == D3D_OK );

	// Assume that the texture is the correct size.
	hr = g_pd3dDeviceActual->StretchRect(backbuffer, &backbufferRect, textureSurface, NULL, D3DTEXF_NONE); // No Filtering

	// Reference count maintenance
	textureSurface->Release();
	backbuffer->Release();
}

void glReadPixels (GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels)
{
	if(format!=GL_RGB) return;

	// from any format to BGRA
	Texture* screenTexture = Texture::Create(TextureParams(width,height,D3DFMT_A8R8G8B8,TF_RENDERTARGET));
	if(screenTexture)
	{
		IDirect3DSurface9* renderTarget;
		if(SUCCEEDED(OpenGLD3D::g_pd3dDevice->GetRenderTarget(0,&renderTarget)))
		{
			RECT rect = {0,0,width,height};
			if(SUCCEEDED(OpenGLD3D::g_pd3dDevice->StretchRect(renderTarget,&rect,screenTexture->GetRenderTarget(),&rect,D3DTEXF_POINT)))
			{
				// from video to system memory
				IDirect3DSurface9* memsurface;
				if(SUCCEEDED(g_pd3dDevice->CreateOffscreenPlainSurface(width,height,D3DFMT_A8R8G8B8,D3DPOOL_SYSTEMMEM,&memsurface,NULL)))
				{
					if(SUCCEEDED(g_pd3dDevice->GetRenderTargetData(screenTexture->GetRenderTarget(), memsurface)))
					{
						D3DLOCKED_RECT lrect;
						if(SUCCEEDED(memsurface->LockRect(&lrect,NULL,D3DLOCK_READONLY)))
						{
							// from BGRA to RGB up-down inverted
							char* dst = (char*)pixels;
							char* src = (char*)lrect.pBits;
							for(unsigned y=0;y<height;y++)
							{
								for(unsigned x=0;x<width;x++)
								{
									*dst++ = src[width*4*(height-1-y)+4*x+2];
									*dst++ = src[width*4*(height-1-y)+4*x+1];
									*dst++ = src[width*4*(height-1-y)+4*x+0];
								}
							}
							memsurface->UnlockRect();
						}
					}
					memsurface->Release();
				}
			}
			renderTarget->Release();
		}
		delete screenTexture;
	}
}


// This function is basically the same as glTexImage2D, except that this function is
// supposed to produce mipmapped textures

int gluBuild2DMipmaps (GLenum target, GLint components, GLint width, GLint height, GLenum format, GLenum type, const void *data)
{
	GL_TRACE_IMP(" gluBuild2DMipmaps(%s, %d, %d, %d, %s, %s, %p)", glEnumToString(target), components, width, height, glEnumToString(format), glEnumToString(type), data)

	DarwiniaDebugAssert( target == GL_TEXTURE_2D );

	LPDIRECT3DTEXTURE9 &pTexture = s_textureIds[s_activeTextureState->target];

	// Free the previous texture if necessary
	if (pTexture != NULL) {
		pTexture->Release();
		pTexture = NULL;
	}

	// We enforce that the width and height are powers of 2.
	// gluBuild2DMipmaps ought to be able to deal with it, but
	// it's easier to get the application to do it.
	DarwiniaDebugAssert( width == 1 << (int) ceil(log((float) width)/log(2.0f)) );
	DarwiniaDebugAssert( height == 1 << (int) ceil(log((float) height)/log(2.0f)) );

	// We also enforce that the format is convenient
	DarwiniaDebugAssert( format == GL_RGBA );
	DarwiniaDebugAssert( type == GL_UNSIGNED_BYTE );

	HRESULT rc = g_pd3dDevice->CreateTexture(width, height, 0, D3DUSAGE_AUTOGENMIPMAP, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &pTexture, NULL);
	DarwiniaDebugAssert( rc != D3DERR_INVALIDCALL );
	DarwiniaDebugAssert( rc != D3DXERR_INVALIDDATA );

	IDirect3DSurface9 *pSurface;
	rc = pTexture->GetSurfaceLevel(0, &pSurface);

	RECT rect = { 0, 0, width , height };

	// Triangle filtering is the default, but it is also the slowest filter
	// Box filtering ought to work ok for us.
	rc = D3DXLoadSurfaceFromMemory(
		pSurface, NULL, NULL, data, D3DFMT_A8B8G8R8, width * 4, NULL, &rect, D3DX_FILTER_NONE /* D3DX_FILTER_BOX */, 0);

	DarwiniaDebugAssert( rc != D3DERR_INVALIDCALL );
	DarwiniaDebugAssert( rc != D3DXERR_INVALIDDATA );

	pSurface->Release();

	pTexture->GenerateMipSubLevels();

	if (s_currentAttribs.texturingEnabled)
		setupTexturing();

	return 0;
}

void glTexImage2D (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels)
{
	GL_TRACE_IMP(" glTexImage2D(%s, %d, %d, %d, %d, %d, %s, %s, %p)", glEnumToString(target), level, internalformat, width, height, border, glEnumToString(format), glEnumToString(type), pixels)
	// glTexImage2D(GL_TEXTURE_2D, 0, 4, 16, 16, 0, GL_RGBA, GL_UNSIGNED_BYTE, 11786058)
	DarwiniaDebugAssert( target == GL_TEXTURE_2D );
	DarwiniaDebugAssert( level == 0 );
	DarwiniaDebugAssert( internalformat == 4 );
	DarwiniaDebugAssert( format == GL_RGBA );

	gluBuild2DMipmaps( target, 4, width, height, format, type, pixels );
}


void glTexCoord2f_impl ( GLfloat u, GLfloat v )
{
	s_currentAttribs.u = u;
	s_currentAttribs.v = v;
}

static DWORD glMinMagFilterToDirect3D(GLenum param)
{
	switch (param) {
		case GL_LINEAR: return D3DTEXF_LINEAR;
		case GL_NEAREST: return D3DTEXF_POINT;
		case GL_LINEAR_MIPMAP_LINEAR: return D3DTEXF_LINEAR;
		default:
			DarwiniaReleaseAssert(FALSE, "Unknown Filter %u", param);
			return D3DTEXF_NONE;
	}
}

static GLenum direct3DMinMagFilterToOpenGL(DWORD param)
{
	switch (param) {
		case D3DTEXF_LINEAR: return GL_LINEAR;
		case D3DTEXF_POINT: return GL_NEAREST;
		default:
			DarwiniaReleaseAssert(FALSE, "Unknown Filter %u", param);
			return 0;
	}
}

static DWORD glTextureWrapToDirect3D(GLenum param)
{
	switch (param) {
		case GL_CLAMP: return D3DTADDRESS_CLAMP;
		case GL_REPEAT: return D3DTADDRESS_WRAP;
		default:
			DarwiniaReleaseAssert(FALSE, "Unknown Texture Wrap mode %u", param);
			return D3DTADDRESS_WRAP;
	}
}


// Implement glTexParameteri , glTexParameterf
//  See: SetSamplerState and 'Texture Wrapping' and 'Texture Addressing Modes'

void glTexParameteri (GLenum target, GLenum pname, GLint param)
{
	// D3DTEXTUREADDRESS

	GL_TRACE_IMP(" glTexParameteri(%s, %s, %s)", glEnumToString(target), glEnumToString(pname), glEnumToString(param))
	DarwiniaDebugAssert( target == GL_TEXTURE_2D );
	switch (pname) {
		case GL_TEXTURE_WRAP_S:
			g_pd3dDevice->SetSamplerState( s_activeTexture, D3DSAMP_ADDRESSU, glTextureWrapToDirect3D(param) );
			break;

		case GL_TEXTURE_WRAP_T:
			g_pd3dDevice->SetSamplerState( s_activeTexture, D3DSAMP_ADDRESSV, glTextureWrapToDirect3D(param) );
			break;

		case GL_TEXTURE_MAG_FILTER:
			g_pd3dDevice->SetSamplerState( s_activeTexture, D3DSAMP_MAGFILTER, glMinMagFilterToDirect3D(param) );
			break;

		case GL_TEXTURE_MIN_FILTER:
			g_pd3dDevice->SetSamplerState( s_activeTexture, D3DSAMP_MINFILTER, glMinMagFilterToDirect3D(param) );
			g_pd3dDevice->SetSamplerState( s_activeTexture, D3DSAMP_MIPFILTER,
				param == GL_LINEAR_MIPMAP_LINEAR ? D3DTEXF_LINEAR : D3DTEXF_NONE );
			break;
	}
}

void glTexParameterf ( GLenum target, GLenum pname, GLfloat param )
{
    GL_TRACE_IMP(" glTexParameterf(%s, %s, %s)", glEnumToString(target), glEnumToString(pname), glEnumToString((GLenum) param));

    glTexParameteri( target, pname, param );
}


static D3DBLEND openGLBlendFactorToDirect3D( GLenum factor )
{
	switch (factor) {

		case GL_ZERO:					return D3DBLEND_ZERO;
		case GL_ONE:					return D3DBLEND_ONE;
		case GL_SRC_COLOR:				return D3DBLEND_SRCCOLOR;
		case GL_ONE_MINUS_SRC_COLOR:	return D3DBLEND_INVSRCCOLOR;
		case GL_SRC_ALPHA:				return D3DBLEND_SRCALPHA;
		case GL_ONE_MINUS_SRC_ALPHA:	return D3DBLEND_INVSRCALPHA;

		default:
			// Unknown blending factor
			DarwiniaDebugAssert(FALSE);
			return D3DBLEND_ONE;
	}
}

void glBlendFunc ( GLenum sfactor, GLenum dfactor )
{
	GL_TRACE_IMP(" glBlendFunc(%s, %s)", glEnumToString(sfactor), glEnumToString(dfactor))
	g_pd3dDevice->SetRenderState(D3DRS_SRCBLEND, openGLBlendFactorToDirect3D(sfactor));
	g_pd3dDevice->SetRenderState(D3DRS_DESTBLEND, openGLBlendFactorToDirect3D(dfactor));
}

void glDepthMask ( GLboolean flag )
{
	GL_TRACE_IMP(" glDepthMask(%s)", glBooleanToString(flag))
	g_pd3dDevice->SetRenderState( D3DRS_ZWRITEENABLE, flag );
}

void glFogf_impl (GLenum pname, GLfloat param)
{
	switch (pname) {
		case GL_FOG_DENSITY:
			g_pd3dDevice->SetRenderState( D3DRS_FOGDENSITY, *((DWORD*) (&param)) );
			break;

		case GL_FOG_END:
			g_pd3dDevice->SetRenderState( D3DRS_FOGEND, *((DWORD*) (&param)) );
			break;

		case GL_FOG_MODE:
			glFogi( GL_FOG_MODE, *((GLint *) (&param)) );
			break;

		case GL_FOG_START:
			g_pd3dDevice->SetRenderState( D3DRS_FOGSTART, *((DWORD*) (&param)) );
			break;

		default:
			// Invalid fog parameter (colour must be specified using the 'v' variant).
			DarwiniaDebugAssert(FALSE);
			break;
	}
}

static D3DFOGMODE openGLFogModeToDirect3D( GLenum param )
{
	switch (param) {

		case GL_LINEAR: return D3DFOG_LINEAR;
		//case GL_EXP: return D3DFOG_EXP;
		//case GL_EXP2: return D3DFOG_EXP2;
		default:
			// Invalid fog mode
			DarwiniaDebugAssert(FALSE);
			return D3DFOG_NONE;
	};
}

void glFogi_impl (GLenum pname, GLint param)
{
	switch (pname) {
		case GL_FOG_DENSITY:
		case GL_FOG_END:
		case GL_FOG_START:
			if (param > 0)
				glFogf_impl( pname, (float) param / INT_MAX );
			else
				glFogf_impl( pname, (float) param / INT_MIN );
			break;

		case GL_FOG_MODE:
			// glHint( GL_FOG, GL_DONT_CARE ) should result in D3DRS_FOGVERTEXMODE being used
			// instead, if pixel based fog is inefficient

			// g_pd3dDevice->SetRenderState( D3DRS_FOGVERTEXMODE, openGLFogModeToDirect3D((GLenum) param) );
			g_pd3dDevice->SetRenderState( D3DRS_FOGTABLEMODE, openGLFogModeToDirect3D((GLenum) param) );
			break;
	}
}

void glFogfv (GLenum pname, const GLfloat *params)
{
	GL_TRACE_IMP(" glFogfv(%s, (const float *)%p)", glEnumToString(pname), params)

	switch (pname) {
		case GL_FOG_COLOR:
			g_pd3dDevice->SetRenderState( D3DRS_FOGCOLOR, D3DCOLOR_COLORVALUE(params[0], params[1], params[2], params[3]) );
			break;

		default:
			// Not implemented
			DarwiniaDebugAssert(FALSE);
			break;
	}
}

void glViewport (GLint x, GLint y, GLsizei width, GLsizei height)
{
	GL_TRACE_IMP(" glViewport(%d, %d, %d, %d)", x, y, width, height)
	D3DVIEWPORT9 viewport;

	g_pd3dDevice->GetViewport( &viewport );
	viewport.X = x;
	viewport.Y = y;
	viewport.Width = width;
	viewport.Height = height;

	g_pd3dDevice->SetViewport( &viewport );
}

static GLenum direct3DFillModeToOpenGL( D3DFILLMODE mode )
{
	switch (mode) {
		case D3DFILL_SOLID:
			return GL_FILL;

		case D3DFILL_WIREFRAME:
			return GL_LINE;

		default:
			DarwiniaDebugAssert( mode == D3DFILL_SOLID || mode == D3DFILL_WIREFRAME );
			return GL_FILL;
	}
}

static GLenum direct3DShadeModelToOpenGL( D3DSHADEMODE mode )
{
	switch (mode) {
		case D3DSHADE_FLAT:
			return GL_FLAT;

		case D3DSHADE_PHONG:
		case D3DSHADE_GOURAUD:
			return GL_SMOOTH;

		default:
			DarwiniaDebugAssert( FALSE );
			return GL_SMOOTH;
	}
}


static GLenum direct3DFuncToOpenGL( D3DCMPFUNC func )
{
	switch (func) {
		case D3DCMP_LESSEQUAL: return GL_LEQUAL;
		case D3DCMP_GREATER: return GL_GREATER;
		default:
			DarwiniaDebugAssert(FALSE);
			return GL_LEQUAL;
	}
}

static GLenum direct3DBlendFactorToOpenGL( D3DBLEND factor )
{
	switch (factor) {

		case D3DBLEND_ZERO:				return GL_ZERO;
		case D3DBLEND_ONE:				return GL_ONE;
		case D3DBLEND_SRCCOLOR:			return GL_SRC_COLOR;
		case D3DBLEND_INVSRCCOLOR:		return GL_ONE_MINUS_SRC_COLOR;
		case D3DBLEND_SRCALPHA:			return GL_SRC_ALPHA;
		case D3DBLEND_INVSRCALPHA:		return GL_ONE_MINUS_SRC_ALPHA;

		default:
			// Unknown blending factor
			DarwiniaDebugAssert(FALSE);
			return GL_ONE;
	}
}

static GLenum direct3DFogModeToOpenGL( D3DFOGMODE param )
{
	switch (param) {

		case D3DFOG_LINEAR: return GL_LINEAR;
		//case GL_EXP: return D3DFOG_EXP;
		//case GL_EXP2: return D3DFOG_EXP2;
		default:
			// Invalid fog mode
			DarwiniaDebugAssert(FALSE);
			return GL_LINEAR;
	};
}

void glGetIntegerv (GLenum pname, GLint *params)
{
	GL_TRACE_IMP(" glGetIntegerv(%s, (int *)%p)", glEnumToString(pname), params);

	switch (pname) {
		case GL_ALPHA_TEST_FUNC:
			{
				D3DCMPFUNC func;
				g_pd3dDevice->GetRenderState(D3DRS_ALPHAFUNC, (DWORD *) &func);
				params[0] = direct3DFuncToOpenGL(func);
				break;
			}

		case GL_BLEND_DST:
			{
				D3DBLEND factor;
				g_pd3dDevice->GetRenderState(D3DRS_DESTBLEND, (DWORD *) &factor);
				params[0] = direct3DBlendFactorToOpenGL(factor);
				break;
			}

		case GL_BLEND_SRC:
			{
				D3DBLEND factor;
				g_pd3dDevice->GetRenderState(D3DRS_SRCBLEND, (DWORD *) &factor);
				params[0] = direct3DBlendFactorToOpenGL(factor);
				break;
			}

		case GL_DEPTH_FUNC:
			{
				D3DCMPFUNC func;
				g_pd3dDevice->GetRenderState(D3DRS_ZFUNC, (DWORD *) &func);
				params[0] = direct3DFuncToOpenGL(func);
				break;
			}

		case GL_DEPTH_WRITEMASK:
			{
				DWORD state;
				g_pd3dDevice->GetRenderState( D3DRS_ZWRITEENABLE, &state );
				params[0] = state == TRUE;
				break;
			}

		case GL_COLOR_MATERIAL_FACE:
			// Direct3D doesn't seem to support independent front and back materials
			params[0] = GL_FRONT_AND_BACK;
			break;

		case GL_COLOR_MATERIAL_PARAMETER:
			params[0] = s_currentAttribs.colorMaterialMode;
			break;

		case GL_FOG_HINT:
			params[0] = s_fogHint;
			break;

		case GL_FOG_MODE:
			{
				D3DFOGMODE mode;
				g_pd3dDevice->GetRenderState( D3DRS_FOGTABLEMODE, (DWORD *) &mode );
				params[0] = direct3DFogModeToOpenGL( mode );
				break;
			}

		case GL_FRONT_FACE:
			params[0] = s_ccwFrontFace ? GL_CCW : GL_CW;
			break;

		case GL_MATRIX_MODE:
			params[0] = s_matrixMode;
			break;

		case GL_POLYGON_MODE:
			{
				D3DFILLMODE mode;
				g_pd3dDevice->GetRenderState( D3DRS_FILLMODE, (DWORD *) &mode );
				params[0] = params[1] = direct3DFillModeToOpenGL( mode );
				break;
			}

		case GL_POLYGON_SMOOTH_HINT:
			params[0] = s_polygonSmoothHint;
			break;

		case GL_SHADE_MODEL:
			{
				D3DSHADEMODE mode;
				g_pd3dDevice->GetRenderState( D3DRS_SHADEMODE, (DWORD *) &mode );
				params[0] = direct3DShadeModelToOpenGL( mode );
				break;
			}
		case GL_VIEWPORT:
			{
				D3DVIEWPORT9 viewport;
				g_pd3dDevice->GetViewport( &viewport );

				params[0] = viewport.X;
				params[1] = viewport.Y;
				params[2] = viewport.Width;
				params[3] = viewport.Height;
			}
			break;

		default:
			GL_TRACE("-glGetIntegerv(%s, (int *)%p) not implemented", glEnumToString(pname), params)
	}
}

static void direct3DColourToFloatv( D3DCOLOR colour, GLfloat *params )
{
	// ((D3DCOLOR)((((a)&0xff)<<24)|(((r)&0xff)<<16)|(((g)&0xff)<<8)|((b)&0xff)))
	params[1] = ((colour >> 16) & 0xff) / 255.0f;
	params[2] = ((colour >> 8)  & 0xff) / 255.0f;
	params[3] = ((colour)       & 0xff) / 255.0f;
	params[4] = ((colour >> 24) & 0xff) / 255.0f;
}

void glGetFloatv (GLenum pname, GLfloat *params)
{
	GL_TRACE_IMP(" glGetFloatv(%s, (float *)%p)", glEnumToString(pname), params);

	switch (pname) {
		case GL_ALPHA_TEST_REF:
			params[0] = s_currentAttribs.alphaRef;
			break;

		case GL_CURRENT_COLOR:
			params[0] = s_currentAttribs.r8*0.003921568627450980392156862745098f;
			params[1] = s_currentAttribs.g8*0.003921568627450980392156862745098f;
			params[2] = s_currentAttribs.b8*0.003921568627450980392156862745098f;
			params[3] = s_currentAttribs.a8*0.003921568627450980392156862745098f;
			break;

		case GL_FOG_COLOR:
			{
				D3DCOLOR colour;
				g_pd3dDevice->GetRenderState( D3DRS_FOGCOLOR, &colour );
				direct3DColourToFloatv( colour, params );
			}
			break;

		case GL_FOG_DENSITY:
			g_pd3dDevice->GetRenderState( D3DRS_FOGDENSITY, (DWORD *) params );
			break;

		case GL_FOG_END:
			g_pd3dDevice->GetRenderState( D3DRS_FOGEND, (DWORD *) params );
			break;

		case GL_LIGHT_MODEL_AMBIENT:
			{
				D3DCOLOR colour;
				g_pd3dDevice->GetRenderState( D3DRS_AMBIENT, (DWORD *) &colour );
				direct3DColourToFloatv( colour, params );
			}
			break;

		default:
			GL_TRACE("-glGetFloatv(%s, (float *)%p) not implemented", glEnumToString(pname), params);
			DarwiniaDebugAssert( false );
			break;
	}
}


template <class DoubleFloat>
static void direct3DMatrixToOpenGL( const D3DMATRIX &_matrix, DoubleFloat *_out )
{
	// OpenGL matrices are in column-major order, whereas Direct3D matrices
	// are in row-major order.
	//
	// However, Direct3D Matrices are transposed already, because Direct3D does pre-multiplying
	// and OpenGL does post-multiplying. Therefore, we do not need to transpose them
	// again.

	// We should provide a specialisation of this function which performs a
	// memcpy for Float.
	_out[0] = _matrix._11;
	_out[1] = _matrix._12;
	_out[2] = _matrix._13;
	_out[3] = _matrix._14;

	_out[4] = _matrix._21;
	_out[5] = _matrix._22;
	_out[6] = _matrix._23;
	_out[7] = _matrix._24;

	_out[ 8] = _matrix._31;
	_out[ 9] = _matrix._32;
	_out[10] = _matrix._33;
	_out[11] = _matrix._34;

	_out[12] = _matrix._41;
	_out[13] = _matrix._42;
	_out[14] = _matrix._43;
	_out[15] = _matrix._44;
}

template <class DoubleFloat>
static void openGLMatrixToDirect3D( const DoubleFloat *_in, D3DMATRIX &_out )
{
	// See comment in direct3DMatrixToOpenGL

	_out._11 = _in[0];
	_out._12 = _in[1];
	_out._13 = _in[2];
	_out._14 = _in[3];

	_out._21 = _in[4];
	_out._22 = _in[5];
	_out._23 = _in[6];
	_out._24 = _in[7];

	_out._31 = _in[8];
	_out._32 = _in[9];
	_out._33 = _in[10];
	_out._34 = _in[11];

	_out._41 = _in[12];
	_out._42 = _in[13];
	_out._43 = _in[14];
	_out._44 = _in[15];
}

void glGetDoublev (GLenum pname, GLdouble *params)
{
	GL_TRACE_IMP(" glGetDoublev(%s, (double *)%p)", glEnumToString(pname), params)

	switch (pname) {
		case GL_MODELVIEW_MATRIX:
			{
				D3DXMATRIX matrix = s_pModelViewMatrixStack->GetTransform();
				direct3DMatrixToOpenGL( matrix, params );
			}
			break;

		case GL_PROJECTION_MATRIX:
			{
				D3DXMATRIX matrix = s_pProjectionMatrixStack->GetTransform();
				direct3DMatrixToOpenGL( matrix, params );
			}
			break;

		default:
			GL_TRACE("-glGetDoublev(%s, (double *)%p) not implemented", glEnumToString(pname), params)
			break;
	}
}

int gluProject (GLdouble objx, GLdouble objy, GLdouble objz, const GLdouble modelMatrix[16], const GLdouble projMatrix[16], const GLint viewport[4], GLdouble *winx, GLdouble *winy, GLdouble *winz)
{
	D3DXMATRIX model, proj;

	openGLMatrixToDirect3D( modelMatrix, model );
	openGLMatrixToDirect3D( projMatrix, proj );

	D3DXVECTOR4 obj( objx, objy, objz, 1.0f );
	D3DXVECTOR4 win0, win;
	D3DXVec4Transform( &win0, &obj, &model );
	D3DXVec4Transform( &win, &win0, &proj );

	if (win.w == 0.0f)
		return false;

	win /= win.w;

	// Map from [-1, 1] to [0, 1]
	win.x = win.x * 0.5f + 0.5f;
	win.y = win.y * 0.5f + 0.5f;
	win.z = win.z * 0.5f + 0.5f;

    // Map to viewport
	*winx = win.x * viewport[2] + viewport[0];
	*winy = win.y * viewport[3] + viewport[1];
	*winz = win.z;

	GL_TRACE_IMP(" gluProject(%10.4g, %10.4g, %10.4g, %s, %s, %s, => %10.4g, %10.4g, %10.4g)", objx, objy, objz, glArrayToString(modelMatrix, 16), glArrayToString(projMatrix, 16), glArrayToString(viewport, 4), *winx, *winy, *winz)
	return true;
}

int gluUnProject (GLdouble winx, GLdouble winy, GLdouble winz, const GLdouble modelMatrix[16], const GLdouble projMatrix[16], const GLint viewport[4], GLdouble *objx, GLdouble *objy, GLdouble *objz)
{
	// Attempt to determine a 3d coordinate that a given window coordinate (winx, winy)
	// corresponds to, given the winz, and model and projection matrices
	// (as well as the viewport dimensions).

	D3DXMATRIX model, proj;

	openGLMatrixToDirect3D( modelMatrix, model );
	openGLMatrixToDirect3D( projMatrix, proj );

	D3DXMATRIX modelProj = model * proj;

	D3DXMATRIX inv;
	if (D3DXMatrixInverse( &inv, NULL, &modelProj ) == NULL)
		return false;

	D3DXVECTOR4 win(
		(winx - viewport[0] /* left */) / viewport[2] /* width */,
		(winy - viewport[1] /* top */) / viewport[3] /* height */,
		winz,
		1.0f);

	// Map from [0, 1] to [-1, 1]
	win.x = win.x * 2 - 1;
	win.y = win.y * 2 - 1;
	win.z = win.z * 2 - 1;

	D3DXVECTOR4 source;
	D3DXVec4Transform( &source, &win, &inv );
	if (source.w == 0.0f)
		return false;

	*objx = source.x / source.w;
	*objy = source.y / source.w;
	*objz = source.z / source.w;

	GL_TRACE_IMP(" gluUnProject(%10.4g, %10.4g, %10.4g, %s, %s, %s, => %10.4g, %10.4g, %10.4g,", winx, winy, winz, glArrayToString(modelMatrix, 16), glArrayToString(projMatrix, 16), glArrayToString(viewport, 4), *objx, *objy, *objz)

	return true;
}

void glLoadMatrixd (const GLdouble *_m)
{
	GL_TRACE_IMP(" glLoadMatrixd((const double *)%p)", m)

	D3DMATRIX m;
	openGLMatrixToDirect3D( _m, m );
	s_pTargetMatrixStack->Load(m);
}

static void transformByModelView( D3DVECTOR &v, float w /* 0.0f for directional, 1.0f for positional */ )
{
	D3DXVECTOR4 in(v.x, v.y, v.z, w);
	D3DXVECTOR4 out;

	const D3DXMATRIX* tmp = &s_pModelViewMatrixStack->GetTransform();
	D3DXVec4Transform(&out, &in, tmp );

	v.x = out.x;
	v.y = out.y;
	v.z = out.z;
}

void glLightfv (GLenum light, GLenum pname, const GLfloat *params)
{
	GL_TRACE_IMP(" glLightfv(%s, %s, (const float *)%p)", glEnumToString(light), glEnumToString(pname), params)

	// Possible rewrite:
	//	- Change this function to modify just a D3DLIGHT9 structure
	//	- and make the state changes only when required.

      //20 glLightfv(GL_LIGHT0, GL_POSITION, (const float *)0017F458)
      //21 glLightfv(GL_LIGHT0, GL_DIFFUSE, (const float *)0017F484)
      //22 glLightfv(GL_LIGHT0, GL_SPECULAR, (const float *)0017F484)
      //23 glLightfv(GL_LIGHT0, GL_AMBIENT, (const float *)0017F49C)

	DWORD lightIndex = light - GL_LIGHT0;
	D3DLIGHT9 lightInfo;

	g_pd3dDevice->GetLight( lightIndex, &lightInfo );

	// Apply the modelview matrix to the parameters to get the
	// lights into eye-space

	switch (pname) {
		case GL_POSITION:
			if (params[3] == 0.0f) {
				lightInfo.Type = D3DLIGHT_DIRECTIONAL;
				lightInfo.Direction.x = params[0];
				lightInfo.Direction.y = params[1];
				lightInfo.Direction.z = params[2];
				transformByModelView(lightInfo.Direction, 0.0f);
				/* the same effect using distant point light
				lightInfo.Type = D3DLIGHT_POINT;
				lightInfo.Range=1000000;
				lightInfo.Attenuation0=1;
				lightInfo.Attenuation1=0;
				lightInfo.Attenuation2=0;
				lightInfo.Falloff=1;
				lightInfo.Theta=3;
				lightInfo.Phi=3;
				lightInfo.Position.x = -10000*params[0];
				lightInfo.Position.y = -10000*params[1];
				lightInfo.Position.z = -10000*params[2];
				transformByModelView(lightInfo.Position, 1.0f);*/
			}
			else {
				lightInfo.Type = D3DLIGHT_POINT;
				lightInfo.Position.x = params[0];
				lightInfo.Position.y = params[1];
				lightInfo.Position.z = params[2];

				transformByModelView(lightInfo.Direction, 1.0f);
			}
			break;

		case GL_DIFFUSE:
			lightInfo.Diffuse.r = params[0];
			lightInfo.Diffuse.g = params[1];
			lightInfo.Diffuse.b = params[2];
			lightInfo.Diffuse.a = params[3];
			break;

		case GL_SPECULAR:
			lightInfo.Specular.r = params[0];
			lightInfo.Specular.g = params[1];
			lightInfo.Specular.b = params[2];
			lightInfo.Specular.a = params[3];
			break;

		case GL_AMBIENT:
			lightInfo.Ambient.r = params[0];
			lightInfo.Ambient.g = params[1];
			lightInfo.Ambient.b = params[2];
			lightInfo.Ambient.a = params[3];
			break;

		default:
			GL_TRACE("-glLightfv(%s, %s, (const float *)%p) not implemented", glEnumToString(light), glEnumToString(pname), params);
			break;
	}

	g_pd3dDevice->SetLight( lightIndex, &lightInfo );
}

void glGetLightfv (GLenum light, GLenum pname, GLfloat *params)
{
	GL_TRACE_IMP(" glGetLightfv(%s, %s, (float *)%p)", glEnumToString(light), glEnumToString(pname), params);

	int lightIndex = light - GL_LIGHT0;
	D3DLIGHT9 lightInfo;

	g_pd3dDevice->GetLight( lightIndex, &lightInfo );

	switch (pname) {
		case GL_POSITION:
			if (lightInfo.Type == D3DLIGHT_DIRECTIONAL) {
				params[0] = lightInfo.Direction.x;
				params[1] = lightInfo.Direction.y;
				params[2] = lightInfo.Direction.z;
				params[3] = 0.0f;
			}
			else {
				params[0] = lightInfo.Position.x;
				params[1] = lightInfo.Position.y;
				params[2] = lightInfo.Position.z;
				params[3] = 1.0f;
			}
			break;

		case GL_DIFFUSE:
			params[0] = lightInfo.Diffuse.r;
			params[1] = lightInfo.Diffuse.g;
			params[2] = lightInfo.Diffuse.b;
			params[3] = lightInfo.Diffuse.a;
			break;

		case GL_SPECULAR:
			params[0] = lightInfo.Specular.r;
			params[1] = lightInfo.Specular.g;
			params[2] = lightInfo.Specular.b;
			params[3] = lightInfo.Specular.a;
			break;

		case GL_AMBIENT:
			params[0] = lightInfo.Ambient.r;
			params[1] = lightInfo.Ambient.g;
			params[2] = lightInfo.Ambient.b;
			params[3] = lightInfo.Ambient.a;
			break;

		default:
			DarwiniaDebugAssert( false );
			break;
	}
}


void glLightModelfv (GLenum pname, const GLfloat *params)
{
	GL_TRACE_IMP(" glLightModelfv(%s, (const float *)%p)", glEnumToString(pname), params);

	switch (pname) {
		case GL_LIGHT_MODEL_AMBIENT:
			g_pd3dDevice->SetRenderState( D3DRS_AMBIENT, D3DCOLOR_COLORVALUE(params[0], params[1], params[2], params[3]) );
			break;

		default:
			// Unknown (unimplemented) parameter
			DarwiniaDebugAssert(FALSE);
	}
}

void glFrontFace (GLenum mode)
{
	GL_TRACE_IMP(" glFrontFace(%s)", glEnumToString(mode));

	bool v = mode == GL_CCW;

	if (s_ccwFrontFace != v) {
		s_ccwFrontFace = v;
		s_cullMode = (v ? D3DCULL_CW : D3DCULL_CCW);

		if (s_currentAttribs.backCullingEnabled)
			g_pd3dDevice->SetRenderState(D3DRS_CULLMODE, s_cullMode);
	}
}

static D3DFILLMODE openGLFillModeToDirect3D( GLenum mode )
{
	switch (mode) {
		case GL_FILL:
			return D3DFILL_SOLID;

		case GL_LINE:
			return D3DFILL_WIREFRAME;

		default:
			DarwiniaDebugAssert( mode == D3DFILL_SOLID || mode == D3DFILL_WIREFRAME );
			return D3DFILL_WIREFRAME;
	}
}

void glPolygonMode (GLenum face, GLenum mode)
{
	GL_TRACE_IMP(" glPolygonMode(%s, %s) .", glEnumToString(face), glEnumToString(mode));

	// Warning: This does not respect whether the face is front facing or
	//          back facing

	g_pd3dDevice->SetRenderState(D3DRS_FILLMODE, openGLFillModeToDirect3D(mode));
}

static D3DSHADEMODE openGLShadeModelToDirect3D( GLenum mode )
{
	switch (mode) {
		case GL_FLAT:
			return D3DSHADE_FLAT;

		case GL_SMOOTH:
			return D3DSHADE_GOURAUD;

		default:
			DarwiniaDebugAssert( mode == GL_FLAT || mode == GL_SMOOTH );
			return D3DSHADE_FLAT;
	}
}

void glShadeModel ( GLenum mode )
{
	GL_TRACE_IMP(" glShadeModel(%s)", glEnumToString(mode))

	g_pd3dDevice->SetRenderState(D3DRS_SHADEMODE, openGLShadeModelToDirect3D( mode ));
}

void glPushMatrix ()
{
	GL_TRACE_IMP(" glPushMatrix()");

	s_pTargetMatrixStack->Push();
}

void glPopMatrix ()
{
	GL_TRACE_IMP(" glPopMatrix()");

	s_pTargetMatrixStack->Pop();
}

void glMultMatrixf (const GLfloat *m)
{
	GL_TRACE_IMP(" glMultMatrixf((const float *)%p)", m)

	D3DXMATRIX mat;
	openGLMatrixToDirect3D(m, mat);
	s_pTargetMatrixStack->Multiply( mat );
}

// Used by the radar dish for the clip planes (horrid hack, but what can you do!)
void SwapToViewMatrix()
{
	D3DXMATRIX identity;
	g_pd3dDevice->SetTransform( D3DTS_WORLD, D3DXMatrixIdentity( &identity ) );
	s_pTargetMatrixStack->m_transformType = D3DTS_VIEW;
}

void SwapToModelMatrix()
{
	D3DXMATRIX identity;
	g_pd3dDevice->SetTransform( D3DTS_VIEW, D3DXMatrixIdentity( &identity ) );
	s_pTargetMatrixStack->m_transformType = D3DTS_WORLD;
}


void glTranslatef (GLfloat x, GLfloat y, GLfloat z)
{
	GL_TRACE_IMP(" glTranslatef(%10.4g, %10.4g, %10.4g)", (double) x, (double) y, (double) z)

	D3DXMATRIX mat;
	D3DXMatrixTranslation( &mat, x, y, z );
	s_pTargetMatrixStack->Multiply( mat );
}

void glScalef (GLfloat x, GLfloat y, GLfloat z)
{
	GL_TRACE_IMP(" glScalef(%10.4g, %10.4g, %10.4g)", (double) x, (double) y, (double) z)

	D3DXMATRIX mat;
	D3DXMatrixScaling( &mat, x, y, z );
	s_pTargetMatrixStack->Multiply( mat );
}

void glRotatef (GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
	GL_TRACE_IMP(" glRotatef(%10.4g, %10.4g, %10.4g, %10.4g)", (double) angle, (double) x, (double) y, (double) z)

	D3DXMATRIX mat;
	D3DXVECTOR3 axis(x, y, z);
	D3DXMatrixRotationAxis( &mat, &axis, D3DXToRadian(angle) );
	s_pTargetMatrixStack->Multiply( mat );
}


static D3DCMPFUNC openGLFuncToDirect3D( GLenum func )
{
	switch (func) {
		//case GL_NEVER: return D3DCMP_NEVER;
		//case GL_LESS: return D3DCMP_LESS;
		//case GL_EQUAL: return D3DCMP_EQUAL;
		case GL_LEQUAL: return D3DCMP_LESSEQUAL;
		case GL_GREATER: return D3DCMP_GREATER;
		//case GL_NOTEQUAL: return D3DCMP_NOTEQUAL;
		//case GL_GEQUAL: return D3DCMP_GREATEREQUAL;
		//case GL_ALWAYS: return D3DCMP_ALWAYS;
		default:
			DarwiniaDebugAssert(FALSE);
			return D3DCMP_LESSEQUAL;
	}
}

void glAlphaFunc (GLenum func, GLclampf ref)
{
	GL_TRACE_IMP(" glAlphaFunc(%s, %10.4g)", glEnumToString(func), (double) ref)

	g_pd3dDevice->SetRenderState(D3DRS_ALPHAFUNC, openGLFuncToDirect3D( func ));
	float cref = s_currentAttribs.alphaRef = (ref<0)?0:((ref>1)?1:ref);
	g_pd3dDevice->SetRenderState(D3DRS_ALPHAREF, DWORD ( cref * 255.0f) );
}


void glDepthFunc (GLenum func)
{
	GL_TRACE_IMP(" glDepthFunc(%s)", glEnumToString(func))

	g_pd3dDevice->SetRenderState(D3DRS_ZFUNC, openGLFuncToDirect3D( func ));
}



// Display Lists


GLuint glGenLists (GLsizei range)
{
	GL_TRACE_IMP(" glGenLists(%d)", range);

	int result = s_lastDisplayId;
	for (unsigned i = 0; i < range; i++)
		s_displayLists[s_lastDisplayId++] = NULL;

	return result;
}

void glDeleteLists (GLuint list, GLsizei range)
{
	GL_TRACE_IMP(" glDeleteLists(%u, %d)", list, range);

	for (unsigned i = 0; i < range; i++) {
		delete s_displayLists[list + i];
		s_displayLists.erase(list + i);
	}
}


void glNewList (GLuint list, GLenum mode)
{
	GL_TRACE_IMP(" glNewList(%u, %s)", list, glEnumToString(mode));

	s_pDisplayListDevice = new DisplayListDevice( list );
	s_pModelViewMatrixStack = new MatrixStackDisplayList( D3DTS_WORLD, s_pDisplayListDevice, s_pModelViewMatrixStackActual );
	s_pProjectionMatrixStack = new MatrixStackDisplayList( D3DTS_PROJECTION, s_pDisplayListDevice, s_pProjectionMatrixStackActual );
	s_pTargetMatrixStack =
		s_matrixMode == GL_MODELVIEW ? s_pModelViewMatrixStack : s_pProjectionMatrixStack;

	g_pd3dDevice = s_pDisplayListDevice;
}

void glEndList ()
{
	GL_TRACE_IMP(" glEndList()")

	DarwiniaDebugAssert( s_pDisplayListDevice != NULL );
	if (!s_pDisplayListDevice)
		return;

	s_displayLists[ s_pDisplayListDevice->GetId() ] = s_pDisplayListDevice->Compile();

	g_pd3dDevice = g_pd3dDeviceActual;

	delete s_pDisplayListDevice;
	delete s_pModelViewMatrixStack;
	delete s_pProjectionMatrixStack;

	s_pDisplayListDevice = NULL;
	s_pModelViewMatrixStack = s_pModelViewMatrixStackActual;
	s_pProjectionMatrixStack = s_pProjectionMatrixStackActual;
	s_pTargetMatrixStack =
		s_matrixMode == GL_MODELVIEW ? s_pModelViewMatrixStack : s_pProjectionMatrixStack;
}

void glCallList (GLuint list)
{
	GL_TRACE_IMP(" glCallList(%u)", list);

	DisplayList *dl = s_displayLists[ list ];

	if (dl) {
		s_pTargetMatrixStack->FastSetTransform();
		dl->Draw();
	}
}

// Materials
void glMaterialfv (GLenum face, GLenum pname, const GLfloat *params)
{
	GL_TRACE_IMP(" glMaterialfv(%s, %s, (const float *)%p)", glEnumToString(face), glEnumToString(pname), params);

	// This is inefficient, since the OpenGL source code will call glMaterialfv multiple times.
	// to set a series of material properties. Better to do the SetMaterial just before a primitive
	// rendering if necessary.

	D3DMATERIAL9 mat;
	g_pd3dDevice->GetMaterial(&mat);

	switch (pname) {
		case GL_SPECULAR:
			mat.Specular.r = params[0];
			mat.Specular.g = params[1];
			mat.Specular.b = params[2];
			mat.Specular.a = params[3];
			break;

		case GL_DIFFUSE:
			mat.Diffuse.r = params[0];
			mat.Diffuse.g = params[1];
			mat.Diffuse.b = params[2];
			mat.Diffuse.a = params[3];
			break;

		case GL_AMBIENT:
			mat.Ambient.r = params[0];
			mat.Ambient.g = params[1];
			mat.Ambient.b = params[2];
			mat.Ambient.a = params[3];
			break;

		case GL_AMBIENT_AND_DIFFUSE:
			mat.Ambient.r = params[0];
			mat.Ambient.g = params[1];
			mat.Ambient.b = params[2];
			mat.Ambient.a = params[3];

			mat.Diffuse.r = params[0];
			mat.Diffuse.g = params[1];
			mat.Diffuse.b = params[2];
			mat.Diffuse.a = params[3];
			break;

		//case GL_EMISSION:
		//	mat.Emissive.r = params[0];
		//	mat.Emissive.g = params[1];
		//	mat.Emissive.b = params[2];
		//	mat.Emissive.a = params[3];
		//	break;

		case GL_SHININESS:
			mat.Power = params[0];
			break;

		default:
			// Unknown material property
			DarwiniaDebugAssert(FALSE);
			break;
	}

	g_pd3dDevice->SetMaterial(&mat);
}

void glColorMaterial (GLenum face, GLenum mode)
{
	GL_TRACE_IMP(" glColorMaterial(%s, %s)", glEnumToString(face), glEnumToString(mode))

	// Warning: This ignores the face setting
	// glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE)

	s_currentAttribs.colorMaterialMode = mode;
	if (s_currentAttribs.colorMaterialEnabled)
		setupColorMaterial();
}

GLboolean glIsEnabled ( GLenum cap )
{
	GL_TRACE_IMP(" glIsEnabled(%s)", glEnumToString(cap));

	DWORD state;

	switch (cap) {
		case GL_ALPHA_TEST:
			g_pd3dDevice->GetRenderState(D3DRS_ALPHATESTENABLE, &state);
			return state == TRUE;

		case GL_BLEND:
			g_pd3dDevice->GetRenderState(D3DRS_ALPHABLENDENABLE, &state);
			return state == TRUE;

		case GL_COLOR_MATERIAL:
			return s_currentAttribs.colorMaterialEnabled;

		case GL_CULL_FACE:
			g_pd3dDevice->GetRenderState(D3DRS_CULLMODE, &state);
			return state != D3DCULL_NONE;

		case GL_DEPTH_TEST:
			g_pd3dDevice->GetRenderState(D3DRS_ZENABLE, &state);
			return state == D3DZB_TRUE;

		case GL_FOG:
			g_pd3dDevice->GetRenderState(D3DRS_FOGENABLE, &state);
			return state == TRUE;

		case GL_LIGHT0:
		case GL_LIGHT1:
		case GL_LIGHT2:
		case GL_LIGHT3:
		case GL_LIGHT4:
		case GL_LIGHT5:
		case GL_LIGHT6:
		case GL_LIGHT7:
			{
				BOOL enabled;
				g_pd3dDevice->GetLightEnable( cap - GL_LIGHT0, &enabled );
				return enabled;
			}

		case GL_LIGHTING:
			g_pd3dDevice->GetRenderState(D3DRS_LIGHTING, &state);
			return state == TRUE;

		case GL_LINE_SMOOTH:
			g_pd3dDevice->GetRenderState( D3DRS_ANTIALIASEDLINEENABLE, &state );
			return state == TRUE;

		case GL_NORMALIZE:
			g_pd3dDevice->GetRenderState(D3DRS_NORMALIZENORMALS, &state);
			return state == TRUE;

		case GL_POINT_SMOOTH:
			return FALSE;

		case GL_SCISSOR_TEST:
			g_pd3dDevice->GetRenderState( D3DRS_SCISSORTESTENABLE, &state );
			return state == TRUE;

		case GL_TEXTURE_2D:
			return s_currentAttribs.texturingEnabled[s_activeTexture];

		default:
			DarwiniaDebugAssert(FALSE);
			break;
	}
	return 0;
}

void glPointSize (GLfloat size)
{
	GL_TRACE_IMP(" glPointSize(%10.4g)", (double) size);

	g_pd3dDevice->SetRenderState(D3DRS_POINTSIZE, * (DWORD *) &size );
}

void glGetTexParameteriv (GLenum target, GLenum pname, GLint *params)
{
	GL_TRACE_IMP(" glGetTexParameteriv(%s, %s, (int *)%p)", glEnumToString(target), glEnumToString(pname), params);

	DarwiniaDebugAssert(target == GL_TEXTURE_2D);

	switch (pname) {
		case GL_TEXTURE_MAG_FILTER:
			{
				DWORD magValue;

				g_pd3dDevice->GetSamplerState( s_activeTexture, D3DSAMP_MAGFILTER, &magValue );
				params[0] = direct3DMinMagFilterToOpenGL(magValue);
			}
			break;

		case GL_TEXTURE_MIN_FILTER:
			{
				DWORD minValue, mipValue;

				g_pd3dDevice->GetSamplerState( s_activeTexture, D3DSAMP_MINFILTER, &minValue );
				g_pd3dDevice->GetSamplerState( s_activeTexture, D3DSAMP_MIPFILTER, &mipValue );

				if (mipValue == D3DTEXF_LINEAR)
					params[0] = GL_LINEAR_MIPMAP_LINEAR;
				else
					params[0] = direct3DMinMagFilterToOpenGL(minValue);
			}
			break;

		case GL_TEXTURE_WRAP_S:
		case GL_TEXTURE_WRAP_T:
			{
				DWORD wrapState;
				g_pd3dDevice->GetRenderState( D3DRS_WRAP0, &wrapState );

				DWORD wrapMask = (pname == GL_TEXTURE_WRAP_S ? D3DWRAP_U : D3DWRAP_V);

				params[0] = (wrapState & wrapMask ? GL_REPEAT : GL_CLAMP );
				break;
			}

		default:
			DarwiniaDebugAssert( FALSE );
	}
}


// Texture Environment Mapping

void glTexEnvf (GLenum target, GLenum pname, GLfloat param)
{
	GL_TRACE_IMP(" glTexEnvf(%s, %s, %s)", glEnumToString(target), glEnumToString(pname), glEnumToString(param));

	switch (target) {
		case GL_TEXTURE_ENV:
			switch (pname) {
				case GL_TEXTURE_ENV_MODE:
					s_activeTextureState->envMode = param;
					if (s_currentAttribs.texturingEnabled[s_activeTexture])
						setupTexturing();
					return;

				case GL_COMBINE_RGB_EXT:
					s_activeTextureState->rgbCombineMode = param;
					if (s_currentAttribs.texturingEnabled[s_activeTexture])
						setupTexturing();
					return;
			}
			break;

	}

	GL_TRACE("-glTexEnvf(%s, %s, %10.4g) not implemented", glEnumToString(target), glEnumToString(pname), (double) param);
	DarwiniaDebugAssert(false);
}

void glTexEnvi (GLenum target, GLenum pname, GLint param)
{
	GL_TRACE_IMP(" glTexEnvi(%s, %s, %s)", glEnumToString(target), glEnumToString(pname), glEnumToString(param));
	switch (target) {
		case GL_TEXTURE_ENV:
			switch (pname) {
				case GL_TEXTURE_ENV_MODE:
					s_activeTextureState->envMode = param;
					if (s_currentAttribs.texturingEnabled[s_activeTexture])
						setupTexturing();
					return;
			}
			break;
	}

	GL_TRACE("glTexEnvi(%s, %s, %d)", glEnumToString(target), glEnumToString(pname), param);
	DarwiniaDebugAssert(false);
}

void glTexEnviv (GLenum target, GLenum pname, const GLint *params)
{
	GL_TRACE_IMP(" glTexEnviv(%s, %s, (const int *)%p)", glEnumToString(target), glEnumToString(pname), params);

	switch (target) {
		case GL_TEXTURE_ENV:
			switch (pname) {
				case GL_TEXTURE_ENV_COLOR:
					{
						TextureState &s = *s_activeTextureState;

						s.envColour.r = params[0] / 255.0f;
						s.envColour.g = params[1] / 255.0f;
						s.envColour.b = params[2] / 255.0f;
						s.envColour.a = params[3] / 255.0f;
						return;
					}
			}
	}

	GL_TRACE("-glTexEnviv(%s, %s, (const int *)%p) not implemented", glEnumToString(target), glEnumToString(pname), params);
	DarwiniaDebugAssert(false);
}

void glGetTexEnviv (GLenum target, GLenum pname, GLint *params)
{
	GL_TRACE_IMP(" glGetTexEnviv(%s, %s, (int *)%p)", glEnumToString(target), glEnumToString(pname), params);

	TextureState &s = *s_activeTextureState;

	switch (target) {
		case GL_TEXTURE_ENV:
			switch (pname) {
				case GL_TEXTURE_ENV_COLOR:
					params[0] = s.envColour.r * 255.0f;
					params[1] = s.envColour.g * 255.0f;
					params[2] = s.envColour.b * 255.0f;
					params[3] = s.envColour.a * 255.0f;
					return;

				case GL_TEXTURE_ENV_MODE:
					params[0] = s.envMode;
					return;
			}
	}

	GL_TRACE("-glGetTexEnviv(%s, %s, (int *)%p) not implemented", glEnumToString(target), glEnumToString(pname), params);
}

void glHint (GLenum target, GLenum mode)
{
	GL_TRACE_IMP(" glHint(%s, %s)", glEnumToString(target), glEnumToString(mode));

	switch (target) {
		case GL_FOG_HINT:
			s_fogHint = mode;
			break;

		case GL_POLYGON_SMOOTH_HINT:
			s_polygonSmoothHint = mode;
			break;
	}
}

GLenum glGetError ()
{
	GL_TRACE_IMP(" glGetError()")
	return 0;
}

const GLubyte* gluErrorString (GLenum errCode)
{
	GL_TRACE_IMP(" gluErrorString(%s)", glEnumToString(errCode))
	return (const GLubyte *) "[Error Reporting Not Yet Implemented]";
}

void glLineWidth ( GLfloat width )
{
	GL_TRACE_IMP(" glLineWidth(%10.4g)", (double) width);

	// Direct3D doesn't support line width directly
	// We have to emulate with quads :(
}


void glClipPlane (GLenum plane, const GLdouble *equation)
{
    GL_TRACE_IMP(" glClipPlane(%s, (const double *)%p)", glEnumToString(plane), equation);

    int planeIndex = plane - GL_CLIP_PLANE0;

    float equationAsFloats[4] = { equation[0], equation[1], equation[2], equation[3] };

    g_pd3dDevice->SetClipPlane(planeIndex, equationAsFloats );
}


void glFinish ()
{
    GL_TRACE_IMP(" glFinish()");

    return;

    // Lock the back buffer to force Direct3D to flush all pending graphics operations

    IDirect3DSurface9 *backbuffer = NULL;
    HRESULT hr = g_pd3dDeviceActual->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &backbuffer );
    DarwiniaDebugAssert( hr == D3D_OK );

    D3DLOCKED_RECT lockedRect;
    hr = backbuffer->LockRect(&lockedRect, NULL, 0);
    DarwiniaDebugAssert( hr != D3DERR_INVALIDCALL );
    DarwiniaDebugAssert( hr != D3DERR_WASSTILLDRAWING );
    DarwiniaDebugAssert( hr == D3D_OK );

    hr = backbuffer->UnlockRect();
    DarwiniaDebugAssert( hr == D3D_OK );

    backbuffer->Release();
}



int Shader::SetSampler(char* name, int tex)
{
	return SetSampler(name,s_textureIds[tex]);
}


#endif // USE_DIRECT3D
