#ifndef INCLUDED_SHAPE_H
#define INCLUDED_SHAPE_H

#include <stdio.h>

#include "lib/llist.h"
#include "lib/matrix34.h"
#include "lib/rgb_colour.h"
#include "lib/vector3.h"


class TextReader;
class ShapeFragment;
class Matrix34;
class Shape;


// ****************
// Class RayPackage
// ****************

class RayPackage
{
public:
	Vector3 m_rayStart;
	Vector3 m_rayEnd;
	Vector3 m_rayDir;
    float   m_rayLen;

	RayPackage(Vector3 const &_start, Vector3 const &_dir, float _length = 1e10)
	:	m_rayStart(_start),
		m_rayDir(_dir)
	{
        m_rayLen = _length;
		m_rayEnd = m_rayStart + m_rayDir * _length;
	}
};


// *******************
// Class SpherePackage
// *******************

class SpherePackage
{
public:
    Vector3 m_pos;
    float   m_radius;

    SpherePackage(Vector3 const &_pos, float _radius)
    :   m_pos(_pos),
        m_radius(_radius)
    {
    }
};


// *****************
// Class ShapeMarker
// *****************

class ShapeMarker
{
public:
	Matrix34		m_transform;
	char		   *m_name;
	char		   *m_parentName;
	int				m_depth;		// Number of levels in the shape fragment tree from root to self
	ShapeFragment **m_parents;		// An array with m_depth elements

	ShapeMarker(char const *_name, char *_parentName, int _depth, Matrix34 const &_transform);
  	ShapeMarker(TextReader *_in, char const *_name);
	~ShapeMarker();

	Matrix34 GetWorldMatrix(Matrix34 const &_rootTransform);

    void WriteToFile(FILE *_out) const;
};


// ******************
// Class VertexPosCol
// ******************

class VertexPosCol
{
public:
	unsigned short m_posId;
	unsigned short m_colId;
};


// *******************
// Class ShapeTriangle
// *******************

class ShapeTriangle
{
public:
	unsigned short v1, v2, v3;	// Vertex indices (into ShapeFragment::m_vertices)
};


// *******************
// Class ShapeFragment
// *******************

class ShapeFragment
{
friend class ShapeExporter;
protected:
	char			*m_displayListName;

	void ParsePositionBlock		(TextReader *in, unsigned int numPositions);
	void ParseNormalBlock		(TextReader *in, unsigned int numNorms);
	void ParseColourBlock		(TextReader *in, unsigned int numColours);
	void ParseVertexBlock		(TextReader *in, unsigned int numVerts);
	void ParseStripBlock		(TextReader *in);
	void ParseAllStripBlocks	(TextReader *in, unsigned int numStrips);
	void ParseTriangleBlock		(TextReader *in, unsigned int numTriangles);

	void GenerateNormals		();

public:
	unsigned int	m_numPositions;
	Vector3			*m_positions;
	Vector3			*m_positionsInWS;	// Temp storage space used to cache World Space versions of all the vertex positions in the hit check routines
	unsigned int	m_numNormals;
	Vector3			*m_normals;
	unsigned int	m_numColours;
	RGBAColour		*m_colours;
	unsigned int	m_numVertices;		// Each element contains an index into m_positions and an index into m_colours
	VertexPosCol	*m_vertices;
	unsigned int	m_numTriangles;
	unsigned int	m_maxTriangles;
	ShapeTriangle	*m_triangles;

	char			*m_name;
	char			*m_parentName;
	Matrix34		m_transform;
	Vector3			m_angVel;
	Vector3			m_vel;

	bool			m_useCylinder;	// If true then use cylinder hit check instead of sphere
    Vector3			m_centre;
	float			m_radius;
	float			m_mostPositiveY;
	float			m_mostNegativeY;

	LList<ShapeFragment *>	m_childFragments;
	LList<ShapeMarker *>	m_childMarkers;

    ShapeFragment				(TextReader *_in, char const *_name);
    ShapeFragment				(char const *_name, char const *_parentName);
    ~ShapeFragment				();

	void BuildDisplayList		();

	void RegisterPositions		(Vector3 *positions, unsigned int numPositions);
    void RegisterNormals		(Vector3 *norms, unsigned int numNorms);
    void RegisterColours		(RGBAColour *colours, unsigned int numColours);
    void RegisterVertices		(VertexPosCol *verts, unsigned int numVerts);
	void RegisterTriangles		(ShapeTriangle *tris, unsigned int numTris);

    void WriteToFile			(FILE *_out) const;

	void Render					(float _predictionTime);// Uses display list
	void RenderSlow				();						// Doesn't use display list
	void RenderHitCheck			(Matrix34 const &_transform);
	void RenderMarkers			(Matrix34 const &_transform);

	ShapeFragment *LookupFragment(char const *_name);	// Recurses into child fragments
	ShapeMarker   *LookupMarker  (char const *_name);	// Recurses into child fragments

    void CalculateCentre        ( Matrix34 const &_transform, Vector3 &_centre, int &_numFragments );       // Recursive
    void CalculateRadius        ( Matrix34 const &_transform, Vector3 const &_centre, float &_radius );     // Recursive

	bool RayHit					(RayPackage *_package, Matrix34 const &_transform, bool _accurate = false);
    bool SphereHit              (SpherePackage *_package, Matrix34 const &_transform, bool _accurate = false);
    bool ShapeHit               (Shape *_shape, Matrix34 const &_theTransform,              // Transform of _shape
                                                Matrix34 const &_ourTransform,              // Transform of this
								 bool _accurate = false);

	void Recolour				( RGBAColour _teamColour );
};


// ***********
// Class Shape
// ***********

class Shape
{
protected:
	void Load					(TextReader *_in);
	char						*m_displayListName;


public:
	bool						m_animating;		// If false then whole shape in one display list otherwise one display list per fragment

	ShapeFragment				*m_rootFragment;
	char						*m_name;

	Shape						();
    Shape						(char const *filename, bool _animating);
    Shape						(TextReader *in, bool _animating);
	~Shape						();

	void						BuildDisplayList();

    void WriteToFile			(FILE *_out) const;

	void Render					(float _predictionTime, Matrix34 const &_transform);
	void RenderHitCheck			(Matrix34 const &_transform);
	void RenderMarkers			(Matrix34 const &_transform);

	bool RayHit					(RayPackage *_package, Matrix34 const &_transform, bool _accurate = false);
    bool SphereHit              (SpherePackage *_package, Matrix34 const &_transform, bool _accurate = false);
    bool ShapeHit               (Shape *_shape, Matrix34 const &_theTransform,              // Transform of _shape
                                                Matrix34 const &_ourTransform,              // Transform of this
								 bool _accurate = false);

    Vector3 CalculateCentre     ( Matrix34 const &_transform );
    float   CalculateRadius     ( Matrix34 const &_transform, Vector3 const &_centre );

	bool isTeamColoured;
	void						Recolour ( RGBAColour _teamColour );
};

#endif
