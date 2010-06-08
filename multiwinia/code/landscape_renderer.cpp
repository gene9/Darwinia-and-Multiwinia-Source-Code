#include "lib/universal_include.h"

#include <float.h>

#include "lib/2d_surface_map.h"
#include "lib/filesys/binary_stream_readers.h"
#include "lib/bitmap.h"
#include "lib/math_utils.h"
#include "lib/ogl_extensions.h"
#include "lib/preferences.h"
#include "lib/profiler.h"
#include "lib/resource.h"
#include "lib/rgb_colour.h"
#include "lib/texture_uv.h"
#include "lib/vector3.h"
#include "lib/FixedPipeline.h"
#include "lib/math/random_number.h"
#include "loading_screen.h"

#include "app.h"
#include "camera.h"
#include "landscape_renderer.h"
#include "location.h"	// For SetupFog
#include "renderer.h"
#include "level_file.h"


#define MAIN_DISPLAY_LIST_NAME "LandscapeMain"
#define OVERLAY_DISPLAY_LIST_NAME "LandscapeOverlay"

#ifdef USE_DIRECT3D
	#define USE_DIRECT3D_TRILIST // Convert from tristrip to trilist (because of flatshading)
#endif

//*****************************************************************************
// Protected Functions
//*****************************************************************************

void LandscapeRenderer::BuildVertArrayAndTriStrip(SurfaceMap2D <double> *_heightMap)
{
	m_highest = _heightMap->GetHighestValue();
    if( g_app->m_location->m_landscape.m_artificalMaxHeight > 0.0f )
    {
        m_highest = g_app->m_location->m_landscape.m_artificalMaxHeight;
    }

	int const rows = _heightMap->GetNumColumns();
	int const cols = _heightMap->GetNumRows();

	m_verts.SetStepDouble();
	LandTriangleStrip *strip = new LandTriangleStrip();
	strip->m_firstVertIndex = 0;
	int degen = 0;
	LandVertex vertex1, vertex2;

	for (int z = 0; z < cols; ++z)
	{
		float fz = (float)z * _heightMap->m_cellSizeY;

		for (int x = 0; x < rows; ++x)
		{
			float heighta = _heightMap->GetData(x - 1, z    );
			float heightb = _heightMap->GetData(x - 1, z + 1);
			float height1 = _heightMap->GetData(x    , z    );
			float height2 = _heightMap->GetData(x    , z + 1);
			float height3 = _heightMap->GetData(x + 1, z    );
			float height4 = _heightMap->GetData(x + 1, z + 1);
			// Is this quad at least partly above water?
			if (heighta > 0.0 || heightb > 0.0 ||
				height1 > 0.0 || height2 > 0.0 ||
				height3 > 0.0 || height4 > 0.0)
			{
				// Yes it is, add it to the strip
				float fx = (float)x * _heightMap->m_cellSizeX;
				vertex1.m_pos.Set(fx, height1, fz);
				vertex2.m_pos.Set(fx, height2, fz + _heightMap->m_cellSizeY);
				if (vertex1.m_pos.y < 0.3) vertex1.m_pos.y = -10.0;
				if (vertex2.m_pos.y < 0.3) vertex2.m_pos.y = -10.0;
				if(degen==1)
				{
					m_verts.PutData(vertex1);
					m_verts.PutData(vertex1);
				}
				degen = 2;
				m_verts.PutData(vertex1);
				m_verts.PutData(vertex2);
			}
			else
			{
				// No, quad is entirely below water, add degenerated joint.
				if(degen==2)
				{
					m_verts.PutData(vertex2);
					m_verts.PutData(vertex2);
					degen = 1;
				}
			}
		}
	}

	// end strip
	strip->m_numVerts = m_verts.NumUsed();
	m_strips.PutData(strip);
	m_numTriangles = strip->m_numVerts - 2;
}


void LandscapeRenderer::BuildNormArray()
{
	if (m_verts.Size() <= 0)
		return;

	int nextNormId = 0;

	// Go through all the strips...
	for (int i = 0; i < m_strips.Size(); ++i)
	{
		LandTriangleStrip *strip = m_strips[i];

		m_verts[nextNormId++].m_norm = g_upVector;
		m_verts[nextNormId++].m_norm = g_upVector;
		int const maxJ = strip->m_numVerts - 2;
		
		// For each vertex in strip
		for (int j = 0; j < maxJ; j += 2)
		{
			Vector3 const &v1 = m_verts[strip->m_firstVertIndex + j].m_pos;
			Vector3 const &v2 = m_verts[strip->m_firstVertIndex + j + 1].m_pos;
			Vector3 const &v3 = m_verts[strip->m_firstVertIndex + j + 2].m_pos;
			Vector3 const &v4 = m_verts[strip->m_firstVertIndex + j + 3].m_pos;

			Vector3 north(v1 - v2);
			Vector3 northEast(v3 - v2);
			Vector3 east(v4 - v2);

			Vector3 norm1(northEast ^ north);
			norm1.Normalise();
			Vector3 norm2(east ^ northEast);
			norm2.Normalise();

			m_verts[nextNormId++].m_norm = norm1;
			m_verts[nextNormId++].m_norm = norm2;

			int vertIndex = strip->m_firstVertIndex + j + 2;
			AppDebugAssert(nextNormId - 2 == vertIndex);
		}
	}

	int vertIndex = m_verts.NumUsed();
	AppDebugAssert(nextNormId == vertIndex);
}


void LandscapeRenderer::BuildUVArray(SurfaceMap2D <double> *_heightMap)
{
	if (m_verts.Size() <= 0)
		return;

	int nextUVId = 0;
	
    float factorX = 1.0 / _heightMap->m_cellSizeX;
    float factorZ = 1.0 / _heightMap->m_cellSizeY;

    for (int i = 0; i < m_strips.Size(); ++i)
	{
		LandTriangleStrip *strip = m_strips[i];
	
		int const numVerts = strip->m_numVerts;
		for (int j = 0; j < numVerts; ++j)
		{
			Vector3 const &vert = m_verts[strip->m_firstVertIndex + j].m_pos;
			float u = vert.x * factorX;
			float v = vert.z * factorZ;
			m_verts[nextUVId++].m_uv = TextureUV(u, v);
		}
	}

	AppDebugAssert(nextUVId == m_verts.NumUsed());
}


// _gradient=1 means flat, _gradient=0 means vertical
// x and z are only passed in as a means to get predicatable noise
void LandscapeRenderer::GetLandscapeColour( float _height, float _gradient, 
											unsigned int _x, unsigned int _y, RGBAColour *_colour )
{
	float heightAboveSea = _height;
    float u = iv_pow(1.0 - _gradient, 0.4);
	float v = 1.0 - heightAboveSea / m_highest;
    AppSeedRandom(_x | _y + AppRandom());
	if (heightAboveSea < 0.0) heightAboveSea = 0.0;
	v += sfrand(0.45 / (heightAboveSea + 2.0));

    int x = u * m_landscapeColour->m_width;
    int y = v * m_landscapeColour->m_height;

    if( x < 0 ) x = 0;
    if( x > m_landscapeColour->m_width - 1) x = m_landscapeColour->m_width - 1;
    if( y < 0 ) y = 0;
    if( y > m_landscapeColour->m_height - 1) y = m_landscapeColour->m_height - 1;

    *_colour = m_landscapeColour->GetPixel(x, y);    
    
    if( g_app->m_negativeRenderer ) _colour->a = 0;
}


void LandscapeRenderer::BuildColourArray()
{
	if (m_verts.Size() <= 0)
		return;

	int nextColId = 0;

	for (int i = 0; i < m_strips.Size(); ++i)
	{
		LandTriangleStrip *strip = m_strips[i];
	
		m_verts[nextColId++].m_col.Set(255,0,255);
		m_verts[nextColId++].m_col.Set(255,0,255);

		int const numVerts = strip->m_numVerts - 2;
		for (int j = 0; j < numVerts; ++j)
		{
			Vector3 const &v1 = m_verts[strip->m_firstVertIndex + j].m_pos;
			Vector3 const &v2 = m_verts[strip->m_firstVertIndex + j + 1].m_pos;
			Vector3 const &v3 = m_verts[strip->m_firstVertIndex + j + 2].m_pos;
			Vector3 centre = (v1 + v2 + v3) * 0.33333;
			Vector3 const &norm = m_verts[strip->m_firstVertIndex + j + 2].m_norm;
			RGBAColour col;
			GetLandscapeColour(centre.y, norm.y, 
							   (unsigned int)centre.x, (unsigned int)centre.z, 
							   &col);

            for( int i = 0; i < g_app->m_location->m_burnPatches.Size(); ++i )
            {
                BurnPatch *patch = g_app->m_location->m_burnPatches.GetPointer(i);
                double dist = (centre - patch->m_pos).Mag();
                if( dist < patch->m_radius * 1.5 )
                {
                    RGBAColour burnCol(20, 20, 20);
                    double ratio = 0.0;
                    if( dist > patch->m_radius / 2.0 )
                    {
                        ratio = (dist - (patch->m_radius / 2.0)) / patch->m_radius;
                        //ratio *= 0.5;
                        ratio = min( 1.0, ratio );
                    }
                    col = (col * ratio ) + (burnCol * (1.0 -ratio));
                }
            }

			//AppDebugOut("%d[%d]: r:%02x g:%02x b:%02x a:%02x\n", i, j,  col.r, col.g, col.b, col.a);
#ifdef USE_DIRECT3D
			D3DCOLOR d3dCol = D3DCOLOR_ARGB(col.a, col.r, col.g, col.b);
			memcpy(&m_verts[nextColId++].m_col, &d3dCol, sizeof(d3dCol));
#else
			m_verts[nextColId++].m_col = col;
#endif
		}
	}

	AppDebugAssert(nextColId == m_verts.NumUsed());
}


//*****************************************************************************
// PublicFunctions
//*****************************************************************************

const unsigned LandscapeRenderer::m_posOffset(0);
const unsigned LandscapeRenderer::m_normOffset(sizeof(FloatVector3));
const unsigned LandscapeRenderer::m_colOffset(sizeof(FloatVector3) * 2);
const unsigned LandscapeRenderer::m_uvOffset(sizeof(FloatVector3) * 2 + sizeof(RGBAColour));

LandscapeRenderer::LandscapeRenderer(SurfaceMap2D <double> *_heightMap)
:	m_vertexBuffer(0)
{
    char fullFilname[256];
    sprintf( fullFilname, "terrain/%s", g_app->m_location->m_levelFile->m_landscapeColourFilename );

    if( g_app->m_location->ChristmasModEnabled() )
    {
        strcpy( fullFilname, "terrain/landscape_icecaps.bmp" );
    }

	// Read render mode from prefs file
#ifdef USE_DIRECT3D
	m_renderMode = RenderModeVertexBufferObject;
#else
	m_renderMode = g_prefsManager->GetInt("RenderLandscapeMode", -1);
	if( m_renderMode == -1 )
	{
		m_renderMode = RenderModeVertexBufferObject;
		if( !IsOGLExtensionPerfectlySupported("GL_ARB_vertex_buffer_object") )
		{
			m_renderMode = RenderModeDisplayList;
		}
	}
#endif

	// Make sure the selected mode is supported on the user's hardware
	if (m_renderMode == RenderModeVertexBufferObject)
	{
		if (!gglBindBufferARB || !IsOGLExtensionSupported("GL_ARB_vertex_buffer_object"))
		{
			// Vertex Buffer Objects not supported, so fallback to display lists
			m_renderMode = RenderModeDisplayList;
		}
	}

	/*BinaryReader *reader = g_app->m_resource->GetBinaryReader(fullFilname);
	AppReleaseAssert(reader != NULL, "Failed to get resource %s", fullFilname);
	m_landscapeColour = new BitmapRGBA(reader, "bmp");*/
    m_landscapeColour = g_app->m_resource->GetBitmap(fullFilname);
//	delete reader;

	BuildOpenGlState(_heightMap);
}


LandscapeRenderer::~LandscapeRenderer()
{
	if (m_renderMode == RenderModeDisplayList) 
	{
		g_app->m_resource->DeleteDisplayList(MAIN_DISPLAY_LIST_NAME);
		g_app->m_resource->DeleteDisplayList(OVERLAY_DISPLAY_LIST_NAME);
	}
	
	m_verts.Empty();
    m_strips.EmptyAndDelete();

#ifdef USE_DIRECT3D
	ReleaseD3DResources();
#endif
	//delete m_landscapeColour;

}

#ifdef USE_DIRECT3D
#include "lib/opengl_directx_internals.h"

static LPDIRECT3DVERTEXDECLARATION9 s_vertexDecl = NULL;

void LandscapeRenderer::ReleaseD3DPoolDefaultResources()
{
	SAFE_RELEASE(s_vertexDecl);
}

void LandscapeRenderer::ReleaseD3DResources()
{
	ReleaseD3DPoolDefaultResources();
	if(m_vertexBuffer)
	{
		gglDeleteBuffersARB( 1, &m_vertexBuffer );
		m_vertexBuffer = 0;
	}
}

static LPDIRECT3DVERTEXDECLARATION9 GetVertexDecl()
{
	static D3DVERTEXELEMENT9 s_vertexDesc[] = {
		{0, LandscapeRenderer::m_posOffset, D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
		{0, LandscapeRenderer::m_normOffset, D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0},
		{0, LandscapeRenderer::m_colOffset, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0},
		{0, LandscapeRenderer::m_uvOffset, D3DDECLTYPE_FLOAT2,    D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
		{0xFF,0,D3DDECLTYPE_UNUSED, 0,0,0} // D3DDECL_END 
	};

	if (!s_vertexDecl) 
	{
		OpenGLD3D::g_pd3dDevice->CreateVertexDeclaration( s_vertexDesc, &s_vertexDecl );
	}

	return s_vertexDecl;
};

#endif

void LandscapeRenderer::BuildOpenGlState(SurfaceMap2D <double> *_heightMap)
{
	BuildVertArrayAndTriStrip(_heightMap);
	BuildNormArray();
	BuildColourArray();
	BuildUVArray(_heightMap);

#ifdef USE_DIRECT3D
	// Flip the normals for Direct3D
	const int numUsed = m_verts.NumUsed();
	for (int i = 0; i < numUsed; ++i)
	{
		m_verts[i].m_norm = -1 * Vector3(m_verts[i].m_norm);
	}
#ifdef USE_DIRECT3D_TRILIST
	// Convert from tristrip to trilist (because of flatshading)
	if(m_verts.NumUsed())
	{
		FastDArray <LandVertex> trilist;
		LandTriangleStrip *strip = m_strips[0];
		trilist.SetSize((strip->m_numVerts-2)*3);
		for(unsigned i=0;i<strip->m_numVerts-2;i++)
		{
			//!!! skip degenerated
			LandVertex tmp = m_verts[i+2];
			tmp.m_pos = m_verts[i        ].m_pos; tmp.m_uv = m_verts[i        ].m_uv; trilist.PutData(tmp);
			tmp.m_pos = m_verts[i+1+(i%2)].m_pos; tmp.m_uv = m_verts[i+1+(i%2)].m_uv; trilist.PutData(tmp);
			tmp.m_pos = m_verts[i+2-(i%2)].m_pos; tmp.m_uv = m_verts[i+2-(i%2)].m_uv; trilist.PutData(tmp);
		}
		char* buf = new char[sizeof(m_verts)];
		memcpy(buf,&m_verts,sizeof(m_verts));
		memcpy(&m_verts,&trilist,sizeof(m_verts));
		memcpy(&trilist,buf,sizeof(m_verts));
		delete[] buf;
		trilist.Empty();
	}
#endif
#endif

	if (m_verts.NumUsed() <= 0)
		return;
		
	
	switch (m_renderMode) {
		case RenderModeVertexBufferObject:			
			AppDebugAssert(!m_vertexBuffer);

			if( NetGetCurrentThreadId() != g_app->m_mainThreadId )
			{
				Method<LandscapeRenderer> m( &LandscapeRenderer::DoBindBuffers, this );
				RunnableJob j( &m );			
				g_loadingScreen->QueueJob( &j );
				j.Wait();
			}
			else
				DoBindBuffers();
			break;

		case RenderModeDisplayList:
			// Generate main display list
			g_app->m_resource->CreateDisplayListAsync(
				MAIN_DISPLAY_LIST_NAME, 
				Method<LandscapeRenderer>( &LandscapeRenderer::RenderMainSlow, this ) );

			// Generate overlay display list
			g_app->m_resource->CreateDisplayListAsync(
				OVERLAY_DISPLAY_LIST_NAME,
				Method<LandscapeRenderer>( &LandscapeRenderer::RenderOverlaySlow, this ) );
			break;
	}
}
	
void LandscapeRenderer::DoBindBuffers()
{
	gglGenBuffersARB( 1, &m_vertexBuffer );
	gglBindBufferARB( GL_ARRAY_BUFFER_ARB, m_vertexBuffer );
	gglBufferDataARB( GL_ARRAY_BUFFER_ARB, m_verts.Size() * sizeof(LandVertex), m_verts.GetPointer(0), GL_STATIC_DRAW_ARB );
	gglBindBufferARB( GL_ARRAY_BUFFER_ARB, 0 );
}

void LandscapeRenderer::RenderMainSlow()
{
    GLfloat materialSpecular[] = { 0.0, 0.0, 0.0, 0.0 };
   	GLfloat materialDiffuse[] = { 1.0, 1.0, 1.0, 0.0 };
	GLfloat materialShininess[] = { 100.0 };
 

	glMaterialfv	(GL_FRONT, GL_SPECULAR, materialSpecular);
	glMaterialfv	(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, materialDiffuse);
	glMaterialfv	(GL_FRONT, GL_SHININESS, materialShininess);

	glEnable		(GL_LIGHT0);
	glEnable		(GL_LIGHT1);
    glEnable		(GL_COLOR_MATERIAL);
    glEnable		(GL_LIGHTING);

	
    if( g_app->m_negativeRenderer )
    {
        glEnable        (GL_BLEND);
        glBlendFunc     (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR);
    }
    
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);

	switch (m_renderMode) {
		case RenderModeVertexBufferObject:
			AppDebugAssert(m_vertexBuffer);
			gglBindBufferARB	( GL_ARRAY_BUFFER_ARB, m_vertexBuffer );
#ifdef USE_DIRECT3D
			OpenGLD3D::g_pd3dDevice->SetVertexDeclaration( GetVertexDecl() );
			OpenGLD3D::g_pd3dDevice->SetStreamSource( 0, *OpenGLD3D::g_currentVertexBuffer, 0, sizeof(LandVertex) );
#else
			glVertexPointer		( 3, GL_FLOAT, sizeof(LandVertex), (char*)m_posOffset );
			glNormalPointer		( GL_FLOAT, sizeof(LandVertex), (char*)m_normOffset );
			glColorPointer		( 4, GL_UNSIGNED_BYTE, sizeof(LandVertex), (char*)m_colOffset );
#endif
			break;

		default:
		{
			char *vertData = (char *)m_verts.GetPointer(0);
			glVertexPointer		( 3, GL_FLOAT, sizeof(LandVertex), vertData + m_posOffset );
			glNormalPointer		( GL_FLOAT, sizeof(LandVertex), vertData + m_normOffset );
			glColorPointer		( 4, GL_UNSIGNED_BYTE, sizeof(LandVertex), vertData + m_colOffset );
		}
	}

#ifdef USE_DIRECT3D_TRILIST
	if(m_verts.NumUsed())
	{
		if(g_fixedPipeline && OpenGLD3D::g_pd3dDevice==OpenGLD3D::g_pd3dDeviceActual)
		{
			g_fixedPipeline->Bind();
		}
		OpenGLD3D::g_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLELIST, 0, m_verts.NumUsed()/3);
	}
#else
	for (int z = 0; z < m_strips.Size(); ++z)
	{
			LandTriangleStrip *strip = m_strips[z];
#ifdef USE_DIRECT3D
			if(strip->m_numVerts>2)
			{
				if(g_fixedPipeline) g_fixedPipeline->Bind();
				OpenGLD3D::g_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, strip->m_firstVertIndex, strip->m_numVerts-2);
			}
#else
			glDrawArrays(GL_TRIANGLE_STRIP, strip->m_firstVertIndex, strip->m_numVerts);
#endif
	}
#endif

	if (m_renderMode == RenderModeVertexBufferObject)
    {
		gglBindBufferARB( GL_ARRAY_BUFFER_ARB, NULL );
	}

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);

    glDisable		(GL_COLOR_MATERIAL);
	glDisable		(GL_LIGHTING);
	glDisable		(GL_LIGHT0);
    glDisable		(GL_LIGHT1);
}


void LandscapeRenderer::RenderOverlaySlow()
{
	glEnable		(GL_COLOR_MATERIAL);
    glEnable		(GL_BLEND);
    glEnable		(GL_TEXTURE_2D);
	glDepthMask		(false);
	
    if( !g_app->m_negativeRenderer )    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    else								glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR);

	int outlineTextureId  = g_app->m_resource->GetTexture("textures/triangleOutline.bmp", true, false);
    glBindTexture	(GL_TEXTURE_2D, outlineTextureId);
	glTexParameteri	(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri	(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
    glTexParameteri	(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
    glTexParameteri	(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
    
    glEnable        (GL_LIGHTING);
    glEnable        (GL_LIGHT0);
    glEnable        (GL_LIGHT1);
    GLfloat materialSpecular[] = { 0.5, 0.5, 0.5, 1.0 };
   	GLfloat materialDiffuse[] = { 1.2, 1.2, 1.2, 0.0 };
	GLfloat materialShininess[] = { 40.0 };
 
	glMaterialfv	(GL_FRONT, GL_SPECULAR, materialSpecular);
	glMaterialfv	(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, materialDiffuse);
	glMaterialfv	(GL_FRONT, GL_SHININESS, materialShininess);

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);

	switch (m_renderMode) {
		case RenderModeVertexBufferObject:
			AppDebugAssert(m_vertexBuffer);
			gglBindBufferARB	( GL_ARRAY_BUFFER_ARB, m_vertexBuffer );
#ifdef USE_DIRECT3D
			OpenGLD3D::g_pd3dDevice->SetVertexDeclaration( GetVertexDecl() );
			OpenGLD3D::g_pd3dDevice->SetStreamSource( 0, *OpenGLD3D::g_currentVertexBuffer, 0, sizeof(LandVertex) );
#else
			glVertexPointer		( 3, GL_FLOAT, sizeof(LandVertex), (char*)m_posOffset );
			glNormalPointer		( GL_FLOAT, sizeof(LandVertex), (char*)m_normOffset );
			glColorPointer		( 4, GL_UNSIGNED_BYTE, sizeof(LandVertex), (char*)m_colOffset );
			glTexCoordPointer	( 2, GL_FLOAT, sizeof(LandVertex), (char*)m_uvOffset);
#endif
			break;

		default:
			{
				char *vertData = (char *)m_verts.GetPointer(0);
				glVertexPointer		( 3, GL_FLOAT, sizeof(LandVertex), vertData + m_posOffset );
				glNormalPointer		( GL_FLOAT, sizeof(LandVertex), vertData + m_normOffset );
				glColorPointer		( 4, GL_UNSIGNED_BYTE, sizeof(LandVertex), vertData + m_colOffset );
				glTexCoordPointer	( 2, GL_FLOAT, sizeof(LandVertex), vertData + m_uvOffset);
			}
			break;
	}

#ifdef USE_DIRECT3D_TRILIST
	if(m_verts.NumUsed())
	{
		if(g_fixedPipeline && OpenGLD3D::g_pd3dDevice==OpenGLD3D::g_pd3dDeviceActual)
		{
			g_fixedPipeline->Bind();
		}
		OpenGLD3D::g_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLELIST, 0, m_verts.NumUsed()/3);
	}
#else
	for (int z = 0; z < m_strips.Size(); ++z)
	{
		LandTriangleStrip *strip = m_strips[z];

#ifdef USE_DIRECT3D
		if(g_fixedPipeline) g_fixedPipeline->Bind();
		OpenGLD3D::g_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, strip->m_firstVertIndex, strip->m_numVerts-2);
#else
		glDrawArrays(GL_TRIANGLE_STRIP, strip->m_firstVertIndex, strip->m_numVerts);
#endif

	}
#endif

	switch (m_renderMode) {
		case RenderModeVertexBufferObject:
			gglBindBufferARB( GL_ARRAY_BUFFER_ARB, NULL );
			break;
	}

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);

	glDisable		(GL_COLOR_MATERIAL);
    glDisable		(GL_BLEND);   
    glDisable		(GL_TEXTURE_2D);
	glDisable		(GL_COLOR_MATERIAL);
    glDisable       (GL_LIGHTING);
    glDisable       (GL_LIGHT0);
    glDisable       (GL_LIGHT1);
	glDepthMask		(true);
	glBlendFunc		(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glTexParameteri	(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri	(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
}


void LandscapeRenderer::Render()
{
	if (m_verts.Size() <= 0)
		return;

    g_app->m_location->SetupFog();
	glEnable		(GL_FOG);

	START_PROFILE( "Render Landscape Main");

	switch (m_renderMode) {
		case RenderModeDisplayList:
			{
				int id = g_app->m_resource->GetDisplayList(MAIN_DISPLAY_LIST_NAME);
				AppDebugAssert(id != -1);
				glCallList(id);
			}
			break;

		default:
			RenderMainSlow();
			break;
	}
	END_PROFILE( "Render Landscape Main");

    int landscapeDetail = 1;//g_prefsManager->GetInt( "RenderLandscapeDetail", 1 );
    if( landscapeDetail < 4 )
    {
	    START_PROFILE( "Render Landscape Overlay");
		switch (m_renderMode) {
			case RenderModeDisplayList:
				{
					int id = g_app->m_resource->GetDisplayList(OVERLAY_DISPLAY_LIST_NAME);
					AppDebugAssert(id != -1);
					glCallList(id);
				}
				break;

			default:
			    RenderOverlaySlow();
				;
	    }
	    END_PROFILE( "Render Landscape Overlay");
    }
    
    glDisable		(GL_FOG);
}
