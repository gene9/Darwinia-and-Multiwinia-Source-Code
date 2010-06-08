#ifndef INCLUDED_PERSISTING_DEBUG_RENDER
#define INCLUDED_PERSISTING_DEBUG_RENDER

#ifdef DEBUG_RENDER_ENABLED

#include "lib/fast_darray.h"
#include "lib/vector3.h"


//*****************************************************************************
// Class 
//*****************************************************************************

class PersistRenderItem
{
public:
	Vector3			m_vect1, m_vect2;
	float			m_size1, m_size2, m_size3;
	unsigned int	m_life;
	unsigned int	m_type;
	char			m_label[64];
};


//*****************************************************************************
// Class PersistingDebugRenderer
//*****************************************************************************

class PersistingDebugRenderer
{
protected:
	enum
	{
		TypeSquare2d,
		TypePointMarker,
		TypeSphere,
		TypeVector,
		TypeNumTypes
	};

	FastDArray<PersistRenderItem> m_items;

	PersistRenderItem *FindItem(char const *_label);

public:
	PersistingDebugRenderer();

	void Square2d(float x, float y, float _size, unsigned int _life, char *_text, ...);
	void PointMarker(Vector3 const &_point, unsigned int _life, char *_text, ...);
	void Sphere(Vector3 const &_centre, float _radius, int _life, char *_text, ...);
	void Vector(Vector3 const &_start, Vector3 const &_end, int _life, char *_text, ...);

	void Render();
};


extern PersistingDebugRenderer g_debugRenderer;

#endif // ifdef DEBUG_RENDER_ENABLED

#endif
