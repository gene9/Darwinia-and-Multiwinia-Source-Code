#ifndef INCLUDED_EXPLOSION
#define INCLUDED_EXPLOSION


#include "lib/matrix33.h"
#include "lib/rgb_colour.h"
#include "lib/shape.h"


//*****************************************************************************
// Class Tumbler
//*****************************************************************************

// This class maintains a rotation matrix that is used to make an ExplodingTri 
// tumble as it flys through space. The m_rotMat matrix has a little bit of
// rotation multiplied on to it every frame.
class Tumbler
{
public:
	Matrix33	m_rotMat;
	Vector3		m_angVel;

	Tumbler();
	void Advance();
};


//*****************************************************************************
// Class ExplodingTri
//*****************************************************************************

class ExplodingTri
{
public:
	Vector3		m_vel;
	Vector3		m_pos;
	Vector3		m_v1, m_v2, m_v3;
	Vector3		m_norm;
	Tumbler		*m_tumbler;
	float		m_timeToDie;
	RGBAColour	m_colour;
};


//*****************************************************************************
// Class Explosion
//*****************************************************************************

class Explosion
{
protected:
	Tumbler		 *m_tumblers;
	unsigned int m_numTumblers;
	ExplodingTri *m_tris;
	unsigned int m_numTris;
	float		 m_timeToDie;

public:
	Explosion(ShapeFragment *_frag, Matrix34 const &_transform, float _fraction);
	~Explosion();

	bool Advance();	// Returns true if the explosion has finished
	void Render();

	Vector3 GetCenter() const;
};


//*****************************************************************************
// Class ExplosionManager
//*****************************************************************************

class ExplosionManager
{
protected:
	LList <Explosion *> m_explosions;

public:
	ExplosionManager();
	~ExplosionManager();

	void AddExplosion(ShapeFragment *_frag, Matrix34 const &_transform, bool _recurse, float _fraction);
	void AddExplosion(Shape *_shape, Matrix34 const &_transform, float _fraction=1.0);
	void Reset();

	void Advance();
	void Render();

	const LList <Explosion *> &GetExplosionList() {return m_explosions;} // read access for DeformEffect
};


extern ExplosionManager g_explosionManager;


#endif
