#include "lib/universal_include.h"


#include <math.h>

#include "lib/debug_utils.h"
#include "lib/hi_res_time.h"
#include "lib/math_utils.h"
#include "lib/profiler.h"
#include "lib/resource.h"

#include "app.h"
#include "camera.h"
#include "main.h"
#include "particle_system.h"
#include "location.h"


// ****************************************************************************
// ParticleType
// ****************************************************************************

// *** Constructor
ParticleType::ParticleType()
:	m_spin(0.0f),
	m_fadeInTime(0.0f),
	m_fadeOutTime(0.0f),
	m_life(10.0f),
	m_size(1000.0f),
	m_friction(0.0f),
	m_gravity(0.0f),
	m_colour1(0.5f, 0.5f, 0.5f),
	m_colour2(0.95f, 0.95f, 0.95f)
{
}


// ****************************************************************************
// Particle
// ****************************************************************************

ParticleType Particle::m_types[TypeNumTypes];

// *** Constructor
Particle::Particle()
{
}


// *** Initialise
void Particle::Initialise(Vector3 const &_pos, Vector3 const &_vel, 
                          int _typeId, float _size)
{
	DarwiniaDebugAssert(_typeId < Particle::TypeNumTypes);

	m_pos = _pos;
	m_vel = _vel;
	m_typeId = _typeId;

	ParticleType const &type = m_types[_typeId];
	float amount = frand();
	m_colour = type.m_colour1 * amount + type.m_colour2 * (1.0f - amount);
    
    if( _size == -1 )
    {
        m_size = type.m_size;
    }
    else
    {
        m_size = _size;
    }

	m_birthTime = g_gameTime;
}


// *** Advance
// Returns true if this particle should be removed from the particle list
bool Particle::Advance()
{   
	ParticleType const &type = m_types[m_typeId];
	double timeToDie = m_birthTime + m_types[m_typeId].m_life;
	if (g_gameTime > timeToDie)
	{
		return true;
	}

	m_pos += m_vel * SERVER_ADVANCE_PERIOD;
	
	float amount = SERVER_ADVANCE_PERIOD * type.m_friction;
	m_vel *= (1.0f - amount);

	if (type.m_gravity != 0.0f)
	{
        float gravityScale = m_size / m_types[m_typeId].m_size;
		m_vel.y -= SERVER_ADVANCE_PERIOD * type.m_gravity * gravityScale;
	}


    //
    // Particles that spawn particles

    bool particleCreated = false;

    if( m_typeId == TypeExplosionDebris )
    {
        if( frand() < 0.3f )
        {
			// *** WARNING ***
			// CreateParticle might have to resize the particles array, which
			// might invalidate what "p" points to, so we need to make local
			// copies of the bits of p that we need
			Vector3 pos = m_pos;
			Vector3 vel = m_vel / 5.0f;
            g_app->m_particleSystem->CreateParticle( pos, vel, Particle::TypeRocketTrail, m_size/2.0f );
            particleCreated = true;
        }       
    }


    //
    // Particles that bounce
    
    if( !particleCreated )
    {
        if( m_typeId == TypeSpark || 
            m_typeId == TypeBrass ||
            m_typeId == TypeBlueSpark ||
			m_typeId == TypeLeaf )
        {
            float landHeight = g_app->m_location->m_landscape.m_heightMap->GetValue(m_pos.x, m_pos.z);
            if( m_pos.y <= landHeight )
            {
	            Vector3 lastPos = m_pos;// - m_vel * g_advanceTime;
	            Vector3 impactPos = (m_pos + lastPos) * 0.5f;
	            m_pos = impactPos;
	            m_pos.y = g_app->m_location->m_landscape.m_heightMap->GetValue(m_pos.x, m_pos.z);
                
                Vector3 normal = g_app->m_location->m_landscape.m_normalMap->GetValue(m_pos.x, m_pos.z);
                float dotProd = normal * m_vel;
                m_vel -= normal * 2.0f * dotProd * 0.7f;
            }        
        }
    }
    
    return false;
}


// *** Render
void Particle::Render(float _predictionTime)
{
    double birthTime = m_birthTime;
	double deathTime = m_birthTime + m_types[m_typeId].m_life;
    float startFade = birthTime + m_types[m_typeId].m_life * 0.75f;
    int alpha;
    if( g_gameTime < startFade )
    {
        alpha = 90;
    }
    else
    {                
        float fractionFade = (g_gameTime - startFade) / (deathTime - startFade);
        alpha = 90 - 90 * fractionFade;
        if( alpha < 0 ) alpha = 0;
    }
                  
    Vector3 predictedPos = m_pos + _predictionTime * m_vel;
    float size = m_size/16.0f;
	Vector3 up(g_app->m_camera->GetUp() * size);
	Vector3 right(g_app->m_camera->GetRight() * size);

    if( m_typeId == TypeMissileTrail )
    {
        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR );
        float fraction = (float) alpha / 100.0f;
        glColor4ub(m_colour.r*fraction, m_colour.g*fraction, m_colour.b*fraction, 0.0f );            
    }
    else
    {
        glBlendFunc ( GL_SRC_ALPHA, GL_ONE );
        glColor4ub(m_colour.r, m_colour.g, m_colour.b, alpha );            
    }

    glBegin( GL_QUADS );
		glTexCoord2i(0, 0);
        glVertex3fv( (predictedPos - up).GetData() );

		glTexCoord2i(0, 1);
        glVertex3fv( (predictedPos + right).GetData() );
        
		glTexCoord2i(1, 1);
		glVertex3fv( (predictedPos + up).GetData() );
        
		glTexCoord2i(1, 0);
		glVertex3fv( (predictedPos - right).GetData() );
    glEnd();
}


// *** SetupParticles
void Particle::SetupParticles()
{
	m_types[TypeRocketTrail].m_life = 2.0f;
	m_types[TypeRocketTrail].m_size = 15.0f;
	m_types[TypeRocketTrail].m_gravity = 15.0f;
	m_types[TypeRocketTrail].m_friction = 0.6f;
    m_types[TypeRocketTrail].m_colour1.Set( 128, 128, 128 );
    m_types[TypeRocketTrail].m_colour2.Set( 200, 200, 200 );

	m_types[TypeExplosionCore].m_life = 2.0f;
	m_types[TypeExplosionCore].m_size = 150.0f;
	m_types[TypeExplosionCore].m_gravity = 10.0f;
	m_types[TypeExplosionCore].m_friction = 0.2f;    
    m_types[TypeExplosionCore].m_colour1.Set( 200, 100, 100 );
    m_types[TypeExplosionCore].m_colour2.Set( 255, 120, 120 );
	
    m_types[TypeExplosionDebris].m_life = 6.0f;
	m_types[TypeExplosionDebris].m_size = 40.0f;
	m_types[TypeExplosionDebris].m_gravity = 20.0f;
	m_types[TypeExplosionDebris].m_friction = 0.2f;
    m_types[TypeExplosionDebris].m_colour1.Set( 200, 128, 128 );
    m_types[TypeExplosionDebris].m_colour2.Set( 250, 200, 200 );
    
    m_types[TypeMuzzleFlash].m_life = 1.0f;
	m_types[TypeMuzzleFlash].m_size = 10.0f;
	m_types[TypeMuzzleFlash].m_gravity = 0.0f;
	m_types[TypeMuzzleFlash].m_friction = 0.2f;
    m_types[TypeMuzzleFlash].m_colour1.Set( 255, 128, 128 );
    m_types[TypeMuzzleFlash].m_colour2.Set( 200, 100, 100 );

    m_types[TypeFire].m_life = 4.0f;
	m_types[TypeFire].m_size = 50.0f;
	m_types[TypeFire].m_gravity = -4.0f;
	m_types[TypeFire].m_friction = 0.0f;
    m_types[TypeFire].m_colour1.Set( 150, 50, 50 );
    m_types[TypeFire].m_colour2.Set( 150, 120, 50 );

    m_types[TypeControlFlash].m_life = 1.0f;
	m_types[TypeControlFlash].m_size = 30.0f;
	m_types[TypeControlFlash].m_gravity = -2.0f;
	m_types[TypeControlFlash].m_friction = 0.0f;
    m_types[TypeControlFlash].m_colour1.Set( 50, 50, 150 );
    m_types[TypeControlFlash].m_colour2.Set( 50, 50, 250 );

    m_types[TypeSpark].m_life = 4.0f;
	m_types[TypeSpark].m_size = 15.0f;
	m_types[TypeSpark].m_gravity = 15.0f;
	m_types[TypeSpark].m_friction = 1.5f;
    m_types[TypeSpark].m_colour1.Set( 250, 200, 0 );
    m_types[TypeSpark].m_colour2.Set( 250, 200, 50 );

    m_types[TypeBlueSpark].m_life = 4.0f;
	m_types[TypeBlueSpark].m_size = 15.0f;
	m_types[TypeBlueSpark].m_gravity = 15.0f;
	m_types[TypeBlueSpark].m_friction = 1.5f;
    m_types[TypeBlueSpark].m_colour1.Set( 50, 50, 200 );
    m_types[TypeBlueSpark].m_colour2.Set( 50, 70, 255 );

    m_types[TypeBrass].m_life = 2.0f;
	m_types[TypeBrass].m_size = 4.0f;
	m_types[TypeBrass].m_gravity = 30.0f;
	m_types[TypeBrass].m_friction = 0.5f;
    m_types[TypeBrass].m_colour1.Set( 250, 200, 0 );
    m_types[TypeBrass].m_colour2.Set( 250, 200, 50 );

    m_types[TypeMissileTrail].m_life = 5.0f;
	m_types[TypeMissileTrail].m_size = 200.0f;
	m_types[TypeMissileTrail].m_gravity = 1.0f;
	m_types[TypeMissileTrail].m_friction = 0.0f;
    m_types[TypeMissileTrail].m_colour1.Set( 100, 100, 100 );
    m_types[TypeMissileTrail].m_colour2.Set( 200, 200, 200 );

    m_types[TypeMissileFire].m_life = 0.5f;
	m_types[TypeMissileFire].m_size = 300.0f;
	m_types[TypeMissileFire].m_gravity = 0.0f;
	m_types[TypeMissileFire].m_friction = 0.0f;
    m_types[TypeMissileFire].m_colour1.Set( 150, 50, 50 );
    m_types[TypeMissileFire].m_colour2.Set( 150, 120, 50 );

    m_types[TypeDarwinianFire].m_life = 1.0f;
	m_types[TypeDarwinianFire].m_size =25.0f;
	m_types[TypeDarwinianFire].m_gravity = -10.0f;
	m_types[TypeDarwinianFire].m_friction = 0.0f;
    m_types[TypeDarwinianFire].m_colour1.Set( 150, 50, 50 );
    m_types[TypeDarwinianFire].m_colour2.Set( 150, 120, 50 );

	m_types[TypeLeaf].m_life = 60.0f;
	m_types[TypeLeaf].m_size = 25.0f;
	m_types[TypeLeaf].m_gravity = 5.0f;
	m_types[TypeLeaf].m_friction = 1.5f;
    m_types[TypeLeaf].m_colour1.Set( 50, 150, 50 );
    m_types[TypeLeaf].m_colour2.Set( 50, 200, 50 );

}


// ****************************************************************************
// ParticleSystem
// ****************************************************************************

// *** Constructor
ParticleSystem::ParticleSystem()
{
    m_particles.SetSize( 1500 );
    m_particles.SetStepSize( 200 );
	m_particles.SetTotalNumSlices(NUM_SLICES_PER_FRAME);

	Particle::SetupParticles();
}


// *** CreateParticle
void ParticleSystem::CreateParticle(Vector3 const &_pos, Vector3 const &_vel, 
                                    int _typeId, float _size, RGBAColour col)
{
	Particle *aParticle = m_particles.GetPointer();
    aParticle->Initialise(_pos, _vel, _typeId, _size);
	if( col != NULL)
	{
		aParticle->m_colour = col;
	}
}


// *** Advance
void ParticleSystem::Advance(int _slice)
{   
    START_PROFILE(g_app->m_profiler, "Advance Particles");

    int lower, upper;
    m_particles.GetNextSliceBounds( _slice, &lower, &upper );
    for( int i = lower; i <= upper; ++i )
    {
        if( m_particles.ValidIndex(i) )
        {
            Particle *p = m_particles.GetPointer(i);
			if (p->Advance())
			{
				m_particles.MarkNotUsed(i);
			}
		}
    }

    END_PROFILE(g_app->m_profiler, "Advance Particles");
}


// *** Render
void ParticleSystem::Render()
{
    START_PROFILE(g_app->m_profiler, "Render Particles");

    glDisable   ( GL_CULL_FACE );
    glBlendFunc ( GL_SRC_ALPHA, GL_ONE );

    glEnable    ( GL_BLEND );
	glEnable	( GL_TEXTURE_2D );
	glBindTexture(GL_TEXTURE_2D, g_app->m_resource->GetTexture("textures/particle.bmp"));
	glTexParameteri	(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    glDepthMask ( false );
    

	// Render all the particles that are up-to-date with server advances
    int lastUpdated = m_particles.GetLastUpdated();
  	int size = m_particles.Size();

    for (int i = 0; i < size; i++)
	{
        if (m_particles.ValidIndex(i))
        {
            Particle *p = m_particles.GetPointer(i);

            if( i <= lastUpdated )
            {
			    p->Render(g_predictionTime);
            }
            else
            {
                p->Render(g_predictionTime+SERVER_ADVANCE_PERIOD);
            }
        }
	}

    glDepthMask ( true );
	glDisable	( GL_TEXTURE_2D );
	glTexParameteri	( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glDisable   ( GL_BLEND );
    glBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glEnable    ( GL_CULL_FACE );

    END_PROFILE(g_app->m_profiler, "Render Particles");
}


// *** Empty
void ParticleSystem::Empty()
{
	m_particles.Empty();
}
