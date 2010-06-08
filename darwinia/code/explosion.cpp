#include "lib/universal_include.h"


#include "lib/fast_darray.h"
#include "lib/math_utils.h"
#include "lib/profiler.h"
#include "lib/resource.h"

#include "app.h"
#include "explosion.h"
#include "globals.h"
#include "main.h"
#include "renderer.h"


//#define EXPLOSION_LIFETIME		50.0f
//#define MAX_INITIAL_SPEED		0.01f
//#define MAX_ANG_VEL				0.01f
//#define ACCEL_DUE_TO_GRAV		0.0f
//#define INITIAL_VERTICAL_SPEED	0.0f
//#define MIN_FRAG_LIFE			0.9f			// As a fraction of EXPLOSION_LIFETIME
#define EXPLOSION_LIFETIME		5.0f
#define MAX_INITIAL_SPEED		3.0f
#define MAX_ANG_VEL				4.0f
#define ACCEL_DUE_TO_GRAV		(-GRAVITY)
#define INITIAL_VERTICAL_SPEED	10.0f
#define MIN_FRAG_LIFE			0.0f			// As a fraction of EXPLOSION_LIFETIME
#define FRICTION_COEF			0.05f			// Bigger means more friction
#define ROT_FRICTION_COEF		0.2f			// Bigger means more rotational friction
#define NUM_TUMBLERS			5


//*****************************************************************************
// Class Tumbler
//*****************************************************************************

Tumbler::Tumbler()
{
	m_rotMat.SetToIdentity();
	m_angVel.x = sfrand(MAX_ANG_VEL);
	m_angVel.y = sfrand(MAX_ANG_VEL);
	m_angVel.z = sfrand(MAX_ANG_VEL);
}


void Tumbler::Advance()
{
	Matrix33 rotIncrement(m_angVel.x * g_advanceTime,
						  m_angVel.y * g_advanceTime,
						  m_angVel.z * g_advanceTime);
	m_rotMat *= rotIncrement;

	// Rotational Friction
	m_angVel *= 1.0f - (g_advanceTime * ROT_FRICTION_COEF);
}


//*****************************************************************************
// Class Explosion
//*****************************************************************************

Explosion::Explosion(ShapeFragment *_frag, Matrix34 const &_transform, float _fraction)
:	m_numTumblers(NUM_TUMBLERS),
	m_numTris(0),
    m_tris(NULL)
{
	m_tumblers = new Tumbler[m_numTumblers];

	m_timeToDie = g_gameTime + EXPLOSION_LIFETIME;

	FastDArray <ExplodingTri> triangles;

	Matrix34 totalTransform = _frag->m_transform * _transform;
	Vector3 transformedFragCentre = totalTransform * _frag->m_centre;

	for (int j = 0; j < _frag->m_numTriangles; ++j)
	{
        if( _fraction < 1.0f && frand(1.0f) > _fraction )
        {
            continue;
        }

		ShapeTriangle const &shapeTri = _frag->m_triangles[j];
		VertexPosCol const &v1 = _frag->m_vertices[shapeTri.v1];
		VertexPosCol const &v2 = _frag->m_vertices[shapeTri.v2];
		VertexPosCol const &v3 = _frag->m_vertices[shapeTri.v3];

		ExplodingTri tri;
		tri.m_colour = _frag->m_colours[v1.m_colId];
		tri.m_v1 = _frag->m_positions[v1.m_posId] * totalTransform;
		tri.m_v2 = _frag->m_positions[v2.m_posId] * totalTransform;
		tri.m_v3 = _frag->m_positions[v3.m_posId] * totalTransform;

		if (tri.m_v1 == tri.m_v2 || tri.m_v2 == tri.m_v3 || tri.m_v1 == tri.m_v3)
		{
			continue;
		}

        float circum = (tri.m_v1 - tri.m_v2).Mag() +
                       (tri.m_v2 - tri.m_v3).Mag() +
                       (tri.m_v3 - tri.m_v1).Mag();

        if( circum < 6.0f )
        {
            // Too small to care about
            continue;
        }

		Vector3 centre = (tri.m_v1 + tri.m_v2 + tri.m_v3) * 0.3333f;

		tri.m_v1 -= centre;
		tri.m_v2 -= centre;
		tri.m_v3 -= centre;

		Vector3 a = tri.m_v1 - tri.m_v2;
		Vector3 b = tri.m_v2 - tri.m_v3;
		tri.m_norm = (a ^ b).Normalise();
		if (j & 1) tri.m_norm = -tri.m_norm;

		tri.m_vel = (centre - transformedFragCentre) * (MAX_INITIAL_SPEED);
		tri.m_vel.y += INITIAL_VERTICAL_SPEED;
		tri.m_pos = centre;
		tri.m_tumbler = &m_tumblers[darwiniaRandom() % NUM_TUMBLERS];
		float const minFragLife = EXPLOSION_LIFETIME * MIN_FRAG_LIFE;
		//tri.m_timeToDie = frand(EXPLOSION_LIFETIME - minFragLife) + g_gameTime + minFragLife;
        tri.m_timeToDie = g_gameTime + EXPLOSION_LIFETIME;

		triangles.PutData(tri);
	}
    
	m_numTris = triangles.Size();
    if( m_numTris > 0 )
    {
	    m_tris = new ExplodingTri[m_numTris];
	    memcpy(m_tris, triangles.GetPointer(0), sizeof(ExplodingTri) * m_numTris);
    }
}


Explosion::~Explosion()
{
	m_numTumblers = 0;
	delete [] m_tumblers;
	m_tumblers = NULL;

	m_numTris = 0;
	delete [] m_tris;
	m_tris = NULL;
}


bool Explosion::Advance()
{
	float deltaVelY = ACCEL_DUE_TO_GRAV * g_advanceTime;

	// Advance all tumblers
	for (int i = 0; i < m_numTumblers; ++i)
	{
		m_tumblers[i].Advance();
	}

	// Advance all ExplodingTriangles
	for (int i = 0; i < m_numTris; ++i)
	{
		if (g_gameTime > m_tris[i].m_timeToDie) continue;

		m_tris[i].m_pos += m_tris[i].m_vel * g_advanceTime;
	
		// Friction
		float speed = m_tris[i].m_vel.Mag();
		float friction = speed * FRICTION_COEF * g_advanceTime;
		if (friction > 1.0f) friction = 1.0f;
		m_tris[i].m_vel *= 1.0f - friction;

		// Gravity
		m_tris[i].m_vel.y += deltaVelY;
	}

	if (g_gameTime > m_timeToDie)
	{
		return true;
	}

	return false;
}


void Explosion::Render()
{
	// someone tries to render explosion with 0 tris
	// happens at "kill all enemies"
	if(!m_numTris) return;

	float age = (EXPLOSION_LIFETIME + (g_gameTime - m_timeToDie)) / EXPLOSION_LIFETIME;

	glBegin(GL_TRIANGLES);
	for (int i = 0; i < m_numTris; ++i)
	{
		if (g_gameTime > m_tris[i].m_timeToDie) continue;

		Vector3 const norm(m_tris[i].m_tumbler->m_rotMat * m_tris[i].m_norm);
		Vector3 const v1(m_tris[i].m_tumbler->m_rotMat * m_tris[i].m_v1 + m_tris[i].m_pos);
		Vector3 const v2(m_tris[i].m_tumbler->m_rotMat * m_tris[i].m_v2 + m_tris[i].m_pos);
		Vector3 const v3(m_tris[i].m_tumbler->m_rotMat * m_tris[i].m_v3 + m_tris[i].m_pos);
	
		glColor4ub(m_tris[i].m_colour.r, 
                   m_tris[i].m_colour.g,
                   m_tris[i].m_colour.b,
                   (1.0f-age) * 255);
        
		glNormal3fv(norm.GetDataConst());

        glTexCoord2i( 0, 0 );       glVertex3fv(v1.GetDataConst());
		glTexCoord2i( 0, 1 );       glVertex3fv(v2.GetDataConst());
		glTexCoord2i( 1, 1 );       glVertex3fv(v3.GetDataConst());
	}
	glEnd();
}

Vector3 Explosion::GetCenter() const
{
	Vector3 sum = Vector3(0,0,0);
	unsigned summed = 0;
	for (int i = 0; i < m_numTris; ++i)
	{
		if (g_gameTime > m_tris[i].m_timeToDie) continue;
		sum += m_tris[i].m_pos;
		summed++;
	}
	if(summed) return sum/summed;
	DarwiniaDebugAssert(0);
	return sum;
}


//*****************************************************************************
// Class ExplosionManager
//*****************************************************************************

ExplosionManager g_explosionManager;


ExplosionManager::ExplosionManager()
{
}


ExplosionManager::~ExplosionManager()
{
	m_explosions.EmptyAndDelete();
}


void ExplosionManager::AddExplosion(ShapeFragment *_frag, 
									Matrix34 const &_transform, bool _recurse, float _fraction)
{
    if( _fraction <= 0.0f )
    {
        return;
    }
    
	if (_frag->m_numTriangles > 0)
	{
		Explosion *explosion = new Explosion(_frag, _transform, _fraction);
		m_explosions.PutData(explosion);
	}

	if (_recurse)
	{
		Matrix34 totalMatrix = _frag->m_transform * _transform;

		for (unsigned int i = 0; i < _frag->m_childFragments.Size(); ++i)
		{
			ShapeFragment *child = _frag->m_childFragments[i];
			AddExplosion(child, totalMatrix, true, _fraction);
		}
	}
}


void ExplosionManager::AddExplosion(Shape *_shape, Matrix34 const &_transform, float _fraction)
{
    if( _fraction > 0.0f ) AddExplosion(_shape->m_rootFragment, _transform, true, _fraction);
}


void ExplosionManager::Reset()
{
	m_explosions.EmptyAndDelete();
}


void ExplosionManager::Advance()
{
	START_PROFILE(g_app->m_profiler, "Advance Explosions");

	for (unsigned int i = 0; i < m_explosions.Size(); ++i)
	{
		if (m_explosions[i]->Advance())
		{
			Explosion *explosion = m_explosions[i];
			m_explosions.RemoveData(i);
			delete explosion;
			--i;
		}
	}

	END_PROFILE(g_app->m_profiler, "Advance Explosions");
}


void ExplosionManager::Render()
{
	START_PROFILE(g_app->m_profiler, "Render Explosions");

	int numExplosions = m_explosions.Size();

	if (numExplosions > 0) {

		CHECK_OPENGL_STATE();

		glEnable        (GL_TEXTURE_2D );
		glBindTexture   (GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/shapewireframe.bmp" ) );
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexEnvf       (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);

		g_app->m_renderer->SetObjectLighting();

		glEnable        (GL_COLOR_MATERIAL);
		glDisable       (GL_CULL_FACE);
		glEnable        (GL_BLEND );

		for (unsigned int i = 0; i < numExplosions; ++i)
		{
			m_explosions[i]->Render();
		}

		glBlendFunc     (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
		glDisable       (GL_BLEND );
		glEnable        (GL_CULL_FACE);
		glDisable       (GL_COLOR_MATERIAL);
		g_app->m_renderer->UnsetObjectLighting();

		glTexEnvf       (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glDisable       (GL_TEXTURE_2D );

	}

	END_PROFILE(g_app->m_profiler, "Render Explosions");
}
