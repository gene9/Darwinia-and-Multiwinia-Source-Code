#include "lib/universal_include.h"


#include <math.h>

#include "lib/debug_utils.h"
#include "lib/hi_res_time.h"
#include "lib/math_utils.h"
#include "lib/profiler.h"
#include "lib/resource.h"
#include "lib/math/random_number.h"

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
:	m_spin(0.0),
	m_fadeInTime(0.0),
	m_fadeOutTime(0.0),
	m_life(10.0),
	m_size(1000.0),
	m_friction(0.0),
	m_gravity(0.0),
	m_colour1(0.5, 0.5, 0.5),
	m_colour2(0.95, 0.95, 0.95)
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
	AppDebugAssert(_typeId < Particle::TypeNumTypes);

	m_pos = _pos;
	m_vel = _vel;
	m_typeId = _typeId;

	ParticleType const &type = m_types[_typeId];
	float amount = frand();
	m_colour = type.m_colour1 * amount + type.m_colour2 * (1.0 - amount);
    
    if( _size == -1 )
    {
        m_size = type.m_size;
    }
    else
    {
        m_size = _size;
    }

	m_birthTime = g_gameTimer.GetGameTime();
}


// *** Advance
// Returns true if this particle should be removed from the particle list
bool Particle::Advance()
{   
	ParticleType const &type = m_types[m_typeId];
	double timeToDie = m_birthTime + m_types[m_typeId].m_life;
	if (g_gameTimer.GetGameTime() >= timeToDie)
	{
		return true;
	}

	m_pos += m_vel * SERVER_ADVANCE_PERIOD;
	
	float amount = SERVER_ADVANCE_PERIOD * type.m_friction;
	m_vel *= (1.0 - amount);

	if (type.m_gravity != 0.0)
	{
        float gravityScale = m_size / m_types[m_typeId].m_size;
		m_vel.y -= SERVER_ADVANCE_PERIOD * type.m_gravity * gravityScale;
	}


    //
    // Particles that spawn particles

    bool particleCreated = false;

     //
    // Particles that die on contact with the ground

    if( m_typeId == TypeFireball ||
        m_typeId == TypeVolcano)
    {
        if( m_pos.y <= g_app->m_location->m_landscape.m_heightMap->GetValue(m_pos.x, m_pos.z) )
        {
            return true;
        }
    }

    if( m_typeId == TypeExplosionDebris )
    {
        if( frand() < 0.5 * SERVER_ADVANCE_PERIOD * 10.0 )
        {
			// *** WARNING ***
			// CreateParticle might have to resize the particles array, which
			// might invalidate what "p" points to, so we need to make local
			// copies of the bits of p that we need
			Vector3 pos = m_pos;
			Vector3 vel = m_vel / 5.0;
            g_app->m_particleSystem->CreateParticle( pos, vel, Particle::TypeMissileTrail, m_size/1.5 );
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
			m_typeId == TypeLeaf  )
        {
            float landHeight = g_app->m_location->m_landscape.m_heightMap->GetValue(m_pos.x, m_pos.z);
            if( m_pos.y <= landHeight )
            {
	            Vector3 lastPos = m_pos;// - m_vel * g_advanceTime;
	            Vector3 impactPos = (m_pos + lastPos) * 0.5;
	            m_pos = impactPos;
	            m_pos.y = g_app->m_location->m_landscape.m_heightMap->GetValue(m_pos.x, m_pos.z);
                
                Vector3 normal = g_app->m_location->m_landscape.m_normalMap->GetValue(m_pos.x, m_pos.z);
                float dotProd = normal * m_vel;
                m_vel -= normal * 2.0 * dotProd * 0.7;
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
    float startFade = birthTime + m_types[m_typeId].m_life * 0.75;
    float timeNow = g_gameTimer.GetGameTime() + _predictionTime;
    int alpha;
    if( timeNow < startFade )
    {
        alpha = 90;
    }
    else
    {                
        float fractionFade = (timeNow - startFade) / (deathTime - startFade);
        alpha = 90 - 90 * fractionFade;
        if( alpha < 0 ) alpha = 0;
    }
                  
    Vector3 predictedPos = m_pos + _predictionTime * m_vel;
    float size = m_size/16.0;
	Vector3 up(g_app->m_camera->GetUp() * size);
	Vector3 right(g_app->m_camera->GetRight() * size);

    if( m_typeId == TypeMissileTrail ||
        m_typeId == TypeExplosionDebris )
    {
        float fraction = (float) alpha / 100.0;
        glColor4ub(m_colour.r*fraction, m_colour.g*fraction, m_colour.b*fraction, 0.0 );            
    }
    else
    {
        glColor4ub(m_colour.r, m_colour.g, m_colour.b, alpha );            
    }

    //glBegin( GL_QUADS );
		glTexCoord2i(0, 0);
        glVertex3dv( (predictedPos - up).GetData() );

		glTexCoord2i(0, 1);
        glVertex3dv( (predictedPos + right).GetData() );
        
		glTexCoord2i(1, 1);
		glVertex3dv( (predictedPos + up).GetData() );
        
		glTexCoord2i(1, 0);
		glVertex3dv( (predictedPos - right).GetData() );
    //glEnd();

}


// *** SetupParticles
void Particle::SetupParticles()
{
	m_types[TypeRocketTrail].m_life = 2.0;
	m_types[TypeRocketTrail].m_size = 15.0;
	m_types[TypeRocketTrail].m_gravity = 15.0;
	m_types[TypeRocketTrail].m_friction = 0.6;
    m_types[TypeRocketTrail].m_colour1.Set( 128, 128, 128 );
    m_types[TypeRocketTrail].m_colour2.Set( 200, 200, 200 );

	m_types[TypeExplosionCore].m_life = 2.0;
	m_types[TypeExplosionCore].m_size = 150.0;
	m_types[TypeExplosionCore].m_gravity = 10.0;
	m_types[TypeExplosionCore].m_friction = 0.2;    
    m_types[TypeExplosionCore].m_colour1.Set( 200, 100, 100 );
    m_types[TypeExplosionCore].m_colour2.Set( 255, 120, 120 );
	
    m_types[TypeExplosionDebris].m_life = 6.0;
	m_types[TypeExplosionDebris].m_size = 40.0;
	m_types[TypeExplosionDebris].m_gravity = 20.0;
	m_types[TypeExplosionDebris].m_friction = 0.2;
    m_types[TypeExplosionDebris].m_colour1.Set( 200, 128, 128 );
    m_types[TypeExplosionDebris].m_colour2.Set( 250, 200, 200 );
    
    m_types[TypeMuzzleFlash].m_life = 1.0;
	m_types[TypeMuzzleFlash].m_size = 10.0;
	m_types[TypeMuzzleFlash].m_gravity = 0.0;
	m_types[TypeMuzzleFlash].m_friction = 0.2;
    m_types[TypeMuzzleFlash].m_colour1.Set( 255, 128, 128 ); 
    m_types[TypeMuzzleFlash].m_colour2.Set( 200, 100, 100 );

    m_types[TypeFire].m_life = 4.0;
	m_types[TypeFire].m_size = 50.0;
	m_types[TypeFire].m_gravity = -4.0;
	m_types[TypeFire].m_friction = 0.0;
    m_types[TypeFire].m_colour1.Set( 150, 50, 50 );
    m_types[TypeFire].m_colour2.Set( 150, 120, 50 );

    m_types[TypeControlFlash].m_life = 1.0;
	m_types[TypeControlFlash].m_size = 30.0;
	m_types[TypeControlFlash].m_gravity = -2.0;
	m_types[TypeControlFlash].m_friction = 0.0;
    m_types[TypeControlFlash].m_colour1.Set( 50, 50, 150 );
    m_types[TypeControlFlash].m_colour2.Set( 50, 50, 250 );

    m_types[TypeSpark].m_life = 4.0;
	m_types[TypeSpark].m_size = 15.0;
	m_types[TypeSpark].m_gravity = 15.0;
	m_types[TypeSpark].m_friction = 1.5;
    m_types[TypeSpark].m_colour1.Set( 250, 200, 0 );
    m_types[TypeSpark].m_colour2.Set( 250, 200, 50 );

    m_types[TypeBlueSpark].m_life = 4.0;
	m_types[TypeBlueSpark].m_size = 15.0;
	m_types[TypeBlueSpark].m_gravity = 15.0;
	m_types[TypeBlueSpark].m_friction = 1.5;
    m_types[TypeBlueSpark].m_colour1.Set( 50, 50, 200 );
    m_types[TypeBlueSpark].m_colour2.Set( 50, 70, 255 );

    m_types[TypeBrass].m_life = 2.0;
	m_types[TypeBrass].m_size = 4.0;
	m_types[TypeBrass].m_gravity = 30.0;
	m_types[TypeBrass].m_friction = 0.5;
    m_types[TypeBrass].m_colour1.Set( 250, 200, 0 );
    m_types[TypeBrass].m_colour2.Set( 250, 200, 50 );

    m_types[TypeMissileTrail].m_life = 5.0;
	m_types[TypeMissileTrail].m_size = 200.0;
	m_types[TypeMissileTrail].m_gravity = 1.0;
	m_types[TypeMissileTrail].m_friction = 0.0;
    m_types[TypeMissileTrail].m_colour1.Set( 100, 100, 100 );
    m_types[TypeMissileTrail].m_colour2.Set( 200, 200, 200 );

    m_types[TypeMissileFire].m_life = 0.5;
	m_types[TypeMissileFire].m_size = 300.0;
	m_types[TypeMissileFire].m_gravity = 0.0;
	m_types[TypeMissileFire].m_friction = 0.0;
    m_types[TypeMissileFire].m_colour1.Set( 150, 50, 50 );
    m_types[TypeMissileFire].m_colour2.Set( 150, 120, 50 );

    m_types[TypeDarwinianFire].m_life = 1.0;
	m_types[TypeDarwinianFire].m_size =25.0;
	m_types[TypeDarwinianFire].m_gravity = -10.0;
	m_types[TypeDarwinianFire].m_friction = 0.0;
    m_types[TypeDarwinianFire].m_colour1.Set( 150, 50, 50 );
    m_types[TypeDarwinianFire].m_colour2.Set( 150, 120, 50 );

	m_types[TypeLeaf].m_life = 60.0;
	m_types[TypeLeaf].m_size = 25.0;
	m_types[TypeLeaf].m_gravity = 5.0;
	m_types[TypeLeaf].m_friction = 1.5;
    m_types[TypeLeaf].m_colour1.Set( 50, 150, 50 );
    m_types[TypeLeaf].m_colour2.Set( 50, 200, 50 );

    m_types[TypePlague].m_life = 2.0;
	m_types[TypePlague].m_size = 25.0;
	m_types[TypePlague].m_gravity = -0.5;
	m_types[TypePlague].m_friction = 0.0;
    m_types[TypePlague].m_colour1.Set( 40, 200, 40 );
    m_types[TypePlague].m_colour2.Set( 50, 200, 50 );

    m_types[TypeHotFeet].m_life = 1.0;
	m_types[TypeHotFeet].m_size =25.0;
	m_types[TypeHotFeet].m_gravity = 0.0;
	m_types[TypeHotFeet].m_friction = 0.0;
    m_types[TypeHotFeet].m_colour1.Set( 150, 50, 50 );
    m_types[TypeHotFeet].m_colour2.Set( 150, 120, 50 );

    m_types[TypeSelectionMarker].m_life = 0.3;
	m_types[TypeSelectionMarker].m_size = 10.0;
	m_types[TypeSelectionMarker].m_gravity = 0.0;
	m_types[TypeSelectionMarker].m_friction = 0.2;
    m_types[TypeSelectionMarker].m_colour1.Set( 255, 128, 128 );
    m_types[TypeSelectionMarker].m_colour2.Set( 200, 100, 100 );

    m_types[TypeFireball].m_life = 1.0;
	m_types[TypeFireball].m_size = 25.0;
	m_types[TypeFireball].m_gravity = 0.0;
	m_types[TypeFireball].m_friction = 0.0;
    m_types[TypeFireball].m_colour1.Set( 150, 50, 50 );
    m_types[TypeFireball].m_colour2.Set( 150, 120, 50 );

    m_types[TypeVolcano].m_life = 12.0;
	m_types[TypeVolcano].m_size = 400.0;
	m_types[TypeVolcano].m_gravity = 10.0;
	m_types[TypeVolcano].m_friction = 0.0;
    m_types[TypeVolcano].m_colour1.Set( 150, 50, 50 );
    m_types[TypeVolcano].m_colour2.Set( 150, 120, 50 );

    m_types[TypePortalFire].m_life = 2.0;
	m_types[TypePortalFire].m_size = 50.0;
	m_types[TypePortalFire].m_gravity = -4.0;
	m_types[TypePortalFire].m_friction = 0.0;
    m_types[TypePortalFire].m_colour1.Set( 100, 50, 50 );
    m_types[TypePortalFire].m_colour2.Set( 120, 100, 50 );

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
    if( g_app->GamePaused() ) return;

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
    START_PROFILE( "Advance Particles");

    if( _slice == 0 )
    {        
        for( int i = 0; i < m_particles.Size(); ++i )
        {
            if( m_particles.ValidIndex(i) )
            {
                Particle *p = m_particles.GetPointer(i);
                if (p->Advance())
                {
                    m_particles.RemoveData(i);
                }
            }
        }
    }


  //  int lower, upper;
  //  m_particles.GetNextSliceBounds( _slice, &lower, &upper );
  //  for( int i = lower; i <= upper; ++i )
  //  {
  //      if( m_particles.ValidIndex(i) )
  //      {
  //          Particle *p = m_particles.GetPointer(i);
		//	if (p->Advance())
		//	{
		//		m_particles.RemoveData(i);
		//	}
		//}
  //  }

    END_PROFILE( "Advance Particles");
}


// *** Render
void ParticleSystem::Render()
{
#ifdef TESTBED_ENABLED
	if( !g_app->GetTestBedRendering() )
		return;
#endif
    START_PROFILE( "Render Particles");

    glDisable   ( GL_CULL_FACE );
    glBlendFunc ( GL_SRC_ALPHA, GL_ONE );

    glEnable        ( GL_BLEND );
	glEnable	    ( GL_TEXTURE_2D );
	glBindTexture   (GL_TEXTURE_2D, g_app->m_resource->GetTexture("textures/particle.bmp"));
	glTexParameteri	(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );    
    glDepthMask     ( false );

	// Render all the particles that are up-to-date with server advances
    int lastUpdated = m_particles.GetLastUpdated();
  	int size = m_particles.Size();

	// Render Missile Trails and Debris Explosion first

	glBlendFunc		( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR );
    glBegin         (GL_QUADS);
    for (int i = 0; i < size; i++)
	{
        if (m_particles.ValidIndex(i))
        {
            Particle *p = m_particles.GetPointer(i);
            if( !g_app->m_camera->PosInViewFrustum( p->m_pos ) ) continue;

			if( p->m_typeId == Particle::TypeMissileTrail ||
				p->m_typeId == Particle::TypeExplosionDebris )
			{
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
	}
	glEnd			();

	// Render other particles
	glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );
    glBegin         (GL_QUADS);
    for (int i = 0; i < size; i++)
	{
        if (m_particles.ValidIndex(i))
        {
            Particle *p = m_particles.GetPointer(i);

			if( p->m_typeId != Particle::TypeMissileTrail &&
				p->m_typeId != Particle::TypeExplosionDebris )
			{
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
	}
	glEnd			();

	glDepthMask		( true );
	glDisable		( GL_TEXTURE_2D );
	glTexParameteri	( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glDisable		( GL_BLEND );
    glBlendFunc		( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glEnable		( GL_CULL_FACE );

    END_PROFILE( "Render Particles");
}


// *** Empty
void ParticleSystem::Empty()
{
	m_particles.Empty();
}
