#ifndef __ShapeExporter__H
#define __ShapeExporter__H

#include "Max.h"
#include "resource.h"
#include "istdplug.h"
#include "iparamb2.h"
#include "iparamm2.h"


class Shape;
class ShapeFragment;
class Vector3;
class RGBAColour;
class VertexPosCol;


class ShapeExporter : public SceneExport 
{
protected:
    FILE           *m_out;
	ShapeFragment  *m_fragments[256];
	int				m_numFragments;

	int			    LookupPosition(Vector3 *_allPositions, unsigned short _numPositions, Vector3 const &_pos);
	int			    LookupColour  (RGBAColour *_allColours, unsigned short _numColours, RGBAColour const &_col);
	int				LookupVertex  (VertexPosCol *_allVerts, unsigned short _numVerts, VertexPosCol const &_vert);

    ShapeFragment  *ExportMesh(INode* node, ShapeFragment *_parent);
	void			ExportMarker(INode *node, ShapeFragment *_parent, int _depth);
    int             ExportNode(INode* node, ShapeFragment *_parent, int _depth);
    ShapeFragment  *BuildShapeFragment(Mesh *_mesh, INode *_node, ShapeFragment *_parent);

public:
	static HWND     hParams;
	int				ExtCount();					// Number of extensions supported
	const TCHAR *	Ext(int n);					// Extension #n (i.e. "3DS")
	const TCHAR *	LongDesc();					// Long ASCII description (i.e. "Autodesk 3D Studio File")
	const TCHAR *	ShortDesc();				// Short ASCII description (i.e. "3D Studio")
	const TCHAR *	AuthorName();				// ASCII Author name
	const TCHAR *	CopyrightMessage();			// ASCII Copyright message
	const TCHAR *	OtherMessage1();			// Other message #1
	const TCHAR *	OtherMessage2();			// Other message #2
	unsigned int	Version();					// Version number * 100 (i.e. v3.01 = 301)
	void			ShowAbout(HWND hWnd);		// Show DLL's "About..." box

	BOOL            SupportsOptions(int ext, DWORD options);
	int				DoExport(const TCHAR *name,ExpInterface *ei,Interface *i, BOOL suppressPrompts=FALSE, DWORD options=0);

	ShapeExporter();
	~ShapeExporter();		
};


extern TCHAR *GetString(int id);
extern HINSTANCE hInstance;

#endif // __ShapeExporter__H
