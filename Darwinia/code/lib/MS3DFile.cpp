#pragma warning(disable : 4786)
#include "lib/universal_include.h"
#include "MS3DFile.h"
#include <set>
#include <vector>

#define MAKEDWORD(a, b)      ((unsigned int)(((word)(a)) | ((word)((word)(b))) << 16))

class CMS3DFileI
{
public:
	std::vector<ms3d_vertex_t> arrVertices;
	std::vector<ms3d_triangle_t> arrTriangles;
	std::vector<ms3d_edge_t> arrEdges;
	std::vector<ms3d_group_t> arrGroups;
	std::vector<ms3d_material_t> arrMaterials;
	float fAnimationFPS;
	float fCurrentTime;
	int iTotalFrames;
	std::vector<ms3d_joint_t> arrJoints;

public:
	CMS3DFileI()
	:	fAnimationFPS(24.0f),
		fCurrentTime(0.0f),
		iTotalFrames(0)
	{
	}
};

CMS3DFile::CMS3DFile()
{
	_i = new CMS3DFileI();
}

CMS3DFile::~CMS3DFile()
{
	delete _i;
}

bool CMS3DFile::LoadFromFile(const char* lpszFileName)
{
	FILE *fp = fopen(lpszFileName, "rb");
	if (!fp)
		return false;

	size_t i;
	ms3d_header_t header;
	fread(&header, 1, sizeof(ms3d_header_t), fp);

	if (strncmp(header.id, "MS3D000000", 10) != 0)
		return false;

	if (header.version != 4)
		return false;

	// vertices
	word nNumVertices;
	fread(&nNumVertices, 1, sizeof(word), fp);
	_i->arrVertices.resize(nNumVertices);
	fread(&_i->arrVertices[0], nNumVertices, sizeof(ms3d_vertex_t), fp);

	// triangles
	word nNumTriangles;
	fread(&nNumTriangles, 1, sizeof(word), fp);
	_i->arrTriangles.resize(nNumTriangles);
	fread(&_i->arrTriangles[0], nNumTriangles, sizeof(ms3d_triangle_t), fp);

	// edges
	std::set<unsigned int> setEdgePair;
	for (i = 0; i < _i->arrTriangles.size(); i++)
	{
		word a, b;
		a = _i->arrTriangles[i].vertexIndices[0];
		b = _i->arrTriangles[i].vertexIndices[1];
		if (a > b)
			std::swap(a, b);
		if (setEdgePair.find(MAKEDWORD(a, b)) == setEdgePair.end())
			setEdgePair.insert(MAKEDWORD(a, b));

		a = _i->arrTriangles[i].vertexIndices[1];
		b = _i->arrTriangles[i].vertexIndices[2];
		if (a > b)
			std::swap(a, b);
		if (setEdgePair.find(MAKEDWORD(a, b)) == setEdgePair.end())
			setEdgePair.insert(MAKEDWORD(a, b));

		a = _i->arrTriangles[i].vertexIndices[2];
		b = _i->arrTriangles[i].vertexIndices[0];
		if (a > b)
			std::swap(a, b);
		if (setEdgePair.find(MAKEDWORD(a, b)) == setEdgePair.end())
			setEdgePair.insert(MAKEDWORD(a, b));
	}

	for(std::set<unsigned int>::iterator it = setEdgePair.begin(); it != setEdgePair.end(); ++it)
	{
		unsigned int EdgePair = *it;
		ms3d_edge_t Edge;
		Edge.edgeIndices[0] = (word) EdgePair;
		Edge.edgeIndices[1] = (word) ((EdgePair >> 16) & 0xFFFF);
		_i->arrEdges.push_back(Edge);
	}

	// groups
	word nNumGroups;
	fread(&nNumGroups, 1, sizeof(word), fp);
	_i->arrGroups.resize(nNumGroups);
	for (i = 0; i < nNumGroups; i++)
	{
		fread(&_i->arrGroups[i].flags, 1, sizeof(byte), fp);
		fread(&_i->arrGroups[i].name, 32, sizeof(char), fp);
		fread(&_i->arrGroups[i].numtriangles, 1, sizeof(word), fp);
		_i->arrGroups[i].triangleIndices = new word[_i->arrGroups[i].numtriangles];
		fread(_i->arrGroups[i].triangleIndices, _i->arrGroups[i].numtriangles, sizeof(word), fp);
		fread(&_i->arrGroups[i].materialIndex, 1, sizeof(char), fp);
	}

	// materials
	word nNumMaterials;
	fread(&nNumMaterials, 1, sizeof(word), fp);
	_i->arrMaterials.resize(nNumMaterials);
	fread(&_i->arrMaterials[0], nNumMaterials, sizeof(ms3d_material_t), fp);

	fread(&_i->fAnimationFPS, 1, sizeof(float), fp);
	fread(&_i->fCurrentTime, 1, sizeof(float), fp);
	fread(&_i->iTotalFrames, 1, sizeof(int), fp);

	// joints
	word nNumJoints;
	fread(&nNumJoints, 1, sizeof(word), fp);
	_i->arrJoints.resize(nNumJoints);
	for (i = 0; i < nNumJoints; i++)
	{
		fread(&_i->arrJoints[i].flags, 1, sizeof(byte), fp);
		fread(&_i->arrJoints[i].name, 32, sizeof(char), fp);
		fread(&_i->arrJoints[i].parentName, 32, sizeof(char), fp);
		fread(&_i->arrJoints[i].rotation, 3, sizeof(float), fp);
		fread(&_i->arrJoints[i].position, 3, sizeof(float), fp);
		fread(&_i->arrJoints[i].numKeyFramesRot, 1, sizeof(word), fp);
		fread(&_i->arrJoints[i].numKeyFramesTrans, 1, sizeof(word), fp);
		_i->arrJoints[i].keyFramesRot = new ms3d_keyframe_rot_t[_i->arrJoints[i].numKeyFramesRot];
		_i->arrJoints[i].keyFramesTrans = new ms3d_keyframe_pos_t[_i->arrJoints[i].numKeyFramesTrans];
		fread(_i->arrJoints[i].keyFramesRot, _i->arrJoints[i].numKeyFramesRot, sizeof(ms3d_keyframe_rot_t), fp);
		fread(_i->arrJoints[i].keyFramesTrans, _i->arrJoints[i].numKeyFramesTrans, sizeof(ms3d_keyframe_pos_t), fp);
	}

	fclose(fp);

	return true;
}

bool CMS3DFile::SaveToShape(const char* lpszFileName)
{
	FILE *fp = fopen(lpszFileName, "w");
	if (!fp)
		return false;

	for ( int i = 0; i < GetNumGroups(); i++ )
	{
		ms3d_group_t *nextGroup;
		GetGroupAt(i, &nextGroup);

		fprintf(fp,"Fragment:\t%s\n", nextGroup->name);
		fprintf(fp,"\tParentName:\tsceneroot\n");
		fprintf(fp, "\tup:    0.00  1.00  0.00\n");
		fprintf(fp, "\tfront: 0.00  0.00  1.00\n");
		fprintf(fp, "\tpos:   0.00  0.00  0.00\n");

		// Write out the positions
		fprintf(fp, "\tPositions: %d\n", (nextGroup->numtriangles*3));
		for ( int j = 0; j < nextGroup->numtriangles; j++)
		{
			word index = nextGroup->triangleIndices[j];
			ms3d_triangle_t *t;
			GetTriangleAt(index,&t);

			for ( int k = 0; k < 3; k++ )
			{
				ms3d_vertex_t *v;
				GetVertexAt(t->vertexIndices[k], &v);
				fprintf(fp, "\t\t%d: %7.3f %7.3f %7.3f\n", (j*3)+k, v->vertex[0], v->vertex[1], v->vertex[2]);
			}
		}

		// Write out the normals
		fprintf(fp, "\tNormals: %d\n", 0);

		// Write out the colours
		fprintf(fp, "\tColours: %d\n", 1);
		ms3d_material_t *m;
		GetMaterialAt(nextGroup->materialIndex, &m);
		fprintf(fp, "\t\t%d: %d %d %d\n", 0, (int) (m->ambient[0]*255), (int) (m->ambient[1]*255), (int) (m->ambient[2]*255));

		// Write out the vertices
		fprintf(fp, "\tVertices: %d    # Position ID then Colour ID\n", (nextGroup->numtriangles*3));
		for (int j = 0; j < nextGroup->numtriangles; j++)
		{
			for ( int k = 0; k < 3; k++ )
			{
				fprintf(fp, "\t\t%d: %d %d\n", (j*3)+k, (j*3)+k, 0);
			}
		}


		// Write out the triangles
		fprintf(fp, "\tTriangles: %d\n", nextGroup->numtriangles);
		for (int j = 0; j < nextGroup->numtriangles; j++)
		{
			fprintf(fp, "\t\t%d %d %d\n", (j*3)+0, (j*3)+1, (j*3)+2);
		}
	}
	fprintf(fp, "\n");

	fclose(fp);

	return true;
}

void CMS3DFile::Clear()
{
	_i->arrVertices.clear();
	_i->arrTriangles.clear();
	_i->arrEdges.clear();
	_i->arrGroups.clear();
	_i->arrMaterials.clear();
	_i->arrJoints.clear();
}

int CMS3DFile::GetNumVertices()
{
	return (int) _i->arrVertices.size();
}

void CMS3DFile::GetVertexAt(int nIndex, ms3d_vertex_t **ppVertex)
{
	if (nIndex >= 0 && nIndex < (int) _i->arrVertices.size())
		*ppVertex = &_i->arrVertices[nIndex];
}

int CMS3DFile::GetNumTriangles()
{
	return (int) _i->arrTriangles.size();
}

void CMS3DFile::GetTriangleAt(int nIndex, ms3d_triangle_t **ppTriangle)
{
	if (nIndex >= 0 && nIndex < (int) _i->arrTriangles.size())
		*ppTriangle = &_i->arrTriangles[nIndex];
}

int CMS3DFile::GetNumEdges()
{
	return (int) _i->arrEdges.size();
}

void CMS3DFile::GetEdgeAt(int nIndex, ms3d_edge_t **ppEdge)
{
	if (nIndex >= 0 && nIndex < (int) _i->arrEdges.size())
		*ppEdge = &_i->arrEdges[nIndex];
}

int CMS3DFile::GetNumGroups()
{
	return (int) _i->arrGroups.size();
}

void CMS3DFile::GetGroupAt(int nIndex, ms3d_group_t **ppGroup)
{
	if (nIndex >= 0 && nIndex < (int) _i->arrGroups.size())
		*ppGroup = &_i->arrGroups[nIndex];
}

int CMS3DFile::GetNumMaterials()
{
	return (int) _i->arrMaterials.size();
}

void CMS3DFile::GetMaterialAt(int nIndex, ms3d_material_t **ppMaterial)
{
	if (nIndex >= 0 && nIndex < (int) _i->arrMaterials.size())
		*ppMaterial = &_i->arrMaterials[nIndex];
}

int CMS3DFile::GetNumJoints()
{
	return (int) _i->arrJoints.size();
}

void CMS3DFile::GetJointAt(int nIndex, ms3d_joint_t **ppJoint)
{
	if (nIndex >= 0 && nIndex < (int) _i->arrJoints.size())
		*ppJoint = &_i->arrJoints[nIndex];
}

int CMS3DFile::FindJointByName(const char* lpszName)
{
	for (size_t i = 0; i < _i->arrJoints.size(); i++)
	{
		if (!strcmp(_i->arrJoints[i].name, lpszName))
			return i;
	}

	return -1;
}

float CMS3DFile::GetAnimationFPS()
{
	return _i->fAnimationFPS;
}

float CMS3DFile::GetCurrentTime()
{
	return _i->fCurrentTime;
}

int CMS3DFile::GetTotalFrames()
{
	return _i->iTotalFrames;
}
