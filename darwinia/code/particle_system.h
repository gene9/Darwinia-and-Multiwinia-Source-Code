#ifndef _included_particle_system_h
#define _included_particle_system_h

#include "lib/rgb_colour.h"
#include "lib/slice_darray.h"
#include "lib/vector3.h"


// ****************************************************************************
// ParticleType
// ****************************************************************************

class ParticleType
{
public:
	float m_spin;			// Rate of spin in radians per second (positive is clockwise)
	float m_fadeInTime;
	float m_fadeOutTime;
	float m_life;
	float m_size;
	float m_friction;		// Amount of friction to apply (0=none 1=a huge amount)
	float m_gravity;		// Amount of gravity to apply (0=none 1=quite a lot)
	RGBAColour m_colour1;		// Start of colour range
	RGBAColour m_colour2;		// End of colour range

	ParticleType();
};


// ****************************************************************************
// Particle
// ****************************************************************************

class Particle
{
public:
	enum
	{
		TypeInvalid = -1,
		TypeRocketTrail = 0,
		TypeExplosionCore,
		TypeExplosionDebris,
		TypeMuzzleFlash,
        TypeFire,
        TypeControlFlash,
        TypeSpark,
        TypeBlueSpark,
        TypeBrass,
        TypeMissileTrail,
        TypeMissileFire,
        TypeDarwinianFire,
		TypeLeaf,
		TypeNumTypes
	};

private:
	static ParticleType m_types[TypeNumTypes];

public:
	Vector3         m_pos;
	Vector3         m_vel;
	float           m_birthTime;
	int				m_typeId;
    float           m_size;
    RGBAColour      m_colour;
	
	Particle();

    void Initialise(Vector3 const &_pos, Vector3 const &_vel, 
                    int _type, float _size=-1.0f);
	bool Advance();
	void Render(float _predictionTime);

    static void SetupParticles();
};



// ****************************************************************************
// ParticleSystem
// ****************************************************************************

class ParticleSystem
{
private:
	SliceDArray         <Particle> m_particles;

public:
	ParticleSystem();

	void CreateParticle(Vector3 const &_pos, Vector3 const &_vel, 
                        int _particleTypeId, float _size=-1.0f, RGBAColour col = NULL );

	void Advance(int _slice);
	void Render();
	void Empty();
};

#endif
