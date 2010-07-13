#include "nvtristrip.h"

#include "lib/debug_utils.h"
#include "lib/rgb_colour.h"
#include "lib/shape.h"
//#include "lib/tosser.h"
#include "ShapeExporter.h"

#define ShapeExporter_CLASS_ID	Class_ID(0x3288038e, 0x40eff553)


class ShapeExporterClassDesc:public ClassDesc2 
{
	public:
	int 			IsPublic() { return TRUE; }
	void *			Create(BOOL loading = FALSE) { return new ShapeExporter(); }
	const TCHAR *	ClassName() { return GetString(IDS_CLASS_NAME); }
	SClass_ID		SuperClassID() { return SCENE_EXPORT_CLASS_ID; }
	Class_ID		ClassID() { return ShapeExporter_CLASS_ID; }
	const TCHAR* 	Category() { return GetString(IDS_CATEGORY); }

	const TCHAR*	InternalName() { return _T("ShapeExporter"); }	// returns fixed parsable name (scripter-visible name)
	HINSTANCE		HInstance() { return hInstance; }				// returns owning module handle

    char *          GetRsrcString( long bla ) { return "Hello!"; }
};


static ShapeExporterClassDesc ShapeExporterDesc;
ClassDesc2* GetShapeExporterDesc() 
{ 
    return &ShapeExporterDesc; 
}


BOOL CALLBACK ShapeExporterOptionsDlgProc(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam) 
{
	static ShapeExporter *imp = NULL;

	switch(message) 
	{
		case WM_INITDIALOG:
			imp = (ShapeExporter *)lParam;
			CenterWindow(hWnd,GetParent(hWnd));
			return TRUE;

		case WM_CLOSE:
			EndDialog(hWnd, 0);
			return TRUE;
	}
	return FALSE;
}


// ****************************************************************************
// Class ShapeExporter
// ****************************************************************************

ShapeExporter::ShapeExporter()
:	m_numFragments(0)
{
}

ShapeExporter::~ShapeExporter() 
{
	for (int i = 0; i < m_numFragments; ++i)
	{
		delete m_fragments[i];
	}
}

// Returns the number of file name extensions supported by the plug-in.
int ShapeExporter::ExtCount()
{
	return 1;
}

// Return the 'i-th' file name extension (i.e. "3DS").
const TCHAR *ShapeExporter::Ext(int n)
{		
	return _T("shp");
}

const TCHAR *ShapeExporter::LongDesc()
{
	return _T("Futurewar Shape Format v1.0");
}
	
const TCHAR *ShapeExporter::ShortDesc() 
{			
	return _T("Futurewar Shape");
}

const TCHAR *ShapeExporter::AuthorName()
{			
	return _T("Introversion Software");
}

const TCHAR *ShapeExporter::CopyrightMessage() 
{	
	return _T("");
}

const TCHAR *ShapeExporter::OtherMessage1() 
{		
	return _T("");
}

const TCHAR *ShapeExporter::OtherMessage2() 
{		
	return _T("");
}

unsigned int ShapeExporter::Version()
{				
	// Return Version number * 100 (i.e. v3.01 = 301)
	return 100;
}

void ShapeExporter::ShowAbout(HWND hWnd)
{			
}

BOOL ShapeExporter::SupportsOptions(int ext, DWORD options)
{
	// TODO Decide which options to support.  Simply return
	// true for each option supported by each Extension 
	// the exporter supports.

	return TRUE;
}


// *** LookupPosition
int ShapeExporter::LookupPosition(Vector3 *_allPositions, 
								  unsigned short _numPositions, 
								  Vector3 const &_pos)
{
	float const threshold = 0.001f * 0.001f;

	for (unsigned short i = 0; i < _numPositions; ++i)
	{
		float magSquared = (_pos - _allPositions[i]).MagSquared();
		if (magSquared < threshold)
		{
			return i;
		}
	}

	return -1;
}


// *** LookupColour
int ShapeExporter::LookupColour(RGBAColour *_allColours, 
								unsigned short _numColours, 
								RGBAColour const &_col)
{
	for (unsigned short i = 0; i < _numColours; ++i)
	{
		if (_allColours[i].r == _col.r &&
			_allColours[i].g == _col.g &&
			_allColours[i].b == _col.b)
		{
			return i;
		}
	}

	return -1;
}


// *** LookupVertex
int ShapeExporter::LookupVertex(VertexPosCol *_allVerts, 
								unsigned short _numVerts, 
								VertexPosCol const &_vert)
{
	for (unsigned short i = 0; i < _numVerts; ++i)
	{
		if (_allVerts[i].m_colId == _vert.m_colId &&
			_allVerts[i].m_posId == _vert.m_posId)
		{
			return i;
		}
	}

	return -1;
}


ShapeFragment *ShapeExporter::BuildShapeFragment(Mesh *_mesh, INode *_node, ShapeFragment *_parent)
{
	char const *parentName = "";
	if (_parent)
	{
		parentName = _parent->m_name;
	}
	ShapeFragment *frag = new ShapeFragment(_node->GetName(), parentName, false);
    int i;


	// Orientation and position
	{
		INode *parent = _node->GetParentNode();
		Matrix3 nodeTM = _node->GetNodeTM(0);
		Matrix3 parentTM = parent->GetNodeTM(0);
		Matrix3 localTM = nodeTM * Inverse(parentTM);
		Point3 r = localTM.GetRow(0);
		Point3 f = localTM.GetRow(1);
		Point3 u = localTM.GetRow(2);
		Point3 pos = localTM.GetRow(3);
		frag->m_transform.r.Set(r.x, r.z, -r.y);
		frag->m_transform.u.Set(u.x, u.z, -u.y);
		frag->m_transform.f.Set(-f.x, -f.z, f.y);
		frag->m_transform.pos.Set(pos.x, pos.z, -pos.y);
		frag->m_transform.Normalise();
	}

	
	// POSITIONS
	// Create an array containing all the _unique_ positions that Max has given
	// us. Then register than array with the shape fragment.
	{
		int maxPositions = _mesh->getNumVerts();
		Vector3 *positions = new Vector3[maxPositions];
		int numPositions = 0;
		for (i = 0; i < maxPositions; i++) 
		{
			Point3 const &point3 = _mesh->verts[i];
			Vector3 pos(point3.x, point3.z, -point3.y);

			// If the position isn't already present in our array
			// of positions then add it
			if (LookupPosition(positions, numPositions, pos) == -1)
			{
				positions[numPositions] = pos;
				numPositions++;
			}
		}
		frag->RegisterPositions(positions, numPositions);
	}
    

	// COLOURS
	// Create an array containing _unique_ colours as with the positions
	{
		int maxColours = _mesh->numCVerts;

		RGBAColour *colours;
		int numColours = 0;

		// If the Max scene contains vertex colour information for this mesh, then
		// build up our array of colours
		if (maxColours) 
		{
			colours = new RGBAColour[maxColours];
			for (i = 0; i < maxColours; i++) 
			{
				RGBAColour col;
				col.r = unsigned char(_mesh->vertCol[i].x * 255.0f);
				col.g = unsigned char(_mesh->vertCol[i].y * 255.0f);
				col.b = unsigned char(_mesh->vertCol[i].z * 255.0f);

				// If the colour isn't already present in our array of colours
				// then add it
				if (LookupColour(colours, numColours, col) == -1)
				{
					colours[numColours] = col;
					numColours++;
				}
			}
		}
		else // Otherwise just use the per object colour data
		{
			colours = new RGBAColour[1];
			numColours = 1;

			Mtl *material = _node->GetMtl();
			if (material)
			{
				Color col = material->GetDiffuse();
				colours[0].Set(col.r * 255.0f, 
							   col.g * 255.0f, 
							   col.b * 255.0f);
			}
			else
			{
				DWORD color = _node->GetWireColor();
				unsigned char red =   (color >> 0) & 0xff;
				unsigned char green = (color >> 8) & 0xff;
				unsigned char blue = (color >> 16) & 0xff;
				colours[0].Set(red, green, blue);
			}
		}

		frag->RegisterColours(colours, numColours);
	}
	

	unsigned int numIndices;
	unsigned short *indices;

	// VERTICES and FACES
	// Our notion of vertices is different to that of Max. In Max a vertex is a
	// position - Max doesn't directly associate colour information with 
	// vertices. In our shape format a vertex has a position and a colour value.
	// Because of this we have to find out which Max colours go with which Max
	// positions and join them together to create our vertices. Also we have to
	// create a new array of faces because the Max faces contain the wrong
	// indices for our vertices. Also, we might as well create the new face array
	// in the format that the NVidia stripping library expects, because we're
	// going to use that next.
	{
		int numFaces = _mesh->getNumFaces();

		// Allocate enough space for all the vertices we could possibly ever need
		// to create for this Mesh
		int maxVerts = numFaces * 3; // Probably nearly three times as much space as we need, but who cares.
		VertexPosCol *vertices = new VertexPosCol[maxVerts];

		// Allocate the triangle list in the format that the NVidia stripping 
		// library expects below.
		numIndices = numFaces * 3;
		indices = new unsigned short[numIndices];

		int numVertices = 0; // This gets incremented every time we find a new unique vertex
		for (int i = 0; i < numFaces; ++i)
		{
			// Work out the index of the three vertices for this face. If the vertices
			// don't already exist then create them
			int vertexIds[3];
			for (int j = 0; j < 3; ++j)
			{
				VertexPosCol vert;

				// Position
				int posSDKId = _mesh->faces[i].v[j];
				Vector3 pos(_mesh->verts[posSDKId].x, _mesh->verts[posSDKId].z, -_mesh->verts[posSDKId].y);
				vert.m_posId = LookupPosition(frag->m_positions, 
											  frag->m_numPositions, 
											  pos);
				
				// Colour
				if (_mesh->vcFace)	// Guards against a mesh with no colour data
				{
					int colSDKId = _mesh->vcFace[i].t[j];
					RGBAColour col(_mesh->vertCol[colSDKId].x * 255.0f,
								  _mesh->vertCol[colSDKId].y * 255.0f, 
								  _mesh->vertCol[colSDKId].z * 255.0f);
					vert.m_colId = LookupColour  (frag->m_colours, 
												  frag->m_numColours, 
												  col);
				}
				else
				{
					vert.m_colId = 0;
				}
	
				// Get the index of this vertex if it already exists in our array
				// of vertices.
				vertexIds[j] = LookupVertex(vertices, numVertices, vert);

				// If the vertex didn't already exist in the array then add it
				if (vertexIds[j] == -1)
				{
					vertexIds[j] = numVertices;
					vertices[numVertices] = vert;
					numVertices++;
				}
			}

			indices[i * 3] = vertexIds[0];
			indices[i * 3 + 1] = vertexIds[1];
			indices[i * 3 + 2] = vertexIds[2];
		}

		frag->RegisterVertices(vertices, numVertices);
	}

	
	// Triangles
	{	
		int numTris = numIndices / 3;
		ShapeTriangle *tris = new ShapeTriangle[numTris];
		for (int i = 0; i < numTris; ++i)
		{
			tris[i].v1 = indices[0];
			tris[i].v2 = indices[1];
			tris[i].v3 = indices[2];
			indices += 3;
		}
		frag->RegisterTriangles(tris, numTris);    
	}

	return frag;
}


ShapeFragment *ShapeExporter::ExportMesh(INode *_node, ShapeFragment *_parent)
{
	ObjectState os = _node->EvalWorldState(TimeValue(0));
	
    if (!os.obj || os.obj->SuperClassID() != GEOMOBJECT_CLASS_ID) 
    {
		return NULL; // Safety net. This shouldn't happen.
	}
	
	if (!os.obj->CanConvertToType(Class_ID(TRIOBJ_CLASS_ID, 0))) 
    { 
        return NULL;
    }

	TriObject *tri = (TriObject *)os.obj->ConvertToType(0/*time*/, Class_ID(TRIOBJ_CLASS_ID, 0));
	if (!tri) 
    {
		return NULL;
	}

	Mesh *mesh = &tri->GetMesh();
    if (mesh->getNumFaces() == 0)
    {
        return NULL;
    }

	mesh->buildNormals();
	
    ShapeFragment *frag = BuildShapeFragment(mesh, _node, _parent);
	_parent->m_childFragments.PutData(frag);

    return frag;
}


void ShapeExporter::ExportMarker(INode *_node, ShapeFragment *_parent, int _depth)
{
	INode *parent = _node->GetParentNode();
	Matrix3 nodeTM = _node->GetNodeTM(0);
	Matrix3 parentTM = parent->GetNodeTM(0);
	Matrix3 localTM = nodeTM * Inverse(parentTM);
	Matrix34 transform;
	Point3 r = localTM.GetRow(0);
	Point3 f = localTM.GetRow(1);
	Point3 u = localTM.GetRow(2);
	Point3 pos = localTM.GetRow(3);
	transform.r.Set(r.x, r.z, -r.y);
	transform.u.Set(u.x, u.z, -u.y);
	transform.f.Set(f.x, f.z, -f.y);
	transform.pos.Set(pos.x, pos.z, -pos.y);
	transform.Normalise();
	ShapeMarker *marker = new ShapeMarker(_node->GetName(), _parent->m_name, _depth, transform);
	_parent->m_childMarkers.PutData(marker);
}


int ShapeExporter::ExportNode(INode *_node, ShapeFragment *_parent, int _depth)
{
	ObjectState os = _node->EvalWorldState(0);
	if ( (!os.obj) || (os.obj->SuperClassID() != GEOMOBJECT_CLASS_ID) )
    {
        return -1;
    }
	
	// Targets are actually geomobjects, but we will export them
	// from the camera and light objects, so we skip them here.
	if (os.obj->ClassID() == Class_ID(TARGET_CLASS_ID, 0))
    {
		return -1;
    }

	if (strnicmp(_node->GetName(), "marker", strlen("marker")) == 0)
	{
		ExportMarker(_node, _parent, _depth);
	}
	else
	{
		ShapeFragment *frag = ExportMesh(_node, _parent);
		if (!frag)
		{
			return -1;
		}

		// For each child of this node, we recurse into ourselves 
		// until no more children are found.
		for (int c = 0; c < _node->NumberOfChildren(); c++) 
		{
			if (ExportNode(_node->GetChildNode(c), frag, _depth + 1) == -1)
			{
				return -1;
			}
		}
	}

    return 0;
}


int	ShapeExporter::DoExport(const TCHAR *_name, ExpInterface *_ei, 
							Interface *_interface, BOOL _suppressPrompts, DWORD _options)
{
	int i;

    m_out = fopen(_name, "w");
    if (!m_out)
    {
        return -1;
    }

/*    if (!_suppressPrompts)
    {
		DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_PANEL), 
				       GetActiveWindow(), ShapeExporterOptionsDlgProc, (LPARAM)this);
    }*/

	Shape shape;
	shape.m_rootFragment = new ShapeFragment("scene root", "", false);

   	int numChildren = _interface->GetRootNode()->NumberOfChildren();
	for (i = 0; i < numChildren; ++i) 
    {
		if (_interface->GetCancel())
        {
			break;
        }
		ExportNode(_interface->GetRootNode()->GetChildNode(i), shape.m_rootFragment, 1);
	}

    shape.WriteToFile(m_out);
    fclose(m_out);

	return 1;
}




/*	// Export face map texcoords if we have them...
	if (GetIncludeTextureCoords() && !CheckForAndExportFaceMap(nodeMaterialInterface, mesh, indentLevel+1)) {
		// If not, export standard tverts
		int numTVx = mesh->getNumTVerts();

		fprintf(m_out, "%s\t%s %d\n",indent.data(), ID_MESH_NUMTVERTEX, numTVx);

		if (numTVx) {
			fprintf(m_out,"%s\t%s {\n",indent.data(), ID_MESH_TVERTLIST);
			for (i=0; i<numTVx; i++) {
				UVVert tv = mesh->tVerts[i];
				fprintf(m_out, "%s\t\t%s %d\t%s\n",indent.data(), ID_MESH_TVERT, i, Format(tv));
			}
			fprintf(m_out,"%s\t}\n",indent.data());
			
			fprintf(m_out, "%s\t%s %d\n",indent.data(), ID_MESH_NUMTVFACES, mesh->getNumFaces());

			fprintf(m_out, "%s\t%s {\n",indent.data(), ID_MESH_TFACELIST);
			for (i=0; i<mesh->getNumFaces(); i++) {
				fprintf(m_out,"%s\t\t%s %d\t%d\t%d\t%d\n",
					indent.data(),
					ID_MESH_TFACE, i,
					mesh->tvFace[i].t[vx1],
					mesh->tvFace[i].t[vx2],
					mesh->tvFace[i].t[vx3]);
			}
			fprintf(m_out, "%s\t}\n",indent.data());
		}

		// CCJ 3/9/99
		// New for R3 - Additional mapping channels
		for (int mp = 2; mp < MAX_MESHMAPS-1; mp++) {
			if (mesh->mapSupport(mp)) {

				fprintf(m_out, "%s\t%s %d {\n",indent.data(), ID_MESH_MAPPINGCHANNEL, mp);


				int numTVx = mesh->getNumMapVerts(mp);
				fprintf(m_out, "%s\t\t%s %d\n",indent.data(), ID_MESH_NUMTVERTEX, numTVx);

				if (numTVx) {
					fprintf(m_out,"%s\t\t%s {\n",indent.data(), ID_MESH_TVERTLIST);
					for (i=0; i<numTVx; i++) {
						UVVert tv = mesh->mapVerts(mp)[i];
						fprintf(m_out, "%s\t\t\t%s %d\t%s\n",indent.data(), ID_MESH_TVERT, i, Format(tv));
					}
					fprintf(m_out,"%s\t\t}\n",indent.data());
					
					fprintf(m_out, "%s\t\t%s %d\n",indent.data(), ID_MESH_NUMTVFACES, mesh->getNumFaces());

					fprintf(m_out, "%s\t\t%s {\n",indent.data(), ID_MESH_TFACELIST);
					for (i=0; i<mesh->getNumFaces(); i++) {
						fprintf(m_out,"%s\t\t\t%s %d\t%d\t%d\t%d\n",
							indent.data(),
							ID_MESH_TFACE, i,
							mesh->mapFaces(mp)[i].t[vx1],
							mesh->mapFaces(mp)[i].t[vx2],
							mesh->mapFaces(mp)[i].t[vx3]);
					}
					fprintf(m_out, "%s\t\t}\n",indent.data());
				}
				fprintf(m_out, "%s\t}\n",indent.data());
			}
		}
	}

	// Export color per vertex info
	if (GetIncludeVertexColors()) {
		int numCVx = mesh->numCVerts;

		fprintf(m_out, "%s\t%s %d\n",indent.data(), ID_MESH_NUMCVERTEX, numCVx);
		if (numCVx) {
			fprintf(m_out,"%s\t%s {\n",indent.data(), ID_MESH_CVERTLIST);
			for (i=0; i<numCVx; i++) {
				Point3 vc = mesh->vertCol[i];
				fprintf(m_out, "%s\t\t%s %d\t%s\n",indent.data(), ID_MESH_VERTCOL, i, Format(vc));
			}
			fprintf(m_out,"%s\t}\n",indent.data());
			
			fprintf(m_out, "%s\t%s %d\n",indent.data(), ID_MESH_NUMCVFACES, mesh->getNumFaces());

			fprintf(m_out, "%s\t%s {\n",indent.data(), ID_MESH_CFACELIST);
			for (i=0; i<mesh->getNumFaces(); i++) {
				fprintf(m_out,"%s\t\t%s %d\t%d\t%d\t%d\n",
					indent.data(),
					ID_MESH_CFACE, i,
					mesh->vcFace[i].t[vx1],
					mesh->vcFace[i].t[vx2],
					mesh->vcFace[i].t[vx3]);
			}
			fprintf(m_out, "%s\t}\n",indent.data());
		}
	}
	
	if (GetIncludeNormals()) {
		// Export mesh (face + vertex) normals
		fprintf(m_out, "%s\t%s {\n",indent.data(), ID_MESH_NORMALS);
		
		Point3 fn;  // Face normal
		Point3 vn;  // Vertex normal
		int  vert;
		Face* f;
		
		// Face and vertex normals.
		// In MAX a vertex can have more than one normal (but doesn't always have it).
		// This is depending on the face you are accessing the vertex through.
		// To get all information we need to export all three vertex normals
		// for every face.
		for (i=0; i<mesh->getNumFaces(); i++) {
			f = &mesh->faces[i];
			fn = mesh->getFaceNormal(i);
			fprintf(m_out,"%s\t\t%s %d\t%s\n", indent.data(), ID_MESH_FACENORMAL, i, Format(fn));
			
			vert = f->getVert(vx1);
			vn = GetVertexNormal(mesh, i, mesh->getRVertPtr(vert));
			fprintf(m_out,"%s\t\t\t%s %d\t%s\n",indent.data(), ID_MESH_VERTEXNORMAL, vert, Format(vn));
			
			vert = f->getVert(vx2);
			vn = GetVertexNormal(mesh, i, mesh->getRVertPtr(vert));
			fprintf(m_out,"%s\t\t\t%s %d\t%s\n",indent.data(), ID_MESH_VERTEXNORMAL, vert, Format(vn));
			
			vert = f->getVert(vx3);
			vn = GetVertexNormal(mesh, i, mesh->getRVertPtr(vert));
			fprintf(m_out,"%s\t\t\t%s %d\t%s\n",indent.data(), ID_MESH_VERTEXNORMAL, vert, Format(vn));
		}
		
		fprintf(m_out, "%s\t}\n",indent.data());
	}
	
	fprintf(m_out, "%s}\n",indent.data());
	
	if (needDel) {
		delete tri;
	}*/
