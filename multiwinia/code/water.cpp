#include "lib/universal_include.h"

#ifdef USE_DIRECT3D
#include "lib/opengl_directx_internals.h"
#include "lib/shader.h"
#include "lib/texture.h"
#endif

#include <math.h>
#include <float.h>

#include "lib/2d_array.h"
#include "lib/filesys/binary_stream_readers.h"
#include "lib/bitmap.h"
#include "lib/debug_utils.h"
#include "lib/hi_res_time.h"
#include "lib/math_utils.h"
#include "lib/ogl_extensions.h"
#include "lib/profiler.h"
#include "lib/preferences.h"
#include "lib/render_utils.h"
#include "lib/resource.h"
#include "lib/math/random_number.h"

#include "app.h"
#include "camera.h"
#include "main.h"
#include "renderer.h"
#include "water.h"
#include "water_reflection.h"
#include "location.h"
#include "level_file.h"
#include "net_lib.h"

#define LIGHTMAP_TEXTURE_NAME "water_lightmap"

float const waveBrightnessScale = 4.0;
float const shoreBrighteningFactor = 250.0;
float const shoreNoiseFactor = 0.25;
#ifdef USE_DIRECT3D
LPDIRECT3DVERTEXDECLARATION9 s_vertexDecl = NULL;
#endif

#ifdef USE_DIRECT3D
	#include "lib/FixedPipeline.h"
	#define USE_DIRECT3D_TRILIST // Convert from tristrip to trilist (because of flatshading)
#endif

#define RENDER_WATER_DETAIL g_prefsManager->GetInt( "RenderWaterDetail" )

// ****************************************************************************
// Class Water
// ****************************************************************************

Water::Water()
:	m_waterDepths(NULL),
	m_shoreNoise(NULL),
	m_waterDepthMap(NULL),
	m_waveTableX(NULL),
	m_waveTableZ(NULL),
	m_renderWaterEffect(0),
    m_flatWaterTiles(NULL),
#ifdef USE_DIRECT3D
	m_renderVertexBuffer(0),
	m_prepareVertexBuffer(0),
	m_updateWaterThread(NULL),
#endif
	m_colourTable(NULL)
#ifdef TARGET_OS_MACOSX
	,m_updateCount(0),
	m_lastUpdateTime(0.0)
#endif
{

#ifdef USE_DIRECT3D
	memset( m_vertexBuffer, 0, sizeof( m_vertexBuffer ) );
	memset( m_vertexBufferReady, 0, sizeof(m_vertexBufferReady) );
#endif

    //if( !g_app->m_editing )
    {
        Landscape *land = &g_app->m_location->m_landscape;
        
	    GenerateLightMap();

	    int detail = RENDER_WATER_DETAIL;

        if (detail > 0)
	    {
            float worldSize = max( g_app->m_location->m_landscape.GetWorldSizeX(),
                                 g_app->m_location->m_landscape.GetWorldSizeZ() );
            worldSize /= 100.0;
        
            m_cellSize = (float)detail * worldSize;

		    int alpha = ( g_app->m_negativeRenderer ? 0 : 255 );

		    // Load colour information from a bitmap
		    {
                char fullFilename[256];
                sprintf( fullFilename, "terrain/%s", g_app->m_location->m_levelFile->m_wavesColourFilename );

                if( g_app->m_location->ChristmasModEnabled() )
                {
                    strcpy( fullFilename, "terrain/waves_earth.bmp" );
                }

			    //BinaryReader *in = g_app->m_resource->GetBinaryReader(fullFilename);
			    //BitmapRGBA bmp(in, "bmp");
                const BitmapRGBA *bmp = g_app->m_resource->GetBitmap( fullFilename );
			    m_colourTable = new RGBAColour[bmp->m_width];
			    m_numColours = bmp->m_width;
			    for( int x = 0; x < bmp->m_width; ++x )
			    {
#ifndef USE_DIRECT3D
				    m_colourTable[x] = bmp->GetPixel( x, 1 );
				    m_colourTable[x].a = alpha;
#else
                    RGBAColour col = bmp.GetPixel( x, 1 );
                    col.a = alpha;

                    D3DCOLOR c = D3DCOLOR_RGBA( col.r, col.g, col.b, col.a );
                    memcpy( &m_colourTable[x], &c, sizeof(D3DCOLOR) );
#endif

			    }
			    //delete in;
		    }

		    BuildTriangleStrips();

		    m_waveTableSizeX = (2.0 * land->GetWorldSizeX()) / m_cellSize + 2;
		    m_waveTableSizeZ = (2.0 * land->GetWorldSizeZ()) / m_cellSize + 2;
		    m_waveTableX = new float[m_waveTableSizeX];
		    m_waveTableZ = new float[m_waveTableSizeZ];

#ifdef USE_DIRECT3D_TRILIST
			// Convert from tristrip to trilist (because of flatshading)
			if(m_renderVerts.Size())
			{
				FastDArray <WaterVertex> trilist;
				WaterTriangleStrip *strip = m_strips[0];
				trilist.SetSize((strip->m_numVerts-2)*3);
				for(unsigned i=0;i<strip->m_numVerts-2;i++)
				{
					WaterVertex tmp = m_renderVerts[i];
					trilist.PutData(tmp);
					if((Vector3(m_renderVerts[i].m_pos)-Vector3(m_renderVerts[i+1].m_pos)).Mag()
						+ (Vector3(m_renderVerts[i].m_pos)-Vector3(m_renderVerts[i+2].m_pos)).Mag()
						+ (Vector3(m_renderVerts[i+1].m_pos)-Vector3(m_renderVerts[i+2].m_pos)).Mag() >20)
					{
						// degenerated triangle, connects strips
						trilist.PutData(tmp);
						trilist.PutData(tmp);
					}
					else
					{
						// standard triangle
						tmp.m_pos = m_renderVerts[i+1+(i%2)].m_pos; trilist.PutData(tmp);
						tmp.m_pos = m_renderVerts[i+2-(i%2)].m_pos; trilist.PutData(tmp);
					}
				}
				char* buf = new char[sizeof(m_renderVerts)];
				memcpy(buf,&m_renderVerts,sizeof(m_renderVerts));
				memcpy(&m_renderVerts,&trilist,sizeof(m_renderVerts));
				memcpy(&trilist,buf,sizeof(m_renderVerts));
				delete[] buf;
				trilist.Empty();
			}
#endif

		    BuildOpenGlState();
			UpdateDynamicWater();
	    }
    }
}


Water::~Water()
{		
#ifdef USE_DIRECT3D
	if( m_updateWaterThread )
	{
		AppDebugOut("Killing water thread: %p\n", m_updateWaterThread);
		m_terminating.Signal();
		WaitForSingleObject( m_updateWaterThread, INFINITE );		
	}
#endif
	m_strips.EmptyAndDelete();
	m_renderVerts.Empty();
	delete [] m_waterDepths;	m_waterDepths = NULL;
	delete [] m_shoreNoise;		m_shoreNoise = NULL;
	delete [] m_colourTable;	m_colourTable = NULL;
	delete [] m_waveTableX;		m_waveTableX = NULL;
	delete [] m_waveTableZ;		m_waveTableZ = NULL;
    if( m_flatWaterTiles ) 
    {
        delete m_flatWaterTiles;
        m_flatWaterTiles = NULL;
    }
    if( m_waterDepthMap )
    {
        delete m_waterDepthMap;
        m_waterDepthMap = NULL;
    }
#ifdef USE_DIRECT3D
	ReleaseD3DResources();
#endif
}

void Water::GenerateLightMap()
{
    double startTime = GetHighResTime();

    #define MASK_SIZE 128

	float landSizeX = g_app->m_location->m_landscape.GetWorldSizeX();
    float landSizeZ = g_app->m_location->m_landscape.GetWorldSizeZ();
	float scaleFactorX = 2.0 * landSizeX / (float)MASK_SIZE;
	float scaleFactorZ = 2.0 * landSizeZ / (float)MASK_SIZE;
	int low = MASK_SIZE / 4;
	int high = (MASK_SIZE / 4) * 3;
	float offX = low * scaleFactorX;
	float offZ = low * scaleFactorZ;


    //
    // Generate basic mask image data
    
	Array2D <float> landData;
	landData.Initialise(MASK_SIZE, MASK_SIZE, 0.0);
	landData.SetAll(0.0);

    for (int z = low; z < high; ++z )
    {
	    for (int x = low; x < high; ++x )
        {
			float landHeight = g_app->m_location->m_landscape.m_heightMap->GetValue(
									(0.0 + (float)x) * scaleFactorX - offX,
									(0.0 + (float)z) * scaleFactorZ - offZ);
            if( landHeight > 0.0 )
            {
                landData.PutData(x, z, 1.0);
            }
        }
    }


	//
	// Horizontal blur
	
	int const blurSize = 11;
	int const halfBlurSize = 5;
	float m[blurSize] = { 0.2, 0.3, 0.4, 0.5, 0.8, 1.0, 0.8, 0.5, 0.4, 0.3, 0.2 };
	for (int i = 0; i < blurSize; ++i)
	{
		m[i] /= 5.4;
	}

	Array2D <float> tempData;
	tempData.Initialise(MASK_SIZE, MASK_SIZE, 0.0);
	tempData.SetAll(0.0);
	for (int z = 0; z < MASK_SIZE; ++z)
	{
		for (int x = 0; x < MASK_SIZE; ++x)
		{
			float val = landData.GetData(x, z);
			if (NearlyEquals(val, 0.0)) continue;

			for (int i = 0; i < blurSize; ++i)
			{
				tempData.AddToData(x + i - halfBlurSize, z, val * m[i]);
			}
		}
	}
	

	//
	// Vertical blur

	landData.SetAll(0.0);
	for (int z = 0; z < MASK_SIZE; ++z)
	{
		for (int x = 0; x < MASK_SIZE; ++x)
		{
			float val = tempData.GetData(x, z);
			if (NearlyEquals(val, 0.0)) continue;

			for (int i = 0; i < blurSize; ++i)
			{
				landData.AddToData(x, z + i - halfBlurSize, val * m[i]);
			}
		}
	}
	

    //
    // Generate finished image and upload to openGL

    BitmapRGBA finalImage( MASK_SIZE, MASK_SIZE );
    for (int x = 0; x < MASK_SIZE; ++x)
    {
        for (int z = 0; z < MASK_SIZE; ++z)
        {
            float grayVal = (float)landData.GetData(x, z) * 855.0;
			if (grayVal > 255.0) grayVal = 255.0;
            finalImage.PutPixel( x, MASK_SIZE - z - 1, RGBAColour(grayVal, grayVal, grayVal) );    
        }
    }

    if( g_app->m_resource->GetBitmap(LIGHTMAP_TEXTURE_NAME) != NULL )
    {
        g_app->m_resource->DeleteBitmap(LIGHTMAP_TEXTURE_NAME);
	}

    if( g_app->m_resource->DoesTextureExist(LIGHTMAP_TEXTURE_NAME) )
	{
        g_app->m_resource->DeleteTextureAsync(LIGHTMAP_TEXTURE_NAME);
    }

    g_app->m_resource->AddBitmap(LIGHTMAP_TEXTURE_NAME, finalImage);

    

	//
	// Create the water depth map

	float depthMapCellSize = (landSizeX * 2.0) / (float)finalImage.m_height;

	if (m_waterDepthMap)
		delete m_waterDepthMap;
	m_waterDepthMap = new SurfaceMap2D <float> (
							landSizeX * 2.0, landSizeZ * 2.0,
							-landSizeX/2.0, -landSizeZ/2.0,
							depthMapCellSize, depthMapCellSize, 1.0);
	//if (!g_app->m_editing)
	{
		for (int z = 0; z < finalImage.m_height; ++z)
		{
			for (int x = 0; x < finalImage.m_width; ++x)
			{
				RGBAColour pixel = finalImage.GetPixel(x, z);
				float depth = (float)pixel.g / 255.0;
				depth = 1.0 - depth;
				m_waterDepthMap->PutData(x, finalImage.m_height - z - 1, depth);
			}
		}
	}

	
	//
	// Take a low res copy of the water depth map that the flat water renderer
	// can efficiently use to determine where under water polys are needed
	
	int const flatWaterRatio = 4;	// One flat water quad is the same size as 8x8 dynamic water quads 
	scaleFactorX *= flatWaterRatio;
	scaleFactorZ *= flatWaterRatio;

	if( m_flatWaterTiles )
		delete m_flatWaterTiles;
	m_flatWaterTiles = new Array2D<bool> (m_waterDepthMap->GetNumColumns() / flatWaterRatio,
										  m_waterDepthMap->GetNumRows() / flatWaterRatio,
										  false);

	for (int z = 0; z < m_flatWaterTiles->GetNumRows(); ++z)
	{
		for (int x = 0; x < m_flatWaterTiles->GetNumColumns(); ++x)
		{
			int currentVal = 0;
			for (int dz = 0; dz < flatWaterRatio; ++dz)
			{
				for (int dx = 0; dx < flatWaterRatio; ++dx)
				{
					float depth = m_waterDepthMap->GetData(x * flatWaterRatio + dx,
														   z * flatWaterRatio + dz);
					if (depth >= 1.0)		// Very deep
						++currentVal;
				}
			}
		
			int const topScore = flatWaterRatio * flatWaterRatio;
			if (currentVal == topScore)	
				m_flatWaterTiles->PutData(x, m_flatWaterTiles->GetNumRows() - z - 1, false);
			else
				m_flatWaterTiles->PutData(x, m_flatWaterTiles->GetNumRows() - z - 1, true);
		}
	}


    double totalTime = GetHighResTime() - startTime;
    AppDebugOut( "Water lightmap generation took %dms\n", int(totalTime * 1000) );
}


void Water::BuildOpenGlState()
{
#ifdef USE_DIRECT3D
	NetLockMutex l(m_vertexBufferLock);

	for( int i = 0; i < NumVertexBuffers; i++ )
	{
		SAFE_RELEASE( m_vertexBuffer[i] );
		m_vertexBufferReady[i] = false;
	}
#endif
}


bool Water::IsVertNeeded(float x, float z)
{
	float landHeight = g_app->m_location->m_landscape.m_heightMap->GetValue(x, z);
	if (landHeight > 4.0) 
	{
		return false;
	}

   // if( g_prefsManager->GetInt( "RenderWaterDetail", 1 ) == 1 ) return true;

	float waterDepth = m_waterDepthMap->GetValue(x, z);
	if (waterDepth > 0.999)
	{
		return false;
	}

	return true;
}


void Water::BuildTriangleStrips()
{
	m_renderVerts.SetStepDouble();
	m_strips.SetStepDouble();

	float const landSizeX = g_app->m_location->m_landscape.GetWorldSizeX();
	float const landSizeZ = g_app->m_location->m_landscape.GetWorldSizeZ();

	float const lowX = -landSizeX * 0.5;
	float const lowZ = -landSizeZ * 0.5;
	float const highX = landSizeX * 1.5;
	float const highZ = landSizeZ * 1.5;
	int const maxXb = (highX - lowX) / m_cellSize;
	int const maxZb = (highZ - lowZ) / m_cellSize;

	WaterTriangleStrip *strip = new WaterTriangleStrip;
	strip->m_startRenderVertIndex = 0;
	int degen = 0;
	WaterVertex vertex1, vertex2;

	for (int zb = 0; zb < maxZb; ++zb)
	{
		float fz = lowZ + (float)zb * m_cellSize;

		for (int xb = 0; xb < maxXb; ++xb)
		{
			float fx = lowX + (float)xb * m_cellSize;
			bool needed1 = IsVertNeeded(fx - m_cellSize, fz);
			bool needed2 = IsVertNeeded(fx - m_cellSize, fz + m_cellSize);
			bool needed3 = IsVertNeeded(fx, fz);
			bool needed4 = IsVertNeeded(fx, fz + m_cellSize);
			bool needed5 = IsVertNeeded(fx + m_cellSize, fz);
			bool needed6 = IsVertNeeded(fx + m_cellSize, fz + m_cellSize);

			if (needed1 || needed2 || needed3 || needed4 || needed5 || needed6)
			{
				// Is needed, so add it to the strip
				vertex1.m_pos.Set(fx, 0.0, fz);
				vertex2.m_pos.Set(fx, 0.0, fz + m_cellSize);
#ifndef USE_DIRECT3D_TRILIST
				if(degen==1)
				{
					m_renderVerts.PutData(vertex1);
					m_renderVerts.PutData(vertex1);
				}
				degen = 2;
#endif
				m_renderVerts.PutData(vertex1);		
				m_renderVerts.PutData(vertex2);		
			}
#ifndef USE_DIRECT3D_TRILIST
			else
			{
				// Not needed, add degenerated joint.
				if(degen==2)
				{
					m_renderVerts.PutData(vertex2);
					m_renderVerts.PutData(vertex2);
					degen = 1;
				}
			}
#endif
		}
	}

	strip->m_numVerts = m_renderVerts.NumUsed();
	m_strips.PutData(strip);

	// Down-size the finished FastDArrays to fit their data tightly
	m_renderVerts.SetSize(m_renderVerts.NumUsed());
	m_strips.SetSize(m_strips.NumUsed());

	// Up-size the empty FastDArrays to be the same size as the vertex array
	m_waterDepths = new float[m_renderVerts.NumUsed()];
	m_shoreNoise = new float[m_renderVerts.NumUsed()];

	// Create other per-vertex arrays 
	for (int i = 0; i < m_renderVerts.Size(); ++i)
	{
		Vector3 const &pos = m_renderVerts[i].m_pos;
		float depth = m_waterDepthMap->GetValue(pos.x, pos.z);
		m_waterDepths[i] = depth;
		float shoreness = 1.0 - depth;
		float whiteness = shoreness + sfrand(shoreness) * shoreNoiseFactor;
		whiteness *= shoreBrighteningFactor;
		m_shoreNoise[i] = whiteness;
	}

	delete m_waterDepthMap; m_waterDepthMap = NULL;
}


#ifdef USE_DIRECT3D
static LPDIRECT3DVERTEXDECLARATION9 GetVertexDeclWater()
{
	static D3DVERTEXELEMENT9 s_vertexDesc[] = {
		{0, 0, D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
		{0, 12, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0},
		{0, 16, D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0},
		{0xFF,0,D3DDECLTYPE_UNUSED, 0,0,0} // D3DDECL_END 
	};

	if (!s_vertexDecl) 
	{
		OpenGLD3D::g_pd3dDevice->CreateVertexDeclaration( s_vertexDesc, &s_vertexDecl );
	}

	return s_vertexDecl;
};

void Water::ReleaseD3DPoolDefaultResources()
{
	BuildOpenGlState();
}

void Water::ReleaseD3DResources()
{
	ReleaseD3DPoolDefaultResources();

	SAFE_RELEASE(s_vertexDecl);
}
#endif

void Water::RenderFlatWaterTiles(
		float posNorth, float posSouth, float posEast, float posWest, float height,
		float texNorth1, float texSouth1, float texEast1, float texWest1,
		float texNorth2, float texSouth2, float texEast2, float texWest2, int steps)
{
    float sizeX = posWest - posEast;
    float sizeZ = posSouth - posNorth;
	float posStepX = sizeX / (float)steps;
	float posStepZ = sizeZ / (float)steps;
	
	float texSizeX1 = texWest1 - texEast1;
	float texSizeZ1 = texSouth1 - texNorth1;
	float texStepX1 = texSizeX1 / (float)steps;
	float texStepZ1 = texSizeZ1 / (float)steps;

	float texSizeX2 = texWest2 - texEast2;
	float texSizeZ2 = texSouth2 - texNorth2;
	float texStepX2 = texSizeX2 / (float)steps;
	float texStepZ2 = texSizeZ2 / (float)steps;

    glBegin(GL_QUADS);
		for (int j = 0; j < steps; ++j)
		{
			float pz = posNorth + j * posStepZ;
			float tz1 = texNorth1 + j * texStepZ1;
			float tz2 = texNorth2 + j * texStepZ2;

			for (int i = 0; i < steps; ++i)
			{
				float px = posEast + i * posStepX;

				if (m_flatWaterTiles->GetData(i, j) == false) continue;

				float tx1 = texEast1 + i * texStepX1;
				float tx2 = texEast2 + i * texStepX2;

				gglMultiTexCoord2fARB(GL_TEXTURE0_ARB, tx1 + texStepX1, tz1);
				gglMultiTexCoord2fARB(GL_TEXTURE1_ARB, tx2 + texStepX2, tz2);
				glVertex3f(px + posStepX, height, pz);

				gglMultiTexCoord2fARB(GL_TEXTURE0_ARB, tx1 + texStepX1, tz1 + texStepZ1);
				gglMultiTexCoord2fARB(GL_TEXTURE1_ARB, tx2 + texStepX2, tz2 + texStepZ2);
				glVertex3f(px + posStepX, height, pz + posStepZ);
				
				gglMultiTexCoord2fARB(GL_TEXTURE0_ARB, tx1, tz1 + texStepZ1);
				gglMultiTexCoord2fARB(GL_TEXTURE1_ARB, tx2, tz2 + texStepZ2);
				glVertex3f(px, height, pz + posStepZ);

				gglMultiTexCoord2fARB(GL_TEXTURE0_ARB, tx1, tz1);
				gglMultiTexCoord2fARB(GL_TEXTURE1_ARB, tx2, tz2);
				glVertex3f(px, height, pz);
			}
		}
	glEnd();
}


void Water::RenderFlatWater()
{    
    Landscape *land = &g_app->m_location->m_landscape;

	glDisable			(GL_CULL_FACE);
	glEnable			(GL_FOG);
    glDisable           (GL_BLEND);
	glDepthMask			(false);

    if( g_app->m_negativeRenderer )
    {
		glEnable		(GL_BLEND);
        glBlendFunc     (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR);
	    glColor4ub		(255, 255, 255, 0);
    }
    else
    {
        glColor4ub		(255, 255, 255, 255);
    }

	char waterFilename[256];
	sprintf( waterFilename, "terrain/%s", g_app->m_location->m_levelFile->m_waterColourFilename );
    
    if( g_app->m_location->ChristmasModEnabled() )
    {
        strcpy( waterFilename, "terrain/water_icecaps.bmp" );
    }

    gglActiveTextureARB (GL_TEXTURE0_ARB);
    glBindTexture	    (GL_TEXTURE_2D, g_app->m_resource->GetTexture(waterFilename, true, true));
	glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
    glTexEnvf           (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glTexEnvf           (GL_TEXTURE_ENV, GL_COMBINE_RGB_EXT, GL_REPLACE);
    glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
    glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
	glEnable		    (GL_TEXTURE_2D);

	// JAK HACK (DISABLED)
	gglActiveTextureARB (GL_TEXTURE1_ARB);
    glBindTexture	    (GL_TEXTURE_2D, g_app->m_resource->GetTexture(LIGHTMAP_TEXTURE_NAME));
	glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
    glTexEnvf           (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_EXT);
    glTexEnvf           (GL_TEXTURE_ENV, GL_COMBINE_RGB_EXT, GL_MODULATE); 
	glEnable		    (GL_TEXTURE_2D);

    float landSizeX = land->GetWorldSizeX();
    float landSizeZ = land->GetWorldSizeZ();
	float borderX = landSizeX / 2.0;
    float borderZ = landSizeZ / 2.0;

	float const timeFactor = g_gameTimer.GetAccurateTime() / 30.0;

#ifdef USE_DIRECT3D
	// sets default vertex format (somebody changes it and not returns back, where?)
	extern LPDIRECT3DVERTEXDECLARATION9 s_pCustomVertexDecl;
	OpenGLD3D::g_pd3dDevice->SetVertexDeclaration( s_pCustomVertexDecl );
#endif

	RenderFlatWaterTiles(
		landSizeZ + borderZ, -borderZ, -borderX, landSizeX + borderX, -9.0, 
		timeFactor, timeFactor + 30.0, timeFactor, timeFactor + 30.0,
		0.0, 1.0, 0.0, 1.0, 
		m_flatWaterTiles->GetNumColumns());

	gglActiveTextureARB  (GL_TEXTURE1_ARB);
    glDisable		    (GL_TEXTURE_2D);

    gglActiveTextureARB  (GL_TEXTURE0_ARB);
    glDisable		    (GL_TEXTURE_2D);
    glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
    glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );
    glTexEnvf           (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	glEnable		(GL_CULL_FACE);
    glDisable       (GL_BLEND);
    glBlendFunc     (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable       (GL_TEXTURE_2D);
    glDisable       (GL_FOG);
	glDepthMask		(true);
}

bool isIdentical(const Vector3& a,const Vector3& b,const Vector3& c)
{
	return a.x==b.x && a.x==c.x && a.z==b.z && a.z==c.z;
}

#ifdef USE_DIRECT3D
#define START_PROFILE_NO_DIRECT3D(...) {}
#define END_PROFILE_NO_DIRECT3D(...) {}
#else
#define START_PROFILE_NO_DIRECT3D( x ) START_PROFILE( x );
#define END_PROFILE_NO_DIRECT3D( x ) END_PROFILE( x );
#endif

void Water::UpdateDynamicWater()
{
	START_PROFILE_NO_DIRECT3D(  "LookupTables" );
	float const scaleFactor = 7.0;

	// 
	// Generate lookup tables

    float timeNow = g_gameTimer.GetAccurateTime();

#ifdef TARGET_OS_MACOSX
	// we only update at most every other frame
	if(++m_updateCount == 2)
		m_updateCount = 0;
	else
		return;

	// we max the water update at 15 times / second
	if((timeNow - m_lastUpdateTime) < 1.0f / 15.0f)
		return;
	
	m_lastUpdateTime = timeNow;
#endif	
	// DEF: Optimization, using floats explicitly so the compiler doesn't use doubles, which then results in mixed precision arithmetic which is slow
	for (int i = 0; i < m_waveTableSizeX; ++i)
	{
		float x = (float)i * m_cellSize;
		float heightForX = iv_sin(x * 0.01f + timeNow * 0.65f) * 1.2f;
		heightForX += iv_sin(x * 0.03f + timeNow * 1.5f) * 0.9f;
		m_waveTableX[i] = heightForX * scaleFactor;
	}
	for (int i = 0; i < m_waveTableSizeZ; ++i)
	{
		float z = (float)i * m_cellSize;
		float heightForZ = iv_sin(z * 0.02f + timeNow * 0.75f) * 0.9f;
		heightForZ += iv_sin(z * 0.03f + timeNow * 1.85f) * 0.65f;
		m_waveTableZ[i] = heightForZ * scaleFactor;
	}

	END_PROFILE_NO_DIRECT3D(  "LookupTables" );
	START_PROFILE_NO_DIRECT3D(  "VertexHeights" );
	//
	// Go through all the strips, updating vertex heights and poly colours

#ifdef USE_DIRECT3D_TRILIST
	int totalNumVertices = 0;

	for (int i = 0; i < m_strips.Size(); ++i)
	{		
		WaterTriangleStrip *strip = m_strips[i];

		float prevHeight1 = 0.0;
		float prevHeight2 = 0.0;

		totalNumVertices += strip->m_numVerts;

		//
		// Set through the triangle strip in pairs of vertices

		int const finalVertIndex = strip->m_startRenderVertIndex + strip->m_numVerts - 1;
		for (int j = strip->m_startRenderVertIndex; j < finalVertIndex-2; ++j)
		{
			WaterVertex *vertex1  = &m_renderVerts[3*j];
			WaterVertex *vertex2  = &m_renderVerts[3*(j+1)];

			float const landSizeX = g_app->m_location->m_landscape.GetWorldSizeX();
			float const landSizeZ = g_app->m_location->m_landscape.GetWorldSizeZ();
			float const lowX = -landSizeX * 0.5;
			float const lowZ = -landSizeZ * 0.5;
			int indexX = int((vertex1->m_pos.x-lowX)/m_cellSize+0.1);
			int indexZ = int((vertex1->m_pos.z-lowZ)/m_cellSize+0.1);
			AppDebugAssert(indexX < m_waveTableSizeX);
			AppDebugAssert(indexZ + 1 < m_waveTableSizeZ);

			// Update the height and calc brightness for FIRST vertex of the pair 
			vertex1->m_pos.y = m_waveTableX[indexX] + m_waveTableZ[indexZ];
			vertex1->m_pos.y *= m_waterDepths[j];
			float brightness = (prevHeight1 + prevHeight2 + vertex1->m_pos.y) * waveBrightnessScale; 
			float shoreness = 1.0 - m_waterDepths[j];
			brightness *= shoreness;	
			brightness += m_shoreNoise[j];
			prevHeight1 = vertex1->m_pos.y;

			// Update the height and calc brightness for SECOND vertex of the pair 
			++j;
			vertex2->m_pos.y = m_waveTableX[indexX] + m_waveTableZ[indexZ + 1];
			vertex2->m_pos.y *= m_waterDepths[j];
			float brightness2 = (prevHeight2 + prevHeight1 + vertex2->m_pos.y) * waveBrightnessScale;
			shoreness = 1.0 - m_waterDepths[j];
			brightness2 *= shoreness;
			brightness2 += m_shoreNoise[j];
			prevHeight2 = vertex2->m_pos.y;

			// Now update the colours for the two vertices (and hence triangles), but
			// mix their colours together to reduce the sawtooth effect caused by too
			// much contrast between two triangles in the same quad.
			vertex1->m_col = GetColour(Round(brightness2 * 0.7 + brightness * 0.3));
			vertex2->m_col = GetColour(Round(brightness * 0.7 + brightness2 * 0.3));

			// Update vertex normals
			float dx1 = -(m_waveTableX[indexX+1] - m_waveTableX[indexX-1])*m_waterDepths[j-1];
			float dx2 = -(m_waveTableX[indexX+1] - m_waveTableX[indexX-1])*m_waterDepths[j  ];
			float dz1 = -(m_waveTableZ[indexZ+1] - m_waveTableZ[indexZ  ])*m_waterDepths[j-1];
			float dz2 = -(m_waveTableZ[indexZ+2] - m_waveTableZ[indexZ+1])*m_waterDepths[j  ];
			// realistic, but with artifacts around islands in wild water
			vertex1->m_normal = Vector3(dx1,vertex1->m_pos.y,dz1);
			vertex2->m_normal = Vector3(dx2,vertex2->m_pos.y,dz2);
			// no artifacts around islands in wild water, but much less realistic
			//vertex1->m_normal = Vector3(0,-vertex1->m_pos.y,0);
			//vertex2->m_normal = Vector3(0,-vertex2->m_pos.y,0);
		}
	}
	for (int j = 0; j+6 < m_renderVerts.Size(); j+=6)
	{
		// smooth pos & norm
		if((Vector3(m_renderVerts[j+6].m_pos)-Vector3(m_renderVerts[j].m_pos)).Mag()<200)
		{
			m_renderVerts[j+1] = m_renderVerts[j+3];
			m_renderVerts[j+2] = m_renderVerts[j+6];
			m_renderVerts[j+4] = m_renderVerts[j+9];
			m_renderVerts[j+5] = m_renderVerts[j+6];
		}
		// flat col
		m_renderVerts[j+1].m_col = m_renderVerts[j].m_col;
		m_renderVerts[j+2].m_col = m_renderVerts[j].m_col;
		m_renderVerts[j+4].m_col = m_renderVerts[j+3].m_col;
		m_renderVerts[j+5].m_col = m_renderVerts[j+3].m_col;
	}
#else
	int totalNumVertices = 0;

	for (int i = 0; i < m_strips.Size(); ++i)
	{		
		WaterTriangleStrip *strip = m_strips[i];

		float prevHeight1 = 0.0;
		float prevHeight2 = 0.0;

		totalNumVertices += strip->m_numVerts;

		//
		// Set through the triangle strip in pairs of vertices

		// DEF: Optimization of this loop.  Since the water is non-critical as far as syncing goes, I took some liberties in this hotspot.
		int const finalVertIndex = strip->m_startRenderVertIndex + strip->m_numVerts - 1;
		for (int j = strip->m_startRenderVertIndex; j < finalVertIndex; ++j)
		{
			WaterVertex *vertex1 = &m_renderVerts[j];
			WaterVertex *vertex2 = &m_renderVerts[j+1];

			double const landSizeX = g_app->m_location->m_landscape.GetWorldSizeX();
			double const landSizeZ = g_app->m_location->m_landscape.GetWorldSizeZ();
			double const lowX = -landSizeX * 0.5;
			double const lowZ = -landSizeZ * 0.5;
			int indexX = int((vertex1->m_pos.x-(float)lowX)/m_cellSize+0.1f);
			int indexZ = int((vertex1->m_pos.z-(float)lowZ)/m_cellSize+0.1f);
			AppDebugAssert(indexX < m_waveTableSizeX);
			AppDebugAssert(indexZ + 1 < m_waveTableSizeZ);

			// Update the height and calc brightness for FIRST vertex of the pair 
			vertex1->m_pos.y = m_waveTableX[indexX] + m_waveTableZ[indexZ];
			vertex1->m_pos.y *= m_waterDepths[j];
			if(j>=2 && isIdentical(m_renderVerts[j-2].m_pos,m_renderVerts[j-1].m_pos,vertex1->m_pos))
			{
				// end of degenerated joint
				m_renderVerts[j-2].m_pos.y = vertex1->m_pos.y;
				m_renderVerts[j-1].m_pos.y = vertex1->m_pos.y;
			}
			float brightness = (prevHeight1 + prevHeight2 + vertex1->m_pos.y) * waveBrightnessScale; 
			float shoreness = 1.0f - m_waterDepths[j];
			brightness *= shoreness;	
			brightness += m_shoreNoise[j];
			prevHeight1 = vertex1->m_pos.y;

			// Update the height and calc brightness for SECOND vertex of the pair 
			++j;
			vertex2->m_pos.y = m_waveTableX[indexX] + m_waveTableZ[indexZ + 1];
			vertex2->m_pos.y *= m_waterDepths[j];
			float brightness2 = (prevHeight2 + prevHeight1 + vertex2->m_pos.y) * waveBrightnessScale;
			shoreness = 1.0f - m_waterDepths[j];
			brightness2 *= shoreness;
			brightness2 += m_shoreNoise[j];
			prevHeight2 = vertex2->m_pos.y;

			// Now update the colours for the two vertices (and hence triangles), but
			// mix their colours together to reduce the sawtooth effect caused by too
			// much contrast between two triangles in the same quad.
			vertex1->m_col = GetColour(Round(brightness2 * 0.7f + brightness * 0.3f));
			vertex2->m_col = GetColour(Round(brightness * 0.7f + brightness2 * 0.3f));

			// Update vertex normals
			float dx1 = -(m_waveTableX[indexX+1] - m_waveTableX[indexX-1])*m_waterDepths[j-1];
			float dx2 = -(m_waveTableX[indexX+1] - m_waveTableX[indexX-1])*m_waterDepths[j  ];
			float dz1 = -(m_waveTableZ[indexZ+1] - m_waveTableZ[indexZ  ])*m_waterDepths[j-1];
			float dz2 = -(m_waveTableZ[indexZ+2] - m_waveTableZ[indexZ+1])*m_waterDepths[j  ];
			// realistic, but with artifacts around islands in wild water
			vertex1->m_normal = Vector3(dx1,vertex1->m_pos.y,dz1);
			vertex2->m_normal = Vector3(dx2,vertex2->m_pos.y,dz2);
			// no artifacts around islands in wild water, but much less realistic
			//vertex1->m_normal = Vector3(0,-vertex1->m_pos.y,0);
			//vertex2->m_normal = Vector3(0,-vertex2->m_pos.y,0);

			if(j>=2 && isIdentical(vertex1->m_pos,vertex2->m_pos,m_renderVerts[j-2].m_pos))
			{
				// start of degenerated joint
				vertex1->m_pos.y = m_renderVerts[j-2].m_pos.y;
				vertex2->m_pos.y = m_renderVerts[j-2].m_pos.y;
			}
		}
	}
#endif
	END_PROFILE_NO_DIRECT3D(  "VertexHeights" );

	START_PROFILE_NO_DIRECT3D(  "VertexBuffer" );

#ifdef USE_DIRECT3D
	if (m_renderVerts.Size() > 0) {
		// Create and update the command buffer 

		NetLockMutex l(m_vertexBufferLock);
		
		IDirect3DVertexBuffer9 *buffer = m_vertexBuffer[m_prepareVertexBuffer];

		int next = (m_prepareVertexBuffer + 1) % NumVertexBuffers;
		int nnext = (m_prepareVertexBuffer + 2) % NumVertexBuffers;

		if( buffer && 
			!m_vertexBufferReady[m_prepareVertexBuffer] &&
			// Make sure the next few are available too (otherwise we might have caught up 
			// with the currently rendering one and that wouldn't be good!)
			!m_vertexBufferReady[next] &&
			!m_vertexBufferReady[nnext] ) 
		{
			unsigned bufferSize = m_renderVerts.Size() * sizeof(WaterVertex);
			void *data;

			buffer->Lock(0, 0, &data, 0);
			memcpy(data, &m_renderVerts[0], bufferSize);
			buffer->Unlock();

			m_vertexBufferReady[m_prepareVertexBuffer] = true;
			m_prepareVertexBuffer = next;
		}
	}
#endif

	END_PROFILE_NO_DIRECT3D(  "VertexBuffer" );
}


void Water::RenderDynamicWater()
{
#ifdef USE_DIRECT3D
	NetLockMutex l(m_vertexBufferLock);

	int nextVertexBuffer = (m_renderVertexBuffer + 1) % NumVertexBuffers;

	if (m_vertexBuffer[nextVertexBuffer] && m_vertexBufferReady[nextVertexBuffer])
	{
		m_vertexBufferReady[m_renderVertexBuffer] = false;
		m_renderVertexBuffer = nextVertexBuffer;
	}

	if (!m_vertexBuffer[m_renderVertexBuffer] || !m_vertexBufferReady[m_renderVertexBuffer])
		return;
#else
	if (m_renderVerts.Size() == 0)
		return;
#endif

	glEnable		    (GL_BLEND);
    glBlendFunc         (GL_SRC_ALPHA, GL_ONE);
    glEnable            (GL_FOG);
	glEnable			(GL_CULL_FACE);

#ifdef USE_DIRECT3D
	IDirect3DVertexDeclaration9*	savedDecl;

	OpenGLD3D::g_pd3dDevice->GetVertexDeclaration( &savedDecl );
	OpenGLD3D::g_pd3dDevice->SetVertexDeclaration( GetVertexDeclWater() );
	OpenGLD3D::g_pd3dDevice->SetStreamSource( 0, m_vertexBuffer[m_renderVertexBuffer], 0, sizeof(WaterVertex) );	
#else
	glEnableClientState	(GL_VERTEX_ARRAY);
	glEnableClientState	(GL_COLOR_ARRAY);

	AppReleaseAssert(m_renderVerts.ValidIndex(0),"RenderDynamicWater: Invalid index 0 in DArray\n");
	glVertexPointer	(3, GL_FLOAT, sizeof(WaterVertex), &m_renderVerts[0].m_pos);

	glColorPointer	(4, GL_UNSIGNED_BYTE, sizeof(WaterVertex), &m_renderVerts[0].m_col);
#endif

#ifdef USE_DIRECT3D
	if (m_renderWaterEffect && g_waterReflectionEffect)
	{
		g_waterReflectionEffect->Start();
	}
#endif

#ifdef USE_DIRECT3D_TRILIST
	if(m_renderVerts.Size())
	{
		if(g_fixedPipeline && OpenGLD3D::g_pd3dDevice==OpenGLD3D::g_pd3dDeviceActual)
		{
			g_fixedPipeline->Bind();
		}
		OpenGLD3D::g_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLELIST, 0, m_renderVerts.Size()/3);
		// We unset the stream source so that it can be updated by the UpdateDynamicWater() method
		OpenGLD3D::g_pd3dDevice->SetStreamSource( 0, NULL, 0, 0 );
	}
#else
	int numStrips = m_strips.Size();
	for (int i = 0; i < numStrips; ++i)
	{
		WaterTriangleStrip *strip = m_strips[i];

#ifdef USE_DIRECT3D
		OpenGLD3D::g_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, strip->m_startRenderVertIndex, strip->m_numVerts-2);
#else
		glDrawArrays(GL_TRIANGLE_STRIP, 
					 strip->m_startRenderVertIndex,
					 strip->m_numVerts);
#endif
	}
#endif

#ifdef USE_DIRECT3D
	if (m_renderWaterEffect && g_waterReflectionEffect)
	{
		g_waterReflectionEffect->Stop();
	}
#endif
    
#ifdef USE_DIRECT3D
	OpenGLD3D::g_pd3dDevice->SetVertexDeclaration( savedDecl );
#else
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
#endif

    glDisable           (GL_FOG);
    glBlendFunc         (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glDisable           (GL_BLEND);
}

void Water::Render()
{
	m_renderWaterEffect = g_prefsManager->GetInt("RenderWaterEffect", 1)>0 && g_prefsManager->GetInt("RenderPixelShader", 1)>0;

	if( g_app->m_editing )
	{
        START_PROFILE(  "Render Water" );

	    glEnable		    (GL_TEXTURE_2D);
        glBindTexture	    (GL_TEXTURE_2D, g_app->m_resource->GetTexture("textures/triangleOutline.bmp", true, false));
	    glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	    glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
        glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
        glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
		Landscape *land = &g_app->m_location->m_landscape;
		glEnable(GL_BLEND);
		glColor4ub(250, 250, 250, 100);
        float size = 100.0;
		glBegin(GL_QUADS);
			glTexCoord2f(0.0,0.0);            glVertex3f(0, -10.0, 0);
			glTexCoord2f(size,0.0);            glVertex3f(0, -10.0, land->GetWorldSizeZ());
			glTexCoord2f(size,size);            glVertex3f(land->GetWorldSizeX(), -10.0, land->GetWorldSizeZ());
			glTexCoord2f(0.0,size);            glVertex3f(land->GetWorldSizeX(), -10.0, 0);
		glEnd();
        glDisable           (GL_TEXTURE_2D);
		glColor4ub(250, 250, 250, 30);
		glBegin(GL_QUADS);
			glTexCoord2f(0.0,0.0);            glVertex3f(0, -10.0, 0);
			glTexCoord2f(size,0.0);            glVertex3f(0, -10.0, land->GetWorldSizeZ());
			glTexCoord2f(size,size);            glVertex3f(land->GetWorldSizeX(), -10.0, land->GetWorldSizeZ());
			glTexCoord2f(0.0,size);            glVertex3f(land->GetWorldSizeX(), -10.0, 0);
		glEnd();
		glDisable(GL_BLEND);
        glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
        glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );

        END_PROFILE(  "Render Water" );
	}
	//else
	{
		if( RENDER_WATER_DETAIL > 0 )
        {
			//Advance();
			START_PROFILE(  "Render Water" );
			RenderFlatWater();
#ifdef USE_DIRECT3D
			if(OpenGLD3D::g_supportsHwVertexProcessing)
#endif
				RenderDynamicWater();
            END_PROFILE(  "Render Water" );
        }
        else
        {
            START_PROFILE(  "Render Water" );
    		RenderFlatWater();    
            END_PROFILE(  "Render Water" );
        }
	}
    
    g_app->m_location->SetupFog();
}

#ifdef USE_DIRECT3D
DWORD WINAPI Water::UpdateWaterThread( void *_self )
{
	Water *self = (Water *) _self;

	do 
	{
		self->UpdateDynamicWater();
	} 
	while( !self->m_terminating.Wait( 1000.0/120.0 ) );

	return 0;
}
#endif

void Water::Advance()
{
	if( //!g_app->m_editing && 
		RENDER_WATER_DETAIL > 0 &&
		m_renderVerts.Size() > 0 
#ifdef USE_DIRECT3D
		&& OpenGLD3D::g_supportsHwVertexProcessing 
#endif
		)
	{
		START_PROFILE(  "Advance Water" );

#ifdef USE_DIRECT3D
		AllocateVertexBuffersIfNecessary();
#endif

		UpdateDynamicWater();

		END_PROFILE(  "Advance Water" );
	}
}

#ifdef USE_DIRECT3D
void Water::AllocateVertexBuffersIfNecessary()
{
	bool needToAlloc = false;

	for( int i = 0; i < NumVertexBuffers; i++ )
	{
		if (m_vertexBuffer[i] == NULL)
		{
			needToAlloc = true;
			break;
		}
	}

	if (!needToAlloc)
		return;

	NetLockMutex l(m_vertexBufferLock);

	unsigned bufferSize = m_renderVerts.Size() * sizeof(WaterVertex);

	for( int i = 0; i < NumVertexBuffers; i++ )
	{
		if (m_vertexBuffer[i] == NULL) {
			HRESULT hr = OpenGLD3D::g_pd3dDevice->CreateVertexBuffer(
				bufferSize, 
				D3DUSAGE_DYNAMIC|D3DUSAGE_WRITEONLY|(OpenGLD3D::g_supportsHwVertexProcessing?0:D3DUSAGE_SOFTWAREPROCESSING),
				0,
				D3DPOOL_DEFAULT,
				&m_vertexBuffer[i],
				NULL);

			AppDebugAssert( hr != D3DERR_INVALIDCALL );
			AppDebugAssert( hr != D3DERR_OUTOFVIDEOMEMORY );
			AppDebugAssert( hr != E_OUTOFMEMORY );
			AppDebugAssert( hr == D3D_OK );
			AppDebugAssert( m_vertexBuffer != NULL );

			m_vertexBufferReady[i] = false;
		}
	}
}
#endif
