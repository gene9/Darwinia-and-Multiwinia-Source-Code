#ifndef INCLUDED_LANDSCAPE_H
#define INCLUDED_LANDSCAPE_H

#include "lib/2d_array.h"
#include "lib/2d_surface_map.h"

class Vector3;
class BitmapRGBA;
class RGBAColour;
class LandscapeFlattenArea;
class Landscape;
class LandscapeDef;
class LandscapeRenderer;


// ****************************************************************************
// Class LandscapeTile
// ****************************************************************************

class LandscapeTile 
{
public:
    //          OUR DEFINITION DATA
    double		m_fractalDimension;             
	double		m_heightScale;		    
	double		m_desiredHeight;
	int			m_generationMethod;
	int			m_randomSeed;
	double		m_lowlandSmoothingFactor; 
	int			m_posX;					// In world space
	double		m_posY;
	int			m_posZ;
	double		m_outsideHeight;
    int         m_guideGridPower;		// Log to the base 2 of the resolution of the guide grid
    int			m_size;		            // Size when copied into the main landscape,
	                                    // (obviously in world space)
    
    Array2D		<unsigned char> *m_guideGrid;

    //          OUR GENERATED DATA
	SurfaceMap2D <double> *m_heightMap;			// Width and height of this must be equal and must be a power of 2 + 1
	double		m_compensatedHeightScale;

	long		m_holdrand;

protected:
    int         GetPowerOfTwo			(int x);
	double       GenerateNoise			(double _halfSize, double _height);
	void        GenerateDiamondMidpoint	(int _x, int _z, int _halfSize);
	void        GenerateSquareMidpoint	(int _x, int _z, int _halfSize);
	void        GenerateMidpoints		(int _x1, int _x2, int _z1, int _z2);

public:
	LandscapeTile();
	~LandscapeTile();

    void        GuideGridSetPower		(int _power);
    int         GuideGridGetPower		();
    char        *GuideGridToString		();
    void        GuideGridFromString		(char *_hex);

	void		Generate				(LandscapeDef *_def);
	void		LandscapeAppSeedRandom (unsigned int seed);
	int			LandscapeAppRandom ();
	double		Landscapefrand(double range);
	double		Landscapesfrand(double range);

};

#define LIGHTMAP_SCALEFACTOR 1



// ****************************************************************************
// Class Landscape
// ****************************************************************************

class Landscape
{
friend class LocationEditor;
public:
    SurfaceMap2D		<double>			*m_heightMap;
	SurfaceMap2D		<Vector3>		*m_normalMap;
    double				m_outsideHeight;    
	LandscapeRenderer	*m_renderer;
    float               m_artificalMaxHeight;

private:
	double		m_worldSizeX;				// Updated in GenerateHeightMap
	double		m_worldSizeZ;				// Updated in GenerateHeightMap

	void		MergeTileIntoLandscape(LandscapeTile const *_tile);
	void        GenerateHeightMap	(LandscapeDef *_def);
    void        GenerateNormals();

	void		FlattenArea			(LandscapeFlattenArea const *_area);

	void		RenderHitNormals	() const;

	bool        UnsafeRayHit		(Vector3 const &_rayStart, Vector3 const &_rayEnd, Vector3 *_result) const;

public:
	Landscape();
	Landscape(double _cellSize, int universeSizeX, int universeSizeZ);
	~Landscape();
   
	void		BuildOpenGlState	();

    void		Init				(LandscapeDef *_def, bool _justMakeTheHeightMap = false);
	void		Empty				();
    
	void		Render				();

    void        DeleteTile			( int tileId );

	double		GetWorldSizeX		() const;
	double		GetWorldSizeZ		() const;
    bool        IsInLandscape       ( Vector3 const &_pos );

	bool		RayHit(Vector3 const &_rayStart, Vector3 const &_rayDir, Vector3 *_result) const;
	bool		RayHitCell(int x0, int z0, Vector3 const &_rayStart, Vector3 const &_rayDir, Vector3 *_result) const;
	double		SphereHit(Vector3 const &_centre, double _radius) const;

};



#endif
