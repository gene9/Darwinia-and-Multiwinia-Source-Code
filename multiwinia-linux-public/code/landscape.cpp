#include "lib/universal_include.h"

#include <float.h>
#include <math.h>

#include "lib/filesys/binary_stream_readers.h"
#include "lib/bitmap.h"
#include "lib/debug_render.h"
#include "lib/debug_utils.h"
#include "lib/hi_res_time.h"
#include "lib/math_utils.h"
#include "lib/preferences.h"
#include "lib/profiler.h"
#include "lib/resource.h"
#include "lib/rgb_colour.h"
#include "lib/vector2.h"
#include "lib/vector3.h"
#include "lib/math/random_number.h"

#include "app.h"
#include "camera.h"
#include "global_world.h"
#include "landscape.h"
#include "landscape_renderer.h"
#include "level_file.h"
#include "renderer.h"
#include "location.h"
#include "water.h"


// ****************************************************************************
// Class LandscapeTile
// ****************************************************************************

// *** Constructor
// Called from Landscape constructor
LandscapeTile::LandscapeTile()
:	m_outsideHeight(0.0),
	m_size(256),
	m_heightMap(NULL),
    m_guideGridPower(0),
    m_guideGrid(NULL),
	m_generationMethod(1),
	m_desiredHeight(200),
    m_fractalDimension(1.2),
    m_heightScale(1.0),
    m_lowlandSmoothingFactor(1.0),
    m_randomSeed(1),
	m_holdrand(1L)
{
}


// *** Destructor
LandscapeTile::~LandscapeTile()
{
	delete m_heightMap;
	delete m_guideGrid;
}


// *** GetPowerOfTwo
int LandscapeTile::GetPowerOfTwo(int x)
{
    return (int)ceil(Log2(x));
}


// *** GuideGridSetPower
void LandscapeTile::GuideGridSetPower(int _power)
{
    if( _power != m_guideGridPower )
    {
		int resolution = (1 << _power) - 1;
		int a = GetPowerOfTwo(resolution + 1);
		int b = GetPowerOfTwo(resolution + 2);
		AppDebugAssert(a != b);

        delete m_guideGrid;
		m_guideGrid = NULL;
        m_guideGridPower = _power;

		if (resolution != 0)
        {
            m_guideGrid = new Array2D <unsigned char> (resolution, resolution, 0);
            m_guideGrid->SetAll(0);
        }
    }    
}


// *** GuideGridGetPower
int LandscapeTile::GuideGridGetPower()
{
    return m_guideGridPower;
}


// *** GuideGridToString
char *LandscapeTile::GuideGridToString()
{
    static char *result = NULL;
    if( result )
    {
        delete result;
        result = NULL;
    }

	int res = m_guideGrid->GetNumColumns();
    if( res > 0 )
    {
        result = new char[ res * res * 2 + 1 ];
        char *target = result;

        for( int z = 0; z < res; ++z )
        {
            for( int x = 0; x < res; ++x )
            {
                unsigned char actualVal = m_guideGrid->GetData(x,z);
               
                *target = 'A' + int( actualVal / 16 );
                *(target+1) = 'A' + actualVal - int( actualVal / 16 ) * 16;

                target+=2;
            }
        }

        *target = '\x0';
    }

    return result;
}


// *** GuideGridFromString
void LandscapeTile::GuideGridFromString( char *_hex)
{
    GuideGridSetPower( m_guideGridPower );
    char *current = _hex;

	int res = m_guideGrid->GetNumColumns();
    for( int z = 0; z < res; ++z )
    {
        for( int x = 0; x < res; ++x )
        {
            int valA = *current - 'A';
            int valB = *(current+1) - 'A';

            int actualVal = valA * 16 + valB;

            m_guideGrid->PutData( x, z, actualVal );

            current+=2;
        }
    }
}


// *** GenerateNoise
double LandscapeTile::GenerateNoise(double _halfSize, double _height)
{
	double length = 256.0 * _halfSize / m_heightMap->GetNumColumns();
	double val = Landscapesfrand(iv_pow(length * 10.0, m_fractalDimension));
	val *= m_compensatedHeightScale;
	val *= 0.1 + (double)iv_pow(fabsf(_height), m_lowlandSmoothingFactor) * 0.15;
	return val;
}


// *** GenerateDiamondMidpoint
void LandscapeTile::GenerateDiamondMidpoint(int _x, int _z, int _halfSize)
{
	double newHeight;
	
	switch (m_generationMethod)
	{
		case 0:
			newHeight = m_heightMap->GetData(_x - _halfSize, _z) + 
						m_heightMap->GetData(_x + _halfSize, _z) +
						m_heightMap->GetData(_x, _z - _halfSize) + 
						m_heightMap->GetData(_x, _z + _halfSize);
			newHeight *= 0.25;
			break;
		case 1:
			if (LandscapeAppRandom() & 1)
			{
				newHeight = m_heightMap->GetData(_x - _halfSize, _z) + 
							m_heightMap->GetData(_x + _halfSize, _z);
			}
			else
			{
				newHeight = m_heightMap->GetData(_x, _z + _halfSize) + 
							m_heightMap->GetData(_x, _z - _halfSize);
			}
			newHeight *= 0.5;
			break;
		case 2:
		{
			int rnd = LandscapeAppRandom() & 0xffff;
			if (rnd < 0x3fff)			newHeight = m_heightMap->GetData(_x - _halfSize, _z);
			else if (rnd < 0x7fff)		newHeight = m_heightMap->GetData(_x + _halfSize, _z);
			else if (rnd < 0xbfff)		newHeight = m_heightMap->GetData(_x, _z + _halfSize);
			else						newHeight = m_heightMap->GetData(_x, _z - _halfSize);
			break;
		}
	}

	newHeight += GenerateNoise((double)_halfSize, newHeight);
	m_heightMap->PutData(_x, _z, newHeight);
}


// *** GenerateSquareMidpoint
void LandscapeTile::GenerateSquareMidpoint(int _x, int _z, int _halfSize)
{
	double newHeight;
	
	switch (m_generationMethod)
	{
		case 0:
			newHeight = m_heightMap->GetData(_x - _halfSize, _z - _halfSize) + 
						m_heightMap->GetData(_x - _halfSize, _z + _halfSize) +
						m_heightMap->GetData(_x + _halfSize, _z - _halfSize) + 
						m_heightMap->GetData(_x + _halfSize, _z + _halfSize);
			newHeight *= 0.25;
			break;
		case 1:
			if (LandscapeAppRandom() & 1)
			{
				newHeight = m_heightMap->GetData(_x - _halfSize, _z - _halfSize) + 
							m_heightMap->GetData(_x + _halfSize, _z + _halfSize);
			}
			else
			{
				newHeight = m_heightMap->GetData(_x - _halfSize, _z + _halfSize) + 
							m_heightMap->GetData(_x + _halfSize, _z - _halfSize);
			}
			newHeight *= 0.5;
			break;
		case 2:
		{
			int rnd = LandscapeAppRandom() & 0xffff;
			if (rnd < 0x3fff)			newHeight = m_heightMap->GetData(_x - _halfSize, _z - _halfSize);
			else if (rnd < 0x7fff)		newHeight = m_heightMap->GetData(_x - _halfSize, _z + _halfSize);
			else if (rnd < 0xbfff)		newHeight = m_heightMap->GetData(_x + _halfSize, _z - _halfSize);
			else						newHeight = m_heightMap->GetData(_x + _halfSize, _z + _halfSize);
			break;
		}
	}

	newHeight += GenerateNoise((double)_halfSize, newHeight);
	m_heightMap->PutData(_x, _z, newHeight);
}


// *** GenerateMidpoints
void LandscapeTile::GenerateMidpoints(int _x1, int _z1, int _x2, int _z2)
{
	int halfSize = (_x2 - _x1) >> 1;
	int midX = _x1 + halfSize;
	int midZ = _z1 + halfSize;
	
	// Create a new point in the centre of the parent square
	GenerateSquareMidpoint(midX, midZ, halfSize);

	// Create a new point at the mid point of the top edge of the parent square
	if (_z1 != 0) 
	{
		GenerateDiamondMidpoint(midX, _z1, halfSize);
	}

	// Create a new point at the mid point of the left edge of the parent square
	if (_x1 != 0)
	{
		GenerateDiamondMidpoint(_x1, midZ, halfSize);
	}
}


// *** Generate
void LandscapeTile::Generate(LandscapeDef *_def)
{
    // Do we need to throw away our height map?
	{
		int reqWorldCells = m_size / _def->m_cellSize;
		int reqCells = GetPowerOfTwo(reqWorldCells - 1);
		reqCells = iv_pow(2, reqCells) + 1;
		if( !m_heightMap ||
			reqCells != m_heightMap->GetNumColumns() ||
			reqCells != m_heightMap->GetNumRows() )
		{
			delete m_heightMap;
			m_heightMap = new SurfaceMap2D <double>(reqCells, reqCells, 
										   0.0, 0.0, 1.0, 1.0, // Cell size of 1.0 is correct here
										   m_outsideHeight);
		}
	}

	// Initialise value of all height samples
	m_heightMap->SetAll(m_outsideHeight);

	// Populate the top level height samples with data from the guide grid
	if( m_guideGrid )
    {
		int res = m_guideGrid->GetNumColumns();
		int scale = (m_heightMap->GetNumColumns()-1) / (res + 1);
        for (unsigned short z = 0; z < res; ++z)
        {
            for (unsigned short x = 0; x < res; ++x)
            {
				double a = m_guideGrid->GetData(x, z);
                m_heightMap->PutData((x + 1) * scale, (z + 1) * scale, a);
            }
        }
    }

	// Generate the terrain
	{
		double fracDimModifier = 30.7 * iv_exp(-6.5 * m_fractalDimension);
		double smoothingModifier = 15.353 * iv_exp(-3.1 * m_lowlandSmoothingFactor);
		m_compensatedHeightScale = m_heightScale;
		m_compensatedHeightScale *= fracDimModifier;
		m_compensatedHeightScale *= smoothingModifier;

		int numIterationsToSkip = m_guideGridPower;
		int numIterations = GetPowerOfTwo(m_heightMap->GetNumColumns()) - numIterationsToSkip;
		int stepSize = ((m_heightMap->GetNumColumns()) - 1 ) >> numIterationsToSkip;
		
		LandscapeAppSeedRandom(m_randomSeed);

		for (int i = 0; i < numIterations; ++i)
		{
			for (int x = 0; x < m_heightMap->GetNumColumns() - 1; x += stepSize)
			{
				for (int z = 0; z < m_heightMap->GetNumColumns() - 1; z += stepSize)
				{
					GenerateMidpoints(x, z, x + stepSize, z + stepSize);
				}
			} 
			stepSize >>= 1;
		}
	}

	// Apply a height shift to the whole tile
	for (unsigned short x = 0; x < m_heightMap->GetNumColumns(); ++x)
	{
		for (unsigned short z = 0; z < m_heightMap->GetNumColumns(); ++z)
		{
			m_heightMap->PutData(x, z, m_heightMap->GetData(x, z) + m_posY);
		}
	}
}

void LandscapeTile::LandscapeAppSeedRandom (unsigned int seed)
{
    m_holdrand = (long) seed;
}

int LandscapeTile::LandscapeAppRandom ()
{
    return (((m_holdrand = m_holdrand * 214013L + 2531011L) >> 16) & 0x7fff);
}

double LandscapeTile::Landscapefrand(double range)
{ 
    return range * ((double)LandscapeAppRandom() / (double)APP_RAND_MAX); 
}


double LandscapeTile::Landscapesfrand(double range)
{
    return (0.5 - (double)LandscapeAppRandom() / (double)(APP_RAND_MAX)) * range; 
}


// ****************************************************************************
// Class Landscape
// ****************************************************************************

void Landscape::FlattenArea(LandscapeFlattenArea const *_area)
{
	int x1 = m_heightMap->GetMapIndexX(_area->m_centre.x - _area->m_size) + 1;
	int x2 = m_heightMap->GetMapIndexX(_area->m_centre.x + _area->m_size) + 1;
	int z1 = m_heightMap->GetMapIndexY(_area->m_centre.z - _area->m_size) + 1;
	int z2 = m_heightMap->GetMapIndexY(_area->m_centre.z + _area->m_size) + 1;

    bool loop = false;

    if( _area->m_flattenType == LandscapeFlattenArea::FlattenTypeSmooth ) 
    {
        loop = true;
    }

    while( true )
    {
	    for (int z = z1; z < z2; ++z)
	    {
		    for (int x = x1; x < x2; ++x)
		    {
                double height = _area->m_centre.y;
                if( _area->m_flattenType == LandscapeFlattenArea::FlattenTypeAdd )
                {
                    height += m_heightMap->GetData( x, z );
                }
                else if( _area->m_flattenType == LandscapeFlattenArea::FlattenTypeSubtract )
                {
                    if( m_heightMap->GetData( x, z ) > _area->m_heightThreshold )
                    {
                        height = m_heightMap->GetData( x, z ) - height;
                    }
                    else
                    {
                        height = m_heightMap->GetData( x, z );
                    }

                }
                else if( _area->m_flattenType == LandscapeFlattenArea::FlattenTypeSubtract2 )
                {
                    if( m_heightMap->GetData( x, z ) < _area->m_heightThreshold )
                    {
                        height = m_heightMap->GetData( x, z ) - height;
                    }
                    else
                    {
                        height = m_heightMap->GetData( x, z );
                    }

                }
                else if( _area->m_flattenType == LandscapeFlattenArea::FlattenTypeSmooth )
                {
                    double thisHeight = m_heightMap->GetData( x, z );
                    if( thisHeight > _area->m_heightThreshold )
                    {
                        double scale = height / thisHeight;
                        height = thisHeight * scale;
                     
                        height += m_heightMap->GetData( x - 1, z );
                        height += m_heightMap->GetData( x + 1, z );
                        height += m_heightMap->GetData( x, z + 1 );
                        height += m_heightMap->GetData( x, z - 1 );
                        height /= 6.0;

                        

                        /*if( loop )
                        {
                            double scale = _area->m_centre.y / m_heightMap->GetData( x, z );
                            height *= scale;
                        }*/
                    }
                    if( height > thisHeight ) height = thisHeight;
                }
			    m_heightMap->PutData(x, z, height);
		    }
	    }

        if( !loop ) break;
        loop = false;
    }
}


// *** MergeTileIntoLandscape
void Landscape::MergeTileIntoLandscape(LandscapeTile const *_tile)
{
	unsigned short posX = (double)(_tile->m_posX / m_heightMap->m_cellSizeX) + 0.5;
	int posY = _tile->m_posY;
	unsigned short posZ = (double)(_tile->m_posZ / m_heightMap->m_cellSizeY) + 0.5;

    // Calculate num cells that this tile will occupy in the main landscape
	int numCells = (double)_tile->m_size / m_heightMap->m_cellSizeX;
    
    // Calculate how much to increment in "tile space" for one unit in "main landscape space"
    double factor = (double)(_tile->m_heightMap->GetNumColumns() - 1) / (double)(numCells - 1);
	
	double heightRange = _tile->m_heightMap->GetHighestValue() - _tile->m_outsideHeight;
	heightRange += 0.001; // Prevent divide by zero below
	double heightFactor = (_tile->m_desiredHeight - _tile->m_outsideHeight) / heightRange;

	_tile->m_heightMap->m_x0 = _tile->m_posX;
	_tile->m_heightMap->m_y0 = _tile->m_posZ;
	_tile->m_heightMap->m_cellSizeX = _tile->m_size / (double)_tile->m_heightMap->GetNumColumns();
	_tile->m_heightMap->m_cellSizeY = _tile->m_size / (double)_tile->m_heightMap->GetNumRows();
	_tile->m_heightMap->m_invCellSizeX = 1.0 / _tile->m_heightMap->m_cellSizeX;
	_tile->m_heightMap->m_invCellSizeY = 1.0 / _tile->m_heightMap->m_cellSizeY;

	for (unsigned short z = 0; z < numCells; ++z)
	{
//        double tileZ = posZ + (double)z * m_heightMap->m_cellSizeY;
        double tileZ = m_heightMap->GetRealY((double)(z + posZ));

        for (unsigned short x = 0; x < numCells; ++x)
		{
//			double tileX = posX + (double)x * m_heightMap->m_cellSizeX;
			double tileX = m_heightMap->GetRealX((double)(x + posX));
			double height1 = _tile->m_heightMap->GetValue(tileX, tileZ);
			height1 -= _tile->m_outsideHeight;
			height1 *= heightFactor;
			height1 += _tile->m_outsideHeight;
//			double height1 = _tile->m_heightMap->GetData(tileX, tileZ) * heightFactor;
			double height2 = m_heightMap->GetData(x + posX, z + posZ);
			if (height1 > height2)
			{
				m_heightMap->PutData(x + posX, z + posZ, height1);
			}
   		}
	}
}


// *** GenerateHeightMap
void Landscape::GenerateHeightMap(LandscapeDef *_def)
{
	// Initialise value of all height samples
	m_heightMap->SetAll(m_outsideHeight);

	// Join the tiles together to form the whole level
	for (int i = 0; i < _def->m_tiles.Size(); ++i)
	{
		LandscapeTile *aTile = _def->m_tiles.GetData(i);
        aTile->m_outsideHeight = m_outsideHeight;
        aTile->Generate(_def);
		MergeTileIntoLandscape(aTile);
	}

	// Apply flatten areas
	{
		LList<LandscapeFlattenArea *> *areasList = &_def->m_flattenAreas;
		for(int i = 0; i < areasList->Size(); ++i)
		{
			FlattenArea(areasList->GetData(i));
		}
	}

	m_worldSizeX = m_heightMap->m_cellSizeX * m_heightMap->GetNumColumns();
	m_worldSizeZ = m_heightMap->m_cellSizeY * m_heightMap->GetNumRows();
}


void Landscape::DeleteTile( int tileId )
{
    LandscapeTile *tile = g_app->m_location->m_levelFile->m_landscape.m_tiles[ tileId ];
    delete tile;
    g_app->m_location->m_levelFile->m_landscape.m_tiles.RemoveData( tileId );
	LandscapeDef *def = &g_app->m_location->m_levelFile->m_landscape;
    Init(def);
}


// Generate normals that will be used for hit check, not rendering
void Landscape::GenerateNormals()
{
	// Generate the normals from the height samples
    
    for (unsigned short z = 0; z < m_heightMap->GetNumRows(); ++z)
    {
		int zTimesNumCells = z * m_heightMap->GetNumColumns();
        for (unsigned short x = 0; x < m_heightMap->GetNumColumns(); ++x)
        {
            double heightN = m_heightMap->GetData(x, z - 1);
            double heightW = m_heightMap->GetData(x + 1, z);
            double heightE = m_heightMap->GetData(x - 1, z);
            double heightS = m_heightMap->GetData(x, z + 1);
	
			if (heightN == heightW &&
				heightE == heightS &&
				heightN == heightE)
			{
				m_normalMap->PutData(x, z, g_upVector);
				continue;
			}
            
			double heightCentre = m_heightMap->GetData(x, z);

            Vector3 vectN ( 0.0, heightCentre - heightN, m_heightMap->m_cellSizeY );
            Vector3 vectW ( -m_heightMap->m_cellSizeX, heightCentre - heightW, 0.0 );
            Vector3 vectS ( 0.0, heightCentre - heightS, -m_heightMap->m_cellSizeY );
            Vector3 vectE ( m_heightMap->m_cellSizeX, heightCentre - heightE, 0.0 );

            Vector3 normA = (vectW ^ vectN).Normalise();
            Vector3 normB = (vectE ^ vectS).Normalise();
            Vector3 norm = (normA + normB).Normalise();

			m_normalMap->PutData(x, z, norm);
        }
    }
}


void Landscape::RenderHitNormals() const
{
	glColor3ub(255,90,90);
	glLineWidth(1.0);
	glBegin(GL_LINES);
		for (int z = 0; z < m_heightMap->GetNumRows(); z++)
		{
			for (int x = 0; x < m_heightMap->GetNumColumns(); x++)
			{
				Vector3 pos(x * m_heightMap->m_cellSizeX, m_heightMap->GetData(x, z), z * m_heightMap->m_cellSizeX);
				glVertex3dv(pos.GetData());
				pos += m_normalMap->GetData(x, z) * 20.0;
				glVertex3dv(pos.GetData());
			}
		}
	glEnd();
}


// ******************
//  Public Functions
// ******************

// *** Constructor
// This one creates a
Landscape::Landscape()
:	m_heightMap(NULL),
	m_normalMap(NULL),
    m_outsideHeight(-20),
	m_renderer(NULL),
    m_artificalMaxHeight(0.0f)
{
}


// *** Destructor
Landscape::~Landscape()
{
	Empty();
}


void Landscape::BuildOpenGlState()
{
	delete m_renderer;
	m_renderer = new LandscapeRenderer(m_heightMap);
}


// *** Init
void Landscape::Init(LandscapeDef *_def, bool _justMakeTheHeightMap)
{
	double t = GetHighResTime();
    //
    // Bring in values from the landscape definition

	m_outsideHeight = _def->m_outsideHeight;

    int detailScaleFactor = 1;//g_prefsManager->GetInt( "RenderLandscapeDetail", 1 );
    double oldCellSize = _def->m_cellSize;
    
    _def->m_cellSize *= ( 1.0 + (detailScaleFactor-1) * 0.5 );
    
	if (!m_heightMap ||
		m_heightMap->m_cellSizeX != _def->m_cellSize ||
		m_heightMap->m_cellSizeY != _def->m_cellSize ||
		m_worldSizeX != _def->m_worldSizeX ||
		m_worldSizeZ != _def->m_worldSizeZ )
	{
		delete m_heightMap;
		delete m_normalMap;
		m_heightMap = new SurfaceMap2D <double>(_def->m_worldSizeX, _def->m_worldSizeZ, 
											   0.0, 0.0, _def->m_cellSize, _def->m_cellSize,
											   m_outsideHeight);
		m_normalMap = new SurfaceMap2D <Vector3>(_def->m_worldSizeX, _def->m_worldSizeZ,
												 0.0, 0.0, _def->m_cellSize, _def->m_cellSize,
												 g_upVector);
	}

		
	//
    // Generate our landscape from those tiles

    GenerateHeightMap(_def);
    _def->m_cellSize = oldCellSize;

	AppDebugOut("Generated Heightmap in %f seconds\n", GetHighResTime()-t); t= GetHighResTime();
	if (_justMakeTheHeightMap)
	{
		return;
	}

    m_artificalMaxHeight = _def->m_maxHeight;   // this is only used for altering the colour map, and isn't required for any other calculations

    GenerateNormals();
    BuildOpenGlState();

	if( g_app->m_location->m_water )
	{
	    g_app->m_location->m_water->GenerateLightMap();
	}


	AppDebugOut("Generated OpenGL setup in %f seconds\n", GetHighResTime()-t); t= GetHighResTime();
}


// *** Empty
void Landscape::Empty()
{
	delete m_renderer;			m_renderer = NULL;
	delete m_heightMap;			m_heightMap = NULL;
	delete m_normalMap;			m_normalMap = NULL;
}

static int g_iDelay = 0;

// *** Render
void Landscape::Render()
{   
	m_renderer->Render();        
}


// *** GetWorldSizeX
double Landscape::GetWorldSizeX() const
{
	if (!m_heightMap)
	{
		return 1e6;
	}
	return m_worldSizeX;
}


// *** GetWorldSizeZ
double Landscape::GetWorldSizeZ() const
{
	if (!m_heightMap)
	{
		return 1e6;
	}
	return m_worldSizeZ;
}


bool Landscape::IsInLandscape( Vector3 const &_pos )
{
    return( _pos.x >= 0.0 && _pos.z >= 0.0 &&
            _pos.x < GetWorldSizeX() &&
            _pos.z < GetWorldSizeZ() );
}


// *** RayHit
// Does not assume that rayDir is normalised
// Warning: I think that it could hit triangle behind rayStart,
// not only triangles in front of rayStart.
bool Landscape::RayHit(Vector3 const &_rayStart, Vector3 const &_rayDir, Vector3 *_result) const
{
	if (!m_heightMap) return false;
	
	Vector3 rayDirNormalised = _rayDir;
	rayDirNormalised.Normalise();

	// Find out which cell the ray starts in
	int x0 = m_heightMap->GetMapIndexX(_rayStart.x);
	int z0 = m_heightMap->GetMapIndexY(_rayStart.z);

    // If the ray starts inside the landscape grid, then we are OK. Otherwise, we
    // we need to clip the ray starting point against the landscape grid.
    if (x0 >=0 && x0 < m_heightMap->GetNumColumns() && z0 >= 0 && z0 <= m_heightMap->GetNumRows())
    {
        return UnsafeRayHit(_rayStart, rayDirNormalised, _result);
    }

	// *** Find out which quadrant the 2D projection of the ray goes in ***

    Vector2 segIntersectResult;
	if (rayDirNormalised.x > 0.0)
	{
		if (rayDirNormalised.z > 0.0)
		{
            // Ray is going North-East

            // Find if it ever intersects the landscape grid. The ray will intersect
            // the landscape grid if and only if it intersect either the bottom edge
            // or the left hand edge of the grid
            Vector2 segStart(0.0, 0.0);
            Vector2 segEnd(GetWorldSizeX(), 0.0);
            bool intersects = SegRayIntersection2D(segStart, segEnd, _rayStart, rayDirNormalised, &segIntersectResult);
            if (!intersects) 
            {
                Vector2 segStart(0.0, 0.0);
                Vector2 segEnd(0.0, GetWorldSizeZ());
                intersects = SegRayIntersection2D(segStart, segEnd, _rayStart, rayDirNormalised, &segIntersectResult);
                if (!intersects)
                {
                    return false;
                }
            }
        }
		else
		{
            // Ray is going south east

            Vector2 segStart(0.0, GetWorldSizeZ());
            Vector2 segEnd(GetWorldSizeX(), GetWorldSizeZ());
            bool intersects = SegRayIntersection2D(segStart, segEnd, _rayStart, rayDirNormalised, &segIntersectResult);
            if (!intersects) 
            {
                Vector2 segStart(0.0, 0.0);
                Vector2 segEnd(0.0, GetWorldSizeZ());
                intersects = SegRayIntersection2D(segStart, segEnd, _rayStart, rayDirNormalised, &segIntersectResult);
                if (!intersects)
                {
                    return false;
                }
            }
        }
	}
	else
	{
		if (rayDirNormalised.z > 0.0)
		{
            // Ray is going north west

            Vector2 segStart(0.0, 0.0);
            Vector2 segEnd(GetWorldSizeX(), 0.0);
            bool intersects = SegRayIntersection2D(segStart, segEnd, _rayStart, rayDirNormalised, &segIntersectResult);
            if (!intersects) 
            {
                Vector2 segStart(GetWorldSizeX(), 0.0);
                Vector2 segEnd(GetWorldSizeX(), GetWorldSizeZ());
                intersects = SegRayIntersection2D(segStart, segEnd, _rayStart, rayDirNormalised, &segIntersectResult);
                if (!intersects)
                {
                    return false;
                }
            }
		}
		else
		{
            // Ray is going south west

            Vector2 segStart(0.0, GetWorldSizeZ());
            Vector2 segEnd(GetWorldSizeX(), GetWorldSizeZ());
            bool intersects = SegRayIntersection2D(segStart, segEnd, _rayStart, rayDirNormalised, &segIntersectResult);
            if (!intersects) 
            {
                Vector2 segStart(GetWorldSizeX(), 0.0);
                Vector2 segEnd(GetWorldSizeX(), GetWorldSizeZ());
                intersects = SegRayIntersection2D(segStart, segEnd, _rayStart, rayDirNormalised, &segIntersectResult);
                if (!intersects)
                {
                    return false;
                }
            }
		}
	}

    x0 = m_heightMap->GetMapIndexX(segIntersectResult.x);
    z0 = m_heightMap->GetMapIndexY(segIntersectResult.y);

    double lambda = (segIntersectResult.x - _rayStart.x) / rayDirNormalised.x;
    double segIntersectResultY = _rayStart.y + lambda * rayDirNormalised.y;

    return UnsafeRayHit(Vector3(segIntersectResult.x, segIntersectResultY, segIntersectResult.y),
						rayDirNormalised, _result);
}


// *** UnsafeRayHit
// Determines if and where a ray intersects the landscape
// ASSUMES: that the specified ray starts inside the landscape grid
// ASSUMES: that _rayDir is normalised
// ASSUMES: that _result is a pointer to a valid Vector3
bool Landscape::UnsafeRayHit(Vector3 const &_rayStart, Vector3 const &_rayDir, Vector3 *_result) const
{
	int gridX = m_heightMap->GetMapIndexX(_rayStart.x);
	int gridZ = m_heightMap->GetMapIndexY(_rayStart.z);

    // Find out which quadrant the ray goes in
	if (_rayDir.x > 0.0 && _rayDir.z > 0.0)
	{
		// Ray goes in North-East quadrant

		// Now step along the line, going either north or east each step, until
		// we step out of the landscape grid, or we find a hit.
		while (gridX < m_heightMap->GetNumColumns() && gridZ < m_heightMap->GetNumRows())
		{
            if (RayHitCell(gridX, gridZ, _rayStart, _rayDir, _result))
            {
                return true;
            }

            Vector3 segStart(m_heightMap->GetRealX(gridX), 0.0, m_heightMap->GetRealY(gridZ+1));
            Vector3 segEnd(m_heightMap->GetRealX(gridX+1), 0.0, m_heightMap->GetRealY(gridZ+1));
            bool intersects = SegRayIntersection2D(segStart, segEnd, _rayStart, _rayDir);
            if (intersects) 
            {
                gridZ++;
            }
            else
            {
                gridX++;
            }
		}
	}
	else if (_rayDir.x < 0.0 && _rayDir.z > 0.0)
	{
		// Ray goes in North-West quadrant

		// Now step along the line, going either north or west each step, until
		// we step out of the landscape grid, or we find a hit.
		while (gridX >= 0 && gridZ < m_heightMap->GetNumRows())
		{
            if (RayHitCell(gridX, gridZ, _rayStart, _rayDir, _result))
            {
                return true;
            }

            Vector3 segStart(m_heightMap->GetRealX(gridX), 0.0, m_heightMap->GetRealY(gridZ+1));
            Vector3 segEnd(m_heightMap->GetRealX(gridX+1), 0.0, m_heightMap->GetRealY(gridZ+1));
            bool intersects = SegRayIntersection2D(segStart, segEnd, _rayStart, _rayDir);
            if (intersects) 
            {
                gridZ++;
            }
            else
            {
                gridX--;
            }
		}
	}
	else if (_rayDir.x < 0.0 && _rayDir.z < 0.0)
	{
		// Ray goes in South-West quadrant
		
		// Now step along the line, going either south or west each step, until
		// we step out of the landscape grid, or we find a hit.
		while (gridX >= 0 && gridZ >= 0)
		{
            if (RayHitCell(gridX, gridZ, _rayStart, _rayDir, _result))
            {
                return true;
            }

            Vector3 segStart(m_heightMap->GetRealX(gridX), 0.0, m_heightMap->GetRealY(gridZ));
            Vector3 segEnd(m_heightMap->GetRealX(gridX+1), 0.0, m_heightMap->GetRealY(gridZ));
            bool intersects = SegRayIntersection2D(segStart, segEnd, _rayStart, _rayDir);
            if (intersects) 
            {
                gridZ--;
            }
            else
            {
                gridX--;
            }
		}
    }
	else
	{
		// Ray goes in South-East quadrant
		
		// Now step along the line, going either south or east each step, until
		// we step out of the landscape grid, or we find a hit.
		while (gridX < m_heightMap->GetNumColumns() && gridZ >= 0)
		{
            if (RayHitCell(gridX, gridZ, _rayStart, _rayDir, _result))
            {
                return true;
            }

            Vector3 segStart(m_heightMap->GetRealX(gridX), 0.0, m_heightMap->GetRealY(gridZ));
            Vector3 segEnd(m_heightMap->GetRealX(gridX+1), 0.0, m_heightMap->GetRealY(gridZ));
            bool intersects = SegRayIntersection2D(segStart, segEnd, _rayStart, _rayDir);
            if (intersects) 
            {
                gridZ--;
            }
            else
            {
                gridX++;
            }
		}
    }

	return false;
}


// *** RayHitCell
// Determines if the specified ray hits the specified landscape cell (this can be thought
// of as a cube with the top left corner being at the heightMap point x0,z0).
// Assumes that _rayDir is normalised and that ray intersects cell in the 2D projection
bool Landscape::RayHitCell(int x0, int z0, 
                           Vector3 const &_rayStart, Vector3 const &_rayDir,
                           Vector3 *_result) const
{
	Vector3 corner1(m_heightMap->GetRealX(x0), m_heightMap->GetData(x0, z0), m_heightMap->GetRealY(z0));
	Vector3 corner2(m_heightMap->GetRealX(x0), m_heightMap->GetData(x0, z0+1), m_heightMap->GetRealY(z0+1));
	Vector3 corner3(m_heightMap->GetRealX(x0+1), m_heightMap->GetData(x0+1, z0), m_heightMap->GetRealY(z0));

	if (RayTriIntersection(_rayStart, _rayDir, corner1, corner2, corner3, 1e10, _result))
	{
		return true;
	}

	Vector3 corner4(m_heightMap->GetRealX(x0+1), m_heightMap->GetData(x0+1, z0+1), m_heightMap->GetRealY(z0+1));
	if (RayTriIntersection(_rayStart, _rayDir, corner2, corner3, corner4, 1e10, _result))
	{
		return true;
	}

	return false;

/*	double height00 = m_heightMap->GetData(x0, z0);
	double height01 = m_heightMap->GetData(x0, z0 + 1);
	double height10 = m_heightMap->GetData(x0 + 1, z0);
	double height11 = m_heightMap->GetData(x0 + 1, z0 + 1);

	double maxHeightOfCube = max(height00, height01);
	maxHeightOfCube = max(height10, maxHeightOfCube);
	maxHeightOfCube = max(height11, maxHeightOfCube);

	double minHeightOfCube = min(height00, height01);
	minHeightOfCube = min(height10, minHeightOfCube);
	minHeightOfCube = min(height11, minHeightOfCube);

	double worldXForX0 = m_heightMap->GetRealX(x0);
	double lambdaWhenRayCrossesX0 = (worldXForX0 - _rayStart.x) / _rayDir.x;

	double worldXForX1 = m_heightMap->GetRealX(x0 + 1);
	double lambdaWhenRayCrossesX1 = (worldXForX1 - _rayStart.x) / _rayDir.x;

	double worldZForZ0 = m_heightMap->GetRealY(z0);
	double lambdaWhenRayCrossesZ0 = (worldZForZ0 - _rayStart.z) / _rayDir.z;

	double worldZForZ1 = m_heightMap->GetRealY(z0 + 1);
	double lambdaWhenRayCrossesZ1 = (worldZForZ1 - _rayStart.z) / _rayDir.z;

	double maxLambda = max(lambdaWhenRayCrossesX0, lambdaWhenRayCrossesX1);
	maxLambda = max(lambdaWhenRayCrossesZ0, maxLambda);
	maxLambda = max(lambdaWhenRayCrossesZ1, maxLambda);

	double minLambda = min(lambdaWhenRayCrossesX0, lambdaWhenRayCrossesX1);
	minLambda = min(lambdaWhenRayCrossesZ0, minLambda);
	minLambda = min(lambdaWhenRayCrossesZ1, minLambda);

	double heightAtMinLambda = _rayStart.y + minLambda * _rayDir.y;
	double heightAtMaxLambda = _rayStart.y + maxLambda * _rayDir.y;

	if (heightAtMinLambda > maxHeightOfCube && heightAtMaxLambda > maxHeightOfCube)
	{
		return false;
	}
	
	if (heightAtMinLambda < minHeightOfCube && heightAtMaxLambda < minHeightOfCube)
	{
		return false;
	}

    _result->x = m_heightMap->GetRealX(x0);
    _result->y = (maxHeightOfCube + minHeightOfCube) * 0.5;
    _result->z = m_heightMap->GetRealY(z0);

	return true;*/
}


// Returns the distance to the nearest point on the landscape if it is
// within the specified radius. Otherwise returns -1.0.
double Landscape::SphereHit(Vector3 const &_centre, double _radius) const
{
	// Make sure the specified radius is +ve and not so large to cause
	// major efficiency problems
	AppDebugAssert(_radius > 0.0 && _radius < 200.0);

	int x1 = m_heightMap->GetMapIndexX(_centre.x - _radius);
	int x2 = m_heightMap->GetMapIndexX(_centre.x + _radius);
	int y1 = m_heightMap->GetMapIndexY(_centre.z - _radius);
	int y2 = m_heightMap->GetMapIndexY(_centre.z + _radius);

	clamp(x1, 0, m_heightMap->GetNumColumns());
	clamp(x2, 0, m_heightMap->GetNumColumns());
	clamp(y1, 0, m_heightMap->GetNumRows());
	clamp(y2, 0, m_heightMap->GetNumRows());

	double nearestSqrd = FLT_MAX;
	Vector3 pos;
	for (int y = y1; y < y2; ++y)
	{
		pos.z = m_heightMap->GetRealY(y);
		
		for (int x = x1; x < x2; ++x)
		{
			pos.x = m_heightMap->GetRealX(x);
			pos.y = m_heightMap->GetData(x, y);
			double distSqrd = (_centre - pos).MagSquared();
			if (distSqrd < nearestSqrd)
			{
				nearestSqrd = distSqrd;
			}
		}
	}

	double dist = iv_sqrt(nearestSqrd);
	if (dist > _radius)
	{
		dist = -1.0; 
	}

	return dist;
}

