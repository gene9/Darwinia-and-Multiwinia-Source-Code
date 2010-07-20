#include "lib/universal_include.h"

#include <float.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "lib/debug_utils.h"
#include "lib/debug_render.h"
#include "lib/math_utils.h"
#include "lib/matrix33.h"
#include "lib/matrix34.h"
#include "lib/shape.h"
#include "lib/text_stream_readers.h"

#ifndef EXPORTER_BUILD
#include "lib/resource.h"
#include "../app.h"
#endif

#define USE_DISPLAY_LISTS

// ****************************************************************************
// Class ShapeMarker
// ****************************************************************************

// *** Constructor
// This constructor is used in the export process. The m_parents array is never
// populated in the exporter, so it is intentionally left blank
ShapeMarker::ShapeMarker(char const *_name, char *_parentName, int _depth, Matrix34 const &_transform)
:	m_parents(NULL),
	m_depth(_depth),
	m_transform(_transform)
{
	m_name = strdup(_name);
	m_parentName = strdup(_parentName);

	// Remove spaces from m_name
	char *c = m_name;
	char *d = m_name;
	while(d[0] != '\0')
	{
		c[0] = d[0];
		if (c[0] != ' ')
		{
			c++;
		}
		d++;
	}
	c[0] = '\0';

	// Remove spaces from m_parentName
	c = m_parentName;
	d = m_parentName;
	while(d[0] != '\0')
	{
		c[0] = d[0];
		if (c[0] != ' ')
		{
			c++;
		}
		d++;
	}
	c[0] = '\0';
}


// *** Constructor
ShapeMarker::ShapeMarker(TextReader *_in, char const *_name)
{
	m_name = strdup(_name);
	m_parentName = NULL;
	while(_in->ReadLine())
	{
		char *firstWord = _in->GetNextToken();

		if (firstWord)
		{
			if (stricmp(firstWord, "ParentName") == 0)
			{
				char *secondWord = _in->GetNextToken();
				m_parentName = strdup(secondWord);
			}
			else if (stricmp(firstWord, "Depth") == 0)
			{
				char *secondWord = _in->GetNextToken();
				m_depth = atoi(secondWord);
			}
			else if (stricmp(firstWord, "front") == 0)
			{
				char *secondWord = _in->GetNextToken();
				m_transform.f.x = (float)atof(secondWord);
				m_transform.f.y = (float)atof(_in->GetNextToken());
				m_transform.f.z = (float)atof(_in->GetNextToken());
			}
			else if (stricmp(firstWord, "up") == 0)
			{
				char *secondWord = _in->GetNextToken();
				m_transform.u.x = (float)atof(secondWord);
				m_transform.u.y = (float)atof(_in->GetNextToken());
				m_transform.u.z = (float)atof(_in->GetNextToken());
			}
			else if (stricmp(firstWord, "pos") == 0)
			{
				char *secondWord = _in->GetNextToken();
				m_transform.pos.x = (float)atof(secondWord);
				m_transform.pos.y = (float)atof(_in->GetNextToken());
				m_transform.pos.z = (float)atof(_in->GetNextToken());
			}
			else if (stricmp(firstWord, "MarkerEnd") == 0)
			{
				break;
			}
		}
	}

    m_transform.Normalise();

	m_parents = new ShapeFragment*[m_depth];

	if (!m_name)		m_name = strdup("unknown");
	if (!m_parentName)	m_parentName = strdup("unknown");
}

ShapeMarker::~ShapeMarker()
{
	SAFE_FREE(m_parentName);
	SAFE_FREE(m_name);
	//for(unsigned i=0;i<m_depth;i++) delete m_parents[i]; should we?
	SAFE_DELETE(m_parents);
}

// *** GetWorldMatrix
Matrix34 ShapeMarker::GetWorldMatrix(Matrix34 const &_rootTransform)
{
	Matrix34 mat = _rootTransform;
	for (int i = 0; i < m_depth; ++i)
	{
		mat = m_parents[i]->m_transform * mat;
	}
	mat = m_transform * mat;

	return mat;
}


void ShapeMarker::WriteToFile(FILE *_out) const
{
	fprintf(_out, "Marker: %s\n", m_name);
	fprintf(_out, "\tParentName: %s\n", m_parentName);
	fprintf(_out, "\tDepth: %d\n", m_depth);
	fprintf(_out, "\tUp:    %5.2f %5.2f %5.2f\n", m_transform.u.x, m_transform.u.y, m_transform.u.z);
	fprintf(_out, "\tFront: %5.2f %5.2f %5.2f\n", m_transform.f.x, m_transform.f.y, m_transform.f.z);
	fprintf(_out, "\tPos:   %5.2f %5.2f %5.2f\n", m_transform.pos.x, m_transform.pos.y, m_transform.pos.z);
	fprintf(_out, "\tMarkerEnd\n\n\n");
}



// ****************************************************************************
// Class ShapeFragment
// ****************************************************************************

// This constructor is used to load a shape from a file.
ShapeFragment::ShapeFragment(TextReader *_in, char const *_name)
:	m_displayListName(NULL),
	m_numPositions(0),
	m_positions(NULL),
	m_positionsInWS(NULL),
	m_numNormals(0),
	m_normals(NULL),
	m_numColours(0),
	m_colours(NULL),
	m_numVertices(0),
	m_vertices(NULL),
	m_numTriangles(0),
	m_maxTriangles(0),
	m_triangles(NULL),
	m_name(NULL),
	m_parentName(NULL),
	m_transform(1),
	m_angVel(0,0,0),
	m_vel(0,0,0),
	m_useCylinder(false),
	m_centre(0.0f, 0.0f, 0.0f),
	m_radius(-1.0f),
	m_mostPositiveY(0.0f),
	m_mostNegativeY(0.0f)
{
	m_maxTriangles = 1;
	m_triangles = new ShapeTriangle[m_maxTriangles];

	DarwiniaDebugAssert(_name);
	m_name = strdup(_name);

	m_transform.SetToIdentity();
	m_angVel.Zero();
	m_vel.Zero();

	while(_in->ReadLine())
	{
		if (!_in->TokenAvailable()) continue;

		char *firstWord = _in->GetNextToken();
		char *secondWord = _in->GetNextToken();

		if (stricmp(firstWord, "ParentName") == 0)
		{
			m_parentName = strdup(secondWord);
		}
		else if (stricmp(firstWord, "front") == 0)
		{
			m_transform.f.x = (float)atof(secondWord);
			m_transform.f.y = (float)atof(_in->GetNextToken());
			m_transform.f.z = (float)atof(_in->GetNextToken());
		}
		else if (stricmp(firstWord, "up") == 0)
		{
			m_transform.u.x = (float)atof(secondWord);
			m_transform.u.y = (float)atof(_in->GetNextToken());
			m_transform.u.z = (float)atof(_in->GetNextToken());
		}
		else if (stricmp(firstWord, "pos") == 0)
		{
			m_transform.pos.x = (float)atof(secondWord);
			m_transform.pos.y = (float)atof(_in->GetNextToken());
			m_transform.pos.z = (float)atof(_in->GetNextToken());
		}
		else if (stricmp(firstWord, "Positions") == 0)
		{
			int numPositions = atoi(secondWord);
			ParsePositionBlock(_in, numPositions);
		}
		else if (stricmp(firstWord, "Normals") == 0)
		{
			int numNorms = atoi(secondWord);
			ParseNormalBlock(_in, numNorms);
		}
		else if (stricmp(firstWord, "Colours") == 0)
		{
			int numColours = atoi(secondWord);
			ParseColourBlock(_in, numColours);
		}
		else if (stricmp(firstWord, "Vertices") == 0)
		{
			int numVerts = atoi(secondWord);
			ParseVertexBlock(_in, numVerts);
		}
		else if (stricmp(firstWord, "Strips") == 0)
		{
			int numStrips = atoi(secondWord);
			ParseAllStripBlocks(_in, numStrips);
			break;
		}
		else if (stricmp(firstWord, "Triangles") == 0)
		{
			int numTriangles = atoi(secondWord);
			ParseTriangleBlock(_in, numTriangles);
			break;
		}
	}

    m_transform.Normalise();

	if (!m_name)		m_name = strdup("unknown");
	if (!m_parentName)	m_parentName = strdup("unknown");

	GenerateNormals();

	m_positionsInWS = new Vector3[m_numVertices];
}


// This constructor is used when you want to build a shape from scratch yourself,
// eg in the exporter.
ShapeFragment::ShapeFragment(char const *_name, char const *_parentName)
:	m_displayListName(NULL),
	m_numPositions(0),
	m_positions(NULL),
	m_numNormals(0),
	m_normals(NULL),
	m_numColours(0),
	m_colours(NULL),
	m_numVertices(0),
	m_vertices(NULL),
	m_positionsInWS(NULL),
	m_numTriangles(0),
	m_maxTriangles(0),
	m_triangles(NULL),
	m_useCylinder(false),
	m_centre(0.0f, 0.0f, 0.0f),
	m_radius(-1.0f),
	m_mostPositiveY(0.0f),
	m_mostNegativeY(0.0f)
{
	m_maxTriangles = 1;
	m_triangles = new ShapeTriangle[m_maxTriangles];

	m_transform.SetToIdentity();
	m_angVel.Zero();
	m_vel.Zero();
	m_name = strdup(_name);
	m_parentName = strdup(_parentName);

	// Remove spaces from m_name
	char *c = m_name;
	char *d = m_name;
	while(d[0] != '\0')
	{
		c[0] = d[0];
		if (c[0] != ' ')
		{
			c++;
		}
		d++;
	}
	c[0] = '\0';

	// Remove spaces from m_parentName
	c = m_parentName;
	d = m_parentName;
	while(d[0] != '\0')
	{
		c[0] = d[0];
		if (c[0] != ' ')
		{
			c++;
		}
		d++;
	}
	c[0] = '\0';
}


ShapeFragment::~ShapeFragment()
{
	SAFE_DELETE_ARRAY(m_positions);
	SAFE_DELETE_ARRAY(m_positionsInWS);
	free(m_name);				m_name = NULL;
	free(m_parentName);			m_parentName = NULL;
    delete [] m_vertices;		m_vertices = NULL;
    delete [] m_normals;		m_normals = NULL;
    delete [] m_colours;		m_colours = NULL;
    delete [] m_triangles;		m_triangles = NULL;
	m_childFragments.EmptyAndDelete();
	m_childMarkers.EmptyAndDelete();
#ifndef EXPORTER_BUILD
	g_app->m_resource->DeleteDisplayList(m_displayListName);
	delete [] m_displayListName;
	m_displayListName = NULL;
#endif
}


void ShapeFragment::BuildDisplayList()
{
#ifndef EXPORTER_BUILD
	DarwiniaDebugAssert(m_displayListName == NULL);
	m_displayListName = g_app->m_resource->GenerateName();
	int id = g_app->m_resource->CreateDisplayList(m_displayListName);
	glNewList(id, GL_COMPILE);
		RenderSlow();
	glEndList();
#endif
}


void ShapeFragment::WriteToFile(FILE *_out) const
{
    int i;

	if (stricmp(m_name, "SceneRoot") != 0)
	{
		fprintf(_out, "Fragment: %s\n", m_name);
		fprintf(_out, "\tParentName: %s\n", m_parentName);
		fprintf(_out, "\tup:    %5.2f %5.2f %5.2f\n", m_transform.u.x, m_transform.u.y, m_transform.u.z);
		fprintf(_out, "\tfront: %5.2f %5.2f %5.2f\n", m_transform.f.x, m_transform.f.y, m_transform.f.z);
		fprintf(_out, "\tpos: %.2f %.2f %.2f\n", m_transform.pos.x, m_transform.pos.y, m_transform.pos.z);

		// Write out the positions
		fprintf(_out, "\tPositions: %d\n", m_numPositions);
		for (i = 0; i < m_numPositions; i++)
		{
			Vector3 const &v = m_positions[i];
			fprintf(_out, "\t\t%d: %7.3f %7.3f %7.3f\n", i, v.x, v.y, v.z);
		}

		// Write out the normals
		fprintf(_out, "\tNormals: %d\n", m_numNormals);
		for (i = 0; i < m_numNormals; i++)
		{
			Vector3 const &n = m_normals[i];
			fprintf(_out, "\t\t%d: %6.3f %6.3f %6.3f\n", i, n.x, n.y, n.z);
		}

		// Write out the colours
		fprintf(_out, "\tColours: %d\n", m_numColours);
		for (i = 0; i < m_numColours; i++)
		{
			RGBAColour const &c = m_colours[i];
			fprintf(_out, "\t\t%d: %d %d %d\n", i, c.r, c.g, c.b);
		}

		// Write out the vertices
		fprintf(_out, "\tVertices: %d    # Position ID then Colour ID\n", m_numVertices);
		for (i = 0; i < m_numVertices; i++)
		{
			VertexPosCol const &v = m_vertices[i];
			fprintf(_out, "\t\t%d: %3d %3d\n", i, v.m_posId, v.m_colId);
		}

		// Write out the triangles
		fprintf(_out, "\tTriangles: %d\n", m_numTriangles);
		for (i = 0; i < m_numTriangles; ++i)
		{
			fprintf(_out, "\t\t%d,%d,%d\n",
				m_triangles[i].v1,
				m_triangles[i].v2,
				m_triangles[i].v3);
		}

		fprintf(_out, "\n");
	}

	// Write out all the child fragments
	for (i = 0; i < m_childFragments.Size(); ++i)
	{
		m_childFragments.GetData(i)->WriteToFile(_out);
	}

	// Write out all the child markers
	for (i = 0; i < m_childMarkers.Size(); ++i)
	{
		m_childMarkers.GetData(i)->WriteToFile(_out);
	}
}


// *** ParsePositionBlock
void ShapeFragment::ParsePositionBlock(TextReader *_in, unsigned int _numPositions)
{
	Vector3 *positions = new Vector3[_numPositions];

	int expectedId = 0;
	while (expectedId < _numPositions)
	{
		if (_in->ReadLine() == 0)
		{
			DarwiniaDebugAssert(0);
		}

		char *c = _in->GetNextToken();
		if (c && isdigit(c[0]))
		{
			int id = atoi(c);
			if (id != expectedId || id >= _numPositions)
			{
				return;
			}

			Vector3 *vect = &positions[id];
			c = _in->GetNextToken();
			DarwiniaDebugAssert(c);
			vect->x = (float)atof(c);
			c = _in->GetNextToken();
			DarwiniaDebugAssert(c);
			vect->y = (float)atof(c);
			c = _in->GetNextToken();
			DarwiniaDebugAssert(c);
			vect->z = (float)atof(c);

			expectedId++;
		}
	}

	RegisterPositions(positions, _numPositions);
}


// *** ParseNormalBlock
void ShapeFragment::ParseNormalBlock(TextReader *_in, unsigned int _numNorms)
{
	if (_numNorms != 0)
	{
		m_normals = new Vector3[_numNorms];
	}
	m_numNormals = _numNorms;

	int expectedId = 0;
	while(expectedId < _numNorms)
	{
		if (_in->ReadLine() == 0)
		{
			DarwiniaDebugAssert(0);
		}

		char *c = _in->GetNextToken();
		if (c && isdigit(c[0]))
		{
			int id = atoi(c);
			if (id != expectedId || id >= _numNorms)
			{
				DarwiniaDebugAssert(0);
			}

			Vector3 *vect = &m_normals[id];
			c = _in->GetNextToken();
			DarwiniaDebugAssert(c);
			vect->x = (float)atof(c);
			c = _in->GetNextToken();
			DarwiniaDebugAssert(c);
			vect->y = (float)atof(c);
			c = _in->GetNextToken();
			DarwiniaDebugAssert(c);
			vect->z = (float)atof(c);

			expectedId++;
		}
	}
}


// *** ParseColourBlock
void ShapeFragment::ParseColourBlock(TextReader *_in, unsigned int _numColours)
{
	m_colours = new RGBAColour[_numColours];
	m_numColours = _numColours;

	int expectedId = 0;
	while(expectedId < _numColours)
	{
		if (_in->ReadLine() == 0)
		{
			DarwiniaDebugAssert(0);
		}

		char *c = _in->GetNextToken();
		if (c && isdigit(c[0]))
		{
			int id = atoi(c);
			if (id != expectedId || id >= _numColours)
			{
				DarwiniaDebugAssert(0);
			}

			RGBAColour *col = &m_colours[id];
			c = _in->GetNextToken();
			DarwiniaDebugAssert(c);
			col->r = atoi(c);
			c = _in->GetNextToken();
			DarwiniaDebugAssert(c);
			col->g = atoi(c);
			c = _in->GetNextToken();
			DarwiniaDebugAssert(c);
			col->b = atoi(c);
			col->a = 0;

			*col *= 0.85f;
			col->a = 255;

			expectedId++;
		}
	}
}


// *** ParseVertexBlock
void ShapeFragment::ParseVertexBlock(TextReader *_in, unsigned int _numVerts)
{
	m_vertices = new VertexPosCol[_numVerts];
	m_numVertices = _numVerts;

	int expectedId = 0;
	while (expectedId < _numVerts)
	{
		if (_in->ReadLine() == 0)
		{
			DarwiniaDebugAssert(0);
		}

		char *c = _in->GetNextToken();
		if (c && isdigit(c[0]))
		{
			int id = atoi(c);
			if (id != expectedId || id >= _numVerts)
			{
				DarwiniaDebugAssert(0);
			}

			VertexPosCol *vert = &m_vertices[id];
			c = _in->GetNextToken();
			vert->m_posId = atoi(c);
			c = _in->GetNextToken();
			vert->m_colId = atoi(c);

			expectedId++;
		}
	}
}


// *** ParseStripBlock
void ShapeFragment::ParseStripBlock(TextReader *_in)
{
	_in->ReadLine();
	char *c = _in->GetNextToken();
	DarwiniaDebugAssert(c);

	// Read material name
	if (stricmp(c, "Material") == 0)
	{
		c = _in->GetNextToken();
		DarwiniaDebugAssert(c);

		_in->ReadLine();
		c = _in->GetNextToken();
	}

	// Read number of vertices in strip
	DarwiniaDebugAssert( c && (stricmp(c, "Verts") == 0) );
	c = _in->GetNextToken();
	DarwiniaDebugAssert(c);
	int numVerts = atoi(c);
	DarwiniaDebugAssert(numVerts > 2);

	// Now just read a sequence of verts
	int i = 0;
	int v1 = -1, v2 = -1;
	while(i < numVerts)
	{
		if (_in->ReadLine() == 0)
		{
			DarwiniaDebugAssert(0);
		}

		while (_in->TokenAvailable())
		{
			char *c = _in->GetNextToken();
			DarwiniaDebugAssert( c[0] == 'v' );

			c++;
			int v3 = atoi(c);
			DarwiniaDebugAssert(v3 < m_numVertices);

			if (i >= 2 && v1 != v2 && v2 != v3 && v1 != v3)
			{
				if (m_numTriangles == m_maxTriangles)
				{
					ShapeTriangle *newTriangles = new ShapeTriangle[m_maxTriangles * 2];
					memcpy(newTriangles, m_triangles, sizeof(ShapeTriangle) * m_maxTriangles);
					delete [] m_triangles;
					m_triangles = newTriangles;
					m_maxTriangles *= 2;
				}
				if (i & 1)
				{
					m_triangles[m_numTriangles].v1 = v3;
					m_triangles[m_numTriangles].v2 = v2;
					m_triangles[m_numTriangles].v3 = v1;
				}
				else
				{
					m_triangles[m_numTriangles].v1 = v1;
					m_triangles[m_numTriangles].v2 = v2;
					m_triangles[m_numTriangles].v3 = v3;
				}
				m_numTriangles++;
			}

			v1 = v2;
			v2 = v3;

			i++;
		}
	}
}


// *** ParseAllStripBlocks
void ShapeFragment::ParseAllStripBlocks(TextReader *_in, unsigned int _numStrips)
{
	int expectedId = 0;
	while(_in->ReadLine())
	{
		char *c = _in->GetNextToken();
		if (c && stricmp(c, "Strip") == 0)
		{
			c = _in->GetNextToken();
			int id = atoi(c);

			DarwiniaDebugAssert(id == expectedId);

			ParseStripBlock(_in);

			expectedId++;

			if (expectedId == _numStrips)
			{
				break;
			}
		}
	}

	// Shrink m_triangles to tightly fit its data
	ShapeTriangle *newTriangles = new ShapeTriangle[m_numTriangles];
	memcpy(newTriangles, m_triangles, sizeof(ShapeTriangle) * m_numTriangles);
	delete [] m_triangles;
	m_maxTriangles = m_numTriangles;
	m_triangles = newTriangles;
}


void ShapeFragment::ParseTriangleBlock(TextReader *_in, unsigned int _numTriangles)
{
	DarwiniaDebugAssert(m_numTriangles == 0 && m_maxTriangles == 1 && m_triangles != NULL);
	delete [] m_triangles;

	m_maxTriangles = _numTriangles;
	m_triangles = new ShapeTriangle[_numTriangles];

	while (m_numTriangles < _numTriangles)
	{
		_in->ReadLine();
		char *c = _in->GetNextToken();
		m_triangles[m_numTriangles].v1 = atoi(c);
		c = _in->GetNextToken();
		m_triangles[m_numTriangles].v2 = atoi(c);
		c = _in->GetNextToken();
		m_triangles[m_numTriangles].v3 = atoi(c);

		m_numTriangles++;
	}
}


// *** GenerateNormals
// Currently this function generates one normal for each face in all the
// strips, rather than one normal per vertex. This is what we need for
// facet shading, rather than smooth (gouraud) shading.
void ShapeFragment::GenerateNormals()
{
	m_numNormals = m_numTriangles;
	m_normals = new Vector3[m_numNormals];
	int normId = 0;

	for (int j = 0; j < m_numTriangles; ++j)
	{
		ShapeTriangle *tri = &m_triangles[j];
		VertexPosCol const &vertA = m_vertices[tri->v1];
		VertexPosCol const &vertB = m_vertices[tri->v2];
		VertexPosCol const &vertC = m_vertices[tri->v3];
		Vector3 &a = m_positions[vertA.m_posId];
		Vector3 &b = m_positions[vertB.m_posId];
		Vector3 &c = m_positions[vertC.m_posId];
		Vector3 ab = b - a;
		Vector3 bc = c - b;
		m_normals[normId] = ab ^ bc;
		m_normals[normId].Normalise();
//		if (!(j & 1)) m_normals[normId] = -m_normals[normId];
		normId++;
	}
}


// *** RegisterPositions
void ShapeFragment::RegisterPositions(Vector3 *_positions, unsigned int _numPositions)
{
	int i;

    delete [] m_positions;
    m_positions = _positions;
    m_numPositions = _numPositions;

	// Calculate bounding sphere
	{
		// Find the centre of the fragment
		{
			float minX = FLT_MAX;
			float maxX = -FLT_MAX;
			float minY = FLT_MAX;
			float maxY = -FLT_MAX;
			float minZ = FLT_MAX;
			float maxZ = -FLT_MAX;
			for (i = 0; i < m_numPositions; ++i)
			{
				if (m_positions[i].x < minX)		minX = m_positions[i].x;
				if (m_positions[i].x > maxX)		maxX = m_positions[i].x;
				if (m_positions[i].y < minY)		minY = m_positions[i].y;
				if (m_positions[i].y > maxY)		maxY = m_positions[i].y;
				if (m_positions[i].z < minZ)		minZ = m_positions[i].z;
				if (m_positions[i].z > maxZ)		maxZ = m_positions[i].z;
			}
			m_centre.x = (maxX + minX) / 2;
			m_centre.y = (maxY + minY) / 2;
			m_centre.z = (maxZ + minZ) / 2;
		}

		// Find the point furthest from the centre
		float radiusSquared = 0.0f;
		for (i = 0; i < m_numPositions; ++i)
		{
			Vector3 delta = m_centre - m_positions[i];
			float magSquared = delta.MagSquared();
			if (magSquared > radiusSquared)
			{
				radiusSquared = magSquared;
			}
		}
		m_radius = sqrtf(radiusSquared);
	}

	// Calculate bounding vertical cylinder
	{
		// Find the vertical extents of the fragment and minimum enclosing
		// cylinder radius
		float radiusSquared = 0.0f;
		m_mostPositiveY = -FLT_MAX;
		m_mostNegativeY = FLT_MAX;
		for (i = 0; i < m_numPositions; ++i)
		{
			if (m_positions[i].y > m_mostPositiveY)
			{
				m_mostPositiveY = m_positions[i].y;
			}
			if (m_positions[i].y < m_mostNegativeY)
			{
				m_mostNegativeY = m_positions[i].y;
			}
			Vector3 delta = m_centre - m_positions[i];
			delta.y = 0.0f;
			float magSquared = delta.MagSquared();
			if (magSquared > radiusSquared)
			{
				radiusSquared = magSquared;
			}
		}
		float radius = sqrtf(radiusSquared);
		float height = m_mostPositiveY - m_mostNegativeY;
		float cylinderVolume = M_PI * radiusSquared * height;
		float sphereVolume = 4.0f/3.0f * M_PI * m_radius * m_radius * m_radius;

		if (cylinderVolume < sphereVolume)
		{
//			m_radius = radius;
//			m_useCylinder = true;
		}
	}
}


void ShapeFragment::RegisterNormals(Vector3 *_norms, unsigned int _numNorms)
{
    delete [] m_normals;
    m_normals = _norms;
    m_numNormals = _numNorms;
}


void ShapeFragment::RegisterColours(RGBAColour *_colours, unsigned int _numColours)
{
    delete [] m_colours;
    m_colours = _colours;
    m_numColours = _numColours;
}


void ShapeFragment::RegisterVertices(VertexPosCol *_verts, unsigned int _numVerts)
{
	delete [] m_vertices;
	m_vertices = _verts;
	m_numVertices = _numVerts;
}


void ShapeFragment::RegisterTriangles(ShapeTriangle *_tris, unsigned int _numTris)
{
	delete [] m_triangles;
	m_triangles = _tris;
	m_numTriangles = _numTris;
}


void ShapeFragment::Render(float _predictionTime)
{
#ifndef EXPORTER_BUILD
	Matrix34 predictedTransform = m_transform;
	predictedTransform.RotateAround(m_angVel * _predictionTime);
	predictedTransform.pos += m_vel * _predictionTime;

	bool matrixIsIdentity = predictedTransform == g_identityMatrix34;
	if (!matrixIsIdentity)
	{
		glPushMatrix();
		glMultMatrixf(predictedTransform.ConvertToOpenGLFormat());
	}

#ifdef USE_DISPLAY_LISTS
	int id = -1;
	if (m_displayListName) id = g_app->m_resource->GetDisplayList(m_displayListName);
	if (id != -1)
	{
		glCallList(id);
	}
	else
#endif
	{
		RenderSlow();
	}

    int numChildren = m_childFragments.Size();
	for (int i = 0; i < numChildren; ++i)
	{
    	m_childFragments.GetData(i)->Render(_predictionTime);
	}

	if (!matrixIsIdentity)
	{
		glPopMatrix();
	}
#endif
}


void ShapeFragment::RenderSlow()
{
#ifndef EXPORTER_BUILD
	if(!m_numTriangles) return;
	glBegin(GL_TRIANGLES);

	int norm = 0;
	for (int i = 0; i < m_numTriangles; i++)
	{
		VertexPosCol const *vertA = &m_vertices[m_triangles[i].v1];
		VertexPosCol const *vertB = &m_vertices[m_triangles[i].v2];
		VertexPosCol const *vertC = &m_vertices[m_triangles[i].v3];

        unsigned char const alpha = 255;

		/*/ calculate normal
		float u[3],v[3],n[3],l;
		u[0]=m_positions[vertB->m_posId].x-m_positions[vertA->m_posId].x;
		u[1]=m_positions[vertB->m_posId].y-m_positions[vertA->m_posId].y;
		u[2]=m_positions[vertB->m_posId].z-m_positions[vertA->m_posId].z;
		v[0]=m_positions[vertC->m_posId].x-m_positions[vertA->m_posId].x;
		v[1]=m_positions[vertC->m_posId].y-m_positions[vertA->m_posId].y;
		v[2]=m_positions[vertC->m_posId].z-m_positions[vertA->m_posId].z;
		n[0]=u[1]*v[2]-u[2]*v[1];
		n[1]=-u[0]*v[2]+u[2]*v[0];
		n[2]=u[0]*v[1]-u[1]*v[0];
		l=(float)sqrt(n[0]*n[0]+n[1]*n[1]+n[2]*n[2]);
		n[0]/=l; n[1]/=l; n[2]/=l;
		glNormal3fv(n);*/

		glNormal3fv(m_normals[norm].GetData());
		glColor4ub (m_colours[vertA->m_colId].r,
                    m_colours[vertA->m_colId].g,
                    m_colours[vertA->m_colId].b,
                    alpha );
		glVertex3fv (m_positions[vertA->m_posId].GetData());

		glNormal3fv(m_normals[norm].GetData());
		glColor4ub (m_colours[vertB->m_colId].r,
                    m_colours[vertB->m_colId].g,
                    m_colours[vertB->m_colId].b,
                    alpha );
		glVertex3fv (m_positions[vertB->m_posId].GetData());

		glNormal3fv(m_normals[norm].GetData());
		glColor4ub (m_colours[vertC->m_colId].r,
                    m_colours[vertC->m_colId].g,
                    m_colours[vertC->m_colId].b,
                    alpha );
		glVertex3fv (m_positions[vertC->m_posId].GetData());
		norm++;
	}
	glEnd();
#endif
}


// *** LookupFragment
// Recursively look through all child fragments until we find a name match
ShapeFragment *ShapeFragment::LookupFragment(char const *_name)
{
	if (stricmp(_name, m_name) == 0)
	{
		return this;
	}
	int numChildFragments = m_childFragments.Size();
	for (int i = 0; i < numChildFragments; ++i)
	{
		ShapeFragment *frag = m_childFragments.GetData(i)->LookupFragment(_name);
		if (frag)
		{
			return frag;
		}
	}

	return NULL;
}


// *** LookupMarker
// Recursively look through all child fragments until we find one with a marker
// matching the specified name
ShapeMarker *ShapeFragment::LookupMarker(char const *_name)
{
	int i;

	int numMarkers = m_childMarkers.Size();
	for (i = 0; i < numMarkers; ++i)
	{
		ShapeMarker *marker = m_childMarkers.GetData(i);
		if (stricmp(_name, marker->m_name) == 0)
		{
			return marker;
		}
	}

	int numChildFragments = m_childFragments.Size();
	for (i = 0; i < numChildFragments; ++i)
	{
		ShapeMarker *marker = m_childFragments.GetData(i)->LookupMarker(_name);
		if (marker)
		{
			return marker;
		}
	}

	return NULL;
}


// *** RenderHitCheck
void ShapeFragment::RenderHitCheck(Matrix34 const &_transform)
{
#ifndef EXPORTER_BUILD
#ifdef DEBUG_RENDER_ENABLED
	Matrix34 totalMatrix = m_transform * _transform;

	if (m_useCylinder)
	{
		Vector3 centreTop = m_centre;
		centreTop.y = m_mostPositiveY;
		centreTop = totalMatrix * centreTop;
		Vector3 centreBase = m_centre;
		centreBase.y = m_mostNegativeY;
		centreBase = totalMatrix * centreBase;
		Vector3 verticalAxis = centreTop - centreBase;
		RenderVerticalCylinder(centreBase, verticalAxis, verticalAxis.Mag(), m_radius);
	}
	else
	{
		// Get world space version of m_centre
		Vector3 centre = totalMatrix * m_centre;
		RenderSphere(centre, m_radius);
	}

    int numChildren = m_childFragments.Size();
	for (int i = 0; i < numChildren; ++i)
	{
    	m_childFragments.GetData(i)->RenderHitCheck(totalMatrix);
	}
#endif
#endif
}


// *** RenderMarkers
void ShapeFragment::RenderMarkers(Matrix34 const &_rootTransform)
{
#ifndef EXPORTER_BUILD
#ifdef DEBUG_RENDER_ENABLED
	int i;

	glDisable(GL_DEPTH_TEST);

	int numMarkers = m_childMarkers.Size();
	for (i = 0; i < numMarkers; ++i)
	{
		ShapeMarker *marker = m_childMarkers.GetData(i);
		Matrix34 mat = marker->GetWorldMatrix(_rootTransform);
		RenderArrow(mat.pos, mat.pos + mat.f * 20.0f, 2.0f);
		RenderArrow(mat.pos, mat.pos + mat.u * 10.0f, 2.0f);
//		glLineWidth(2.0f);
//		glColor3f(1,0,0);
//        glBegin(GL_LINES);
//			glVertex3fv(mat.pos.GetData());
//			glVertex3fv((mat.pos + mat.f * 20.0f).GetData());
//		glEnd();
//		glColor3f(0,1,0);
//		glBegin(GL_LINES);
//			glVertex3fv(mat.pos.GetData());
//			glVertex3fv((mat.pos + mat.u * 10.0f).GetData());
//		glEnd();
	}

	glEnable(GL_DEPTH_TEST);

	int numChildren = m_childFragments.Size();
	for (i = 0; i < numChildren; ++i)
	{
    	m_childFragments.GetData(i)->RenderMarkers(_rootTransform);
	}
#endif
#endif
}


// *** RayHit
bool ShapeFragment::RayHit(RayPackage *_package, Matrix34 const &_transform, bool _accurate)
{
#ifndef EXPORTER_BUILD
	Matrix34 totalMatrix = m_transform * _transform;
	Vector3 centre = totalMatrix * m_centre;

//	Vector3 rayStart = m_transform.InverseMultiplyVector(_package->m_rayStart);
//	Vector3 rayDir = m_transform.GetOr().InverseMultiplyVector(_package->m_rayDir);
//	RayPackage package(rayStart, rayDir);

	// First do bounding sphere check
	if (m_radius > 0.0f &&
		RaySphereIntersection(_package->m_rayStart, _package->m_rayDir,
							  centre, m_radius, _package->m_rayLen))
//		RaySphereIntersection(package.m_rayStart, package.m_rayDir,
//							  m_centre, m_radius, package.m_rayLen))
	{
		// Exit early to save loads of work if we don't care about accuracy too much
		if (_accurate == false)
		{
			return true;
		}

		// Compute World Space versions of all the vertices
		for (int i = 0; i < m_numPositions; ++i)
		{
			m_positionsInWS[i] = m_positions[i] * totalMatrix;
		}

		// Check each triangle in this fragment for intersection
		for (int j = 0; j < m_numTriangles; ++j)
		{
			VertexPosCol *v1 = &m_vertices[m_triangles[j].v1];
			VertexPosCol *v2 = &m_vertices[m_triangles[j].v2];
			VertexPosCol *v3 = &m_vertices[m_triangles[j].v3];
			if (RayTriIntersection(_package->m_rayStart,
								   _package->m_rayDir,
								   m_positionsInWS[v1->m_posId],
								   m_positionsInWS[v2->m_posId],
								   m_positionsInWS[v3->m_posId]))
			{
				return true;
			}
		}
	}

	// If we haven't found a hit then recurse into all child fragments
	int numFragments = m_childFragments.Size();
	for (int i = 0; i < numFragments; ++i)
	{
		ShapeFragment *frag = m_childFragments.GetData(i);
//		if (frag->RayHit(&package, totalMatrix))
		if (frag->RayHit(_package, totalMatrix, _accurate))
		{
			return true;
		}
	}

#endif
	return false;
}


// *** SphereHit
bool ShapeFragment::SphereHit(SpherePackage *_package, Matrix34 const &_transform, bool _accurate)
{
#ifndef EXPORTER_BUILD
	Matrix34 totalMatrix = m_transform * _transform;
	Vector3 centre = totalMatrix * m_centre;

	if (m_radius > 0.0f &&
		SphereSphereIntersection(_package->m_pos, _package->m_radius,
                                 centre, m_radius) )
	{
		// Exit early to save loads of work if we don't care about accuracy too much
		if (_accurate == false)
		{
			return true;
		}

		// Compute World Space versions of all the vertices
		for (int i = 0; i < m_numPositions; ++i)
		{
			m_positionsInWS[i] = m_positions[i] * totalMatrix;
		}

		// Check each triangle in this fragment for intersection
		for (int j = 0; j < m_numTriangles; ++j)
		{
			VertexPosCol *v1 = &m_vertices[m_triangles[j].v1];
			VertexPosCol *v2 = &m_vertices[m_triangles[j].v2];
			VertexPosCol *v3 = &m_vertices[m_triangles[j].v3];
			if (SphereTriangleIntersection(_package->m_pos,
										   _package->m_radius,
										   m_positionsInWS[v1->m_posId],
										   m_positionsInWS[v2->m_posId],
										   m_positionsInWS[v3->m_posId]))
			{
				return true;
			}
		}
	}

	// If we haven't found a hit then recurse into all child fragments
	int numFragments = m_childFragments.Size();
	for (int i = 0; i < numFragments; ++i)
	{
		ShapeFragment *frag = m_childFragments.GetData(i);
		if (frag->SphereHit(_package, totalMatrix, _accurate))
		{
			return true;
		}
	}

#endif
    return false;
}


// *** ShapeHit
bool ShapeFragment::ShapeHit(Shape *_shape, Matrix34 const &_theTransform,
							 Matrix34 const &_ourTransform, bool _accurate)
{
#ifndef EXPORTER_BUILD

	Matrix34 totalMatrix = m_transform * _ourTransform;
	Vector3 centre = totalMatrix * m_centre;

	if (m_radius > 0.0f)
	{
        SpherePackage package( centre, m_radius );
    	if( _shape->SphereHit( &package, _theTransform, _accurate ) )
        {
		    return true;
        }
	}

	int i;

	// If we haven't found a hit then recurse into all child fragments
	int numFragments = m_childFragments.Size();
	for (i = 0; i < numFragments; ++i)
	{
		ShapeFragment *frag = m_childFragments.GetData(i);
		if (frag->ShapeHit(_shape, _theTransform, totalMatrix, _accurate))
		{
			return true;
		}
	}

#endif
    return false;
}


void ShapeFragment::CalculateCentre( Matrix34 const &_transform, Vector3 &_centre, int &_numFragments )
{
	Matrix34 totalMatrix = m_transform * _transform;
	Vector3 centre = totalMatrix * m_centre;

    _centre += centre;
    _numFragments ++;

	int numFragments = m_childFragments.Size();
	for (int i = 0; i < numFragments; ++i)
	{
		ShapeFragment *frag = m_childFragments.GetData(i);
        frag->CalculateCentre( totalMatrix, _centre, _numFragments );
    }
}


void ShapeFragment::CalculateRadius( Matrix34 const &_transform, Vector3 const &_centre, float &_radius )
{
	Matrix34 totalMatrix = m_transform * _transform;
	Vector3 centre = totalMatrix * m_centre;

    float distance = ( centre - _centre ).Mag();
    if( distance + m_radius > _radius )
    {
        _radius = distance + m_radius;
    }

	int numFragments = m_childFragments.Size();
	for (int i = 0; i < numFragments; ++i)
	{
		ShapeFragment *frag = m_childFragments.GetData(i);
        frag->CalculateRadius ( totalMatrix, _centre, _radius );
    }
}



// ****************************************************************************
// Class Shape
// ****************************************************************************

Shape::Shape()
{
}


Shape::Shape(char const *filename, bool _animating)
:	m_displayListName(NULL),
	m_rootFragment(NULL),
	m_name(NULL),
	m_animating(_animating)
{
	TextFileReader in(filename);
	Load(&in);
#ifdef USE_DISPLAY_LISTS
	BuildDisplayList();
#endif
}


Shape::Shape(TextReader *in, bool _animating)
:	m_displayListName(NULL),
	m_rootFragment(NULL),
	m_animating(_animating)
{
	Load(in);
	BuildDisplayList();
}


Shape::~Shape()
{
	delete m_rootFragment;
	free(m_name);
#ifndef EXPORTER_BUILD
	g_app->m_resource->DeleteDisplayList(m_displayListName);
	delete [] m_displayListName;
	m_displayListName = NULL;
#endif
}


void Shape::BuildDisplayList()
{
#ifndef EXPORTER_BUILD
	if (!m_animating)
	{
		m_displayListName = g_app->m_resource->GenerateName();
		int id = g_app->m_resource->CreateDisplayList(m_displayListName);
		glNewList(id, GL_COMPILE);
			m_rootFragment->Render(0.0f);
		glEndList();
	}
#endif
}

void Shape::Load(TextReader *_in)
{
	m_name = strdup(_in->GetFilename());

	int const maxFrags = 100;
	int const maxMarkers = 100;
	int currentFrag = 0;
	int currentMarker = 0;
	ShapeFragment *allFrags[maxFrags];
	ShapeMarker *allMarkers[maxMarkers];

	while(_in->ReadLine())
	{
		if (!_in->TokenAvailable()) continue;

		char *c = _in->GetNextToken();

		if (stricmp(c, "fragment") == 0)
		{
			DarwiniaDebugAssert(currentFrag < maxFrags);
			c = _in->GetNextToken();
			allFrags[currentFrag] = new ShapeFragment(_in, c);
			currentFrag++;
		}
		else if (stricmp(c, "marker") == 0)
		{
			DarwiniaDebugAssert(currentMarker < maxMarkers);
			c = _in->GetNextToken();
			allMarkers[currentMarker] = new ShapeMarker(_in, c);
			currentMarker++;
		}
	}

	m_rootFragment = new ShapeFragment("SceneRoot", "");

	// We need to build the hierarchy of fragments from the flat array
	for (int i = 0; i < currentFrag; ++i)
	{
		if (stricmp(allFrags[i]->m_parentName, "SceneRoot") == 0)
		{
			m_rootFragment->m_childFragments.PutData(allFrags[i]);
		}
		else
		{
			// find the ith fragment's parent
			int j;
			for (j = 0; j < currentFrag; ++j)
			{
				if (i == j)	continue;
				DarwiniaDebugAssert(stricmp(allFrags[i]->m_name, allFrags[j]->m_name) != 0);
				if (stricmp(allFrags[i]->m_parentName, allFrags[j]->m_name) == 0)
				{
					allFrags[j]->m_childFragments.PutData(allFrags[i]);
					break;
				}
			}
			DarwiniaDebugAssert(j < currentFrag);
		}
	}

	// Add the ShapeMarkers into the fragment tree
	for (int i = 0; i < currentMarker; ++i)
	{
		ShapeFragment *parent = m_rootFragment->LookupFragment(allMarkers[i]->m_parentName);
		DarwiniaDebugAssert(parent);
		parent->m_childMarkers.PutData(allMarkers[i]);

		int depth = allMarkers[i]->m_depth - 1;
		allMarkers[i]->m_parents[depth] = parent;
		depth--;
		while (stricmp(parent->m_name, "SceneRoot") != 0)
		{
			parent = m_rootFragment->LookupFragment(parent->m_parentName);
			DarwiniaDebugAssert(parent && depth >= 0);
			allMarkers[i]->m_parents[depth] = parent;
			depth--;
		}
		DarwiniaDebugAssert(depth == -1);
	}
}


void Shape::WriteToFile(FILE *_out) const
{
	m_rootFragment->WriteToFile(_out);
}

void Shape::Render(float _predictionTime, Matrix34 const &_transform)
{
#ifndef EXPORTER_BUILD
	glEnable        (GL_COLOR_MATERIAL);
	glMatrixMode    (GL_MODELVIEW);
	glPushMatrix    ();
	glMultMatrixf   (_transform.ConvertToOpenGLFormat());

#ifdef USE_DISPLAY_LISTS
	int id = -1;
	if (m_displayListName) id = g_app->m_resource->GetDisplayList(m_displayListName);
	if (id != -1)
	{
		glCallList(id);
	}
	else
#endif
	{
		m_rootFragment->Render(_predictionTime);
	}

	glDisable       (GL_COLOR_MATERIAL);
	glMatrixMode    (GL_MODELVIEW);
	glPopMatrix     ();

//	RenderHitCheck(_transform);
#endif
}


void Shape::RenderHitCheck(Matrix34 const &_transform)
{
#ifndef EXPORTER_BUILD
	// Like the main render function this function starts a recursive tree
	// walk, rendering as it goes. UNLIKE the main renderer, it doesn't
	// use the OpenGL matrix stack to store the combined matrix results, but
	// rather does all the matrix muls using our own Matrix34 code. The
	// reason for this is that this function is designed to aid debugging
	// of the hitcheck. Since the hitcheck uses no OpenGL commands, it
	// makes sense to emulate that behaviour here as much as possible.
	m_rootFragment->RenderHitCheck(_transform);
#endif
}


void Shape::RenderMarkers(Matrix34 const &_transform)
{
#ifndef EXPORTER_BUILD
	// Like the main render function this function starts a recursive tree
	// walk, rendering as it goes. UNLIKE the main renderer, it doesn't
	// use the OpenGL matrix stack to store the combined matrix results, but
	// rather does all the matrix muls using our own Matrix34 code. The
	// reason for this is that this function is designed to aid debugging
	// of the hitcheck. Since the hitcheck uses no OpenGL commands, it
	// makes sense to emulate that behaviour here as much as possible.
	m_rootFragment->RenderMarkers(_transform);
#endif
}


bool Shape::RayHit(RayPackage *_package, Matrix34 const &_transform, bool _accurate)
{
#ifndef EXPORTER_BUILD
//	Vector3 rayStart = _transform.InverseMultiplyVector(_package->m_rayStart);
//	Vector3 rayDir = _transform.GetOr().InverseMultiplyVector(_package->m_rayDir);
//	RayPackage package(rayStart, rayDir);

//	bool rv = m_rootFragment->RayHit(&package, _transform);
	bool rv = m_rootFragment->RayHit(_package, _transform, _accurate);
	return rv;
#else
	return false;
#endif
}


bool Shape::SphereHit(SpherePackage *_package, Matrix34 const &_transform, bool _accurate)
{
#ifndef EXPORTER_BUILD
    bool hit = m_rootFragment->SphereHit(_package, _transform, _accurate);
    return hit;
#else
    return false;
#endif
}


bool Shape::ShapeHit(Shape *_shape, Matrix34 const &_theTransform,
					 Matrix34 const &_ourTransform, bool _accurate)
{
#ifndef _EXPORTER_BUILDING
    bool hit = m_rootFragment->ShapeHit(_shape, _theTransform, _ourTransform, _accurate);
    return hit;
#else
    return false;
#endif
}


Vector3 Shape::CalculateCentre( Matrix34 const &_transform )
{
    Vector3 centre;
    int numFragments = 0;

    m_rootFragment->CalculateCentre( _transform, centre, numFragments );

    centre /= (float) numFragments;

    return centre;
}


float Shape::CalculateRadius( Matrix34 const &_transform, Vector3 const &_centre )
{
    float radius = 0.0f;

    m_rootFragment->CalculateRadius( _transform, _centre, radius );

    return radius;
}

