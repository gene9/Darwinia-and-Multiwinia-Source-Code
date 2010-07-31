#include "lib/universal_include.h"

#include "lib/debug_render.h"
#include "lib/file_writer.h"
#include "lib/math_utils.h"
#include "lib/profiler.h"
#include "lib/resource.h"
#include "lib/text_stream_readers.h"
#include "lib/debug_utils.h"
#include "lib/shape.h"
#include "lib/hi_res_time.h"
#include "lib/text_renderer.h"
#include "lib/preferences.h"

#include "worldobject/tree.h"
#include "worldobject/souldestroyer.h"
#include "worldobject/darwinian.h"

#include "sound/soundsystem.h"

#include "app.h"
#include "globals.h"
#include "camera.h"
#include "particle_system.h"
#include "location.h"
#include "level_file.h"
#include "obstruction_grid.h"
#include "entity_grid.h"
#include "global_world.h"
#include "main.h"

#ifdef USE_DIRECT3D
#include "lib/opengl_directx_internals.h"
#endif

Tree::Tree()
:   Building(),
    m_branchDisplayListId(-1),
    m_leafDisplayListId(-1),
    m_height(50.0f),
    m_iterations(6),
    m_budsize(1.0f),
    m_pushOut(1.0f),
    m_pushUp(1.0f),
    m_seed(0),
    m_leafColour(0xffffffff),
    m_branchColour(0xffffffff),
    m_onFire(0.0f),
    m_fireDamage(0.0f),
    m_burnSoundPlaying(false),
	m_leafDropRate(0),
	m_spiritDropRate(0),
	m_evil(0)
{
    m_type = TypeTree;
}

Tree::~Tree()
{
	DeleteDisplayLists();
}

void Tree::Initialise( Building *_template )
{
    Building::Initialise( _template );

    DarwiniaDebugAssert( _template->m_type == TypeTree );
    Tree *tree = (Tree *) _template;

    m_height = tree->m_height;
    m_iterations = tree->m_iterations;
    m_budsize = tree->m_budsize;
    m_pushOut = tree->m_pushOut;
    m_pushUp = tree->m_pushUp;
    m_seed = tree->m_seed;
    m_branchColour = tree->m_branchColour;
    m_leafColour = tree->m_leafColour;
	m_leafDropRate = tree->m_leafDropRate;
	m_spiritDropRate = tree->m_spiritDropRate;
	m_evil = tree->m_evil;
}


void Tree::SetDetail( int _detail )
{
    Building::SetDetail( _detail );

    int oldIterations = m_iterations;
    if( _detail == 0 ) m_iterations = 0;
    else m_iterations -= ( _detail - 1 );
    m_iterations = max( m_iterations, 3 );

    Generate();

    m_iterations = oldIterations;
    m_centrePos = m_pos + m_hitcheckCentre * m_height;
    m_radius = m_hitcheckRadius * m_height * 1.5f;
}


bool Tree::Advance()
{
    m_fireDamage -= SERVER_ADVANCE_PERIOD;
    if( m_fireDamage < 0.0f ) m_fireDamage = 0.0f;
    if( m_fireDamage > 100.0f ) m_fireDamage = 100.0f;

    if( m_onFire > 0.0f )
    {
        //
        // Spawn fire particle

        float actualHeight = GetActualHeight(0.0f);
        int numFire = actualHeight / 5;
        for( int i = 0; i < numFire; ++i )
        {
            Vector3 fireSpawn = m_pos + Vector3(0,actualHeight,0);
            fireSpawn += Vector3( sfrand(actualHeight*1.0f),
                                  sfrand(actualHeight*0.5f),
                                  sfrand(actualHeight*1.0f) );
            float fireSize = actualHeight*2.0f;
            fireSize *= (1.0f + sfrand(0.5f) );
            g_app->m_particleSystem->CreateParticle( fireSpawn, g_zeroVector, Particle::TypeFire, fireSize );
        }

        if( frand(100.0f) < 10.0f )
        {
            Vector3 fireSpawn = m_pos + Vector3(0,actualHeight,0);
            fireSpawn += Vector3( sfrand(actualHeight*0.75f),
                                  sfrand(actualHeight*0.75f),
                                  sfrand(actualHeight*0.75f) );
            g_app->m_particleSystem->CreateParticle( fireSpawn, g_zeroVector, Particle::TypeExplosionDebris );
        }

        //
        // Burn down

        m_onFire += SERVER_ADVANCE_PERIOD * 0.01f;
        if( m_onFire > 1.0f )
        {
            m_onFire = -1.0f;
        }

        //
        // Spread to nearby trees

        Vector3 hitCentre = m_pos + m_hitcheckCentre*actualHeight;
        float hitRadius = m_hitcheckRadius * actualHeight;

        for( int b = 0; b < g_app->m_location->m_buildings.Size(); ++b )
        {
            if( g_app->m_location->m_buildings.ValidIndex(b) )
            {
                Building *building = g_app->m_location->m_buildings[b];
                if( building != this &&
                    building->m_type == TypeTree )
                {
                    Tree *tree = (Tree *) building;
                    float distance = (tree->m_pos - m_pos).Mag();
                    float theirActualHeight = tree->GetActualHeight(0.0f);
                    float theirRadius = theirActualHeight * tree->m_hitcheckRadius * 1.5f;
                    float ourRadius = actualHeight * m_hitcheckRadius * 1.5f;

                    if( theirRadius + ourRadius >= distance )
                    {
                        tree->Damage(-1.0f);
                    }
                }
            }
        }

    }
    else if( m_onFire < 0.0f )
    {
        if( m_burnSoundPlaying )
        {
            g_app->m_soundSystem->StopAllSounds( m_id, "Tree Burn" );
            g_app->m_soundSystem->TriggerBuildingEvent( this, "Create" );
            m_burnSoundPlaying = false;
        }

        //
        // Regrow

        m_onFire += SERVER_ADVANCE_PERIOD * 0.01f;
        if( m_onFire > 0.0f )
        {
            m_onFire = 0.0f;
        }
    }

	if( m_onFire == 0.0f )
	{
		if ( m_leafDropRate > 0 )
		{
			// drop some leaves
			if( rand() % (51-m_leafDropRate) == 0 )
			{
				float actualHeight = GetActualHeight(0.0f);
				Vector3 fireSpawn = m_pos + Vector3(0,actualHeight,0);
				fireSpawn += Vector3( sfrand(actualHeight*1.0f),
									  sfrand(actualHeight*0.25f),
									  sfrand(actualHeight*1.0f) );
				g_app->m_particleSystem->CreateParticle( fireSpawn, g_zeroVector, Particle::TypeLeaf, -1.0f, RGBAColour (m_leafColourArray[0], m_leafColourArray[1], m_leafColourArray[2] ) );
			}
		}
        if ( m_spiritDropRate > 0 && !m_evil )
        {
            // drop some spirits
            if( syncrand() % (51-m_spiritDropRate) == 0 )
            {
                double actualHeight = GetActualHeight(0.0);
			    Vector3 pos = Vector3(frand(actualHeight),actualHeight,0);
                pos.RotateAroundY( frand(2.0 * M_PI) );
                pos += m_pos;

                int teamId = -1;
                while( teamId == -1 )
                {
                    teamId = frand( NUM_TEAMS ); 
                }

                g_app->m_location->SpawnSpirit( pos, g_zeroVector, teamId, WorldObjectId() );
            }
        }
	}
	if ( m_evil ) { return AdvanceEvil(); }
    return false;
}

bool Tree::AdvanceEvil ()
{
    double range = GetActualHeight(0.0f) / 2.0;
    if( syncfrand(10.0) < 0.2 )
    {
		int myTeam = 255;
		for ( int i = 0; i < NUM_TEAMS; i++ ) { if ( g_app->m_location->m_levelFile->m_teamFlags[i] & TEAM_FLAG_EVILTREESPAWNTEAM ) { myTeam = i; } }

		WorldObjectId id = g_app->m_location->m_entityGrid->GetBestEnemy( m_pos.x, m_pos.z, 0, range, myTeam );

        Darwinian *darwinian = (Darwinian *)g_app->m_location->GetEntitySafe( id, Entity::TypeDarwinian );
        if( darwinian && darwinian->m_state != Darwinian::StateOperatingPort )
        {                    
            bool killed = false;
            bool dead = darwinian->m_dead;
            darwinian->ChangeHealth( -999 );            
            if( !dead && darwinian->m_dead ) killed = true;

            if( killed )
            {
                // Eat the spirit
                int spiritIndex = g_app->m_location->GetSpirit( darwinian->m_id );
                if( spiritIndex != -1 )
                {
                    g_app->m_location->m_spirits.MarkNotUsed( spiritIndex );
					if ( myTeam == 255 )
					{
						// Create a zombie
						Zombie *zombie = new Zombie();
						zombie->m_pos = darwinian->m_pos;
						zombie->m_front = darwinian->m_front;
						zombie->m_up = g_upVector;
						zombie->m_up.RotateAround( zombie->m_front * syncsfrand(1) );
						zombie->m_vel = m_vel * 0.5;
						zombie->m_vel.y = 20.0 + syncfrand(25.0);
						int index = g_app->m_location->m_effects.PutData( zombie );
						zombie->m_id.Set( darwinian->m_id.GetTeamId(), UNIT_EFFECTS, index, -1 );
						zombie->m_id.GenerateUniqueId();
					}
					else
					{
						// Create a deadwinian
						WorldObjectId wid = g_app->m_location->SpawnEntities(darwinian->m_pos, myTeam, -1, Entity::TypeDarwinian, 1, darwinian->m_vel*0.5,0);
						Darwinian *newguy = (Darwinian *) g_app->m_location->GetEntity(wid);
						newguy->m_front = darwinian->m_front;
						newguy->m_soulless = true;
					}
                }
            }                            
        }
    }
	return false;
}
float Tree::GetActualHeight( float _predictionTime )
{
    float predictedOnFire = m_onFire;
    if( predictedOnFire != 0.0f )
    {
        predictedOnFire += _predictionTime * 0.01f;
    }

    float actualHeight = m_height * (1.0f-fabs(predictedOnFire));
    if( actualHeight < m_height * 0.1f ) actualHeight = m_height * 0.1f;

    return actualHeight;
}

static void intToArray(unsigned x, unsigned char *a)
{
	a[0] = x & 0xFF;
	x >>= 8;
	a[1] = x & 0xFF;
	x >>= 8;
	a[2] = x & 0xFF;
	x >>= 8;
	a[3] = x & 0xFF;
}

void Tree::DeleteDisplayLists()
{
    if( m_branchDisplayListId != -1 )
    {
        glDeleteLists( m_branchDisplayListId, 1 );
        m_branchDisplayListId = -1;
    }

    if( m_leafDisplayListId != -1 )
    {
        glDeleteLists( m_leafDisplayListId, 1 );
        m_leafDisplayListId = -1;
    }
}

void Tree::Generate()
{
    float timeNow = GetHighResTime();

    m_hitcheckCentre.Zero();
    m_hitcheckRadius = 0.0f;
    m_numLeafs = 0;

	DeleteDisplayLists();

	intToArray(m_branchColour, m_branchColourArray);
	intToArray(m_leafColour, m_leafColourArray);
#ifdef USE_DIRECT3D
    if( m_evil )
    {
        m_branchColourArray[0] = 0;
        m_branchColourArray[1] = 0;
        m_branchColourArray[2] = 0;
        m_branchColourArray[3] = 200;

        m_leafColourArray[0] = 0;
        m_leafColourArray[1] = 0;
        m_leafColourArray[2] = 0;
        m_leafColourArray[3] = 0;
	}
#endif

    int treeDetail = g_prefsManager->GetInt( "RenderTreeDetail", 1 );
    if( treeDetail > 1 )
    {
        int alpha = m_leafColourArray[3];
        alpha *= pow( 1.3f, treeDetail );
        alpha = min( alpha, 255 );
        m_leafColourArray[3] = alpha;
    }

    darwiniaSeedRandom( m_seed );
    m_branchDisplayListId = glGenLists(1);
    glNewList       ( m_branchDisplayListId, GL_COMPILE );
    glBegin         ( GL_QUADS );
#ifdef USE_DIRECT3D
	// The colour is not supposed to be specified here so that it can be changed
	// easily in the editor (when experimenting).
	// Really, the Trees should be rewritten for Direct3D to use Meshes
    glColor4ubv     ( m_branchColourArray ); // Direct3D hack
#endif
    RenderBranch    ( g_zeroVector, g_upVector, m_iterations, false, true, false );
    glEnd           ();
    glEndList       ();

    darwiniaSeedRandom( m_seed );
    m_leafDisplayListId = glGenLists(1);
    glNewList       ( m_leafDisplayListId, GL_COMPILE );
#ifdef USE_DIRECT3D
    glColor4ubv     ( m_leafColourArray );	// Direct3D hack
#endif
    glBegin         ( GL_QUADS );
    RenderBranch    ( g_zeroVector, g_upVector, m_iterations, false, false, true );
    glEnd           ();
    glEndList       ();


    //
    // We now have all the leaf positions accumulated in m_hitcheckCentre
    // So we can calculate the actual centre position, then the radius

    m_hitcheckCentre /= (float) m_numLeafs;
    RenderBranch( g_zeroVector, g_upVector, m_iterations, true, false, true );
    m_hitcheckRadius *= 0.8f;

    float totalTime = GetHighResTime() - timeNow;
    DebugOut( "Tree generated in %dms\n", int(totalTime * 1000.0f) );
}

void Tree::Render( float _predictionTime )
{
    //RenderHitCheck();
}


bool Tree::PerformDepthSort( Vector3 &_centrePos )
{
    _centrePos = m_pos+m_hitcheckCentre*m_height;
    return true;
}


void Tree::RenderAlphas( float _predictionTime )
{
    if( m_branchDisplayListId == -1 ||
        m_leafDisplayListId == -1 )
    {
        Generate();
    }

    if( g_app->m_editing )
    {
	    intToArray(m_branchColour, m_branchColourArray);
	    intToArray(m_leafColour, m_leafColourArray);
    }

    float actualHeight = GetActualHeight( _predictionTime );

    glEnable        ( GL_TEXTURE_2D );
    glEnable        ( GL_BLEND );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTextureWithAlpha( "textures/tree.bmp" ) );
	glTexParameteri	( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri	( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
    glDisable       ( GL_CULL_FACE );
    glDepthMask     ( false );

    if( m_evil )
    {
        m_branchColourArray[0] = 0;
        m_branchColourArray[1] = 0;
        m_branchColourArray[2] = 0;
        m_branchColourArray[3] = 200;

        m_leafColourArray[0] = 0;
        m_leafColourArray[1] = 0;
        m_leafColourArray[2] = 0;
        m_leafColourArray[3] = 0;
                        
        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );        

        actualHeight *= ( 1.0 + sin((GetHighResTime() + m_id.GetUniqueId() * 2) * 2) * 0.01 );
    }
    else
    {
        actualHeight *= ( 1.0 + sin((GetHighResTime() + m_id.GetUniqueId() * 2) * 2) * 0.003 );
    }
    
    glMatrixMode    ( GL_MODELVIEW );
	glPushMatrix    ();
    Matrix34 mat    ( m_front, g_upVector, m_pos );
    glMultMatrixf   ( mat.ConvertToOpenGLFormat());
    glScalef        ( actualHeight, actualHeight, actualHeight );

    if( Location::ChristmasModEnabled() == 1 )
    {
        m_branchColourArray[0] = 180;
        m_branchColourArray[1] = 100;
        m_branchColourArray[2] = 50;
    }

    glColor4ubv     ( m_branchColourArray );
    glCallList      ( m_branchDisplayListId );

    if( Location::ChristmasModEnabled() != 1 && !m_evil )
    {
        glColor4ubv     ( m_leafColourArray );
        glCallList      ( m_leafDisplayListId );
    }

    glPopMatrix     ();

    glDepthMask     ( true );
    glEnable        ( GL_CULL_FACE );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glDisable       ( GL_TEXTURE_2D );
    glDisable       ( GL_BLEND );
}

void Tree::RenderHitCheck ()
{
#ifdef DEBUG_RENDER_ENABLED
    float actualHeight = GetActualHeight( 0.0f );

    RenderSphere( m_pos, 10.0f );
    RenderSphere( m_pos+m_hitcheckCentre*actualHeight, m_hitcheckRadius*actualHeight);
#endif
}

void Tree::Damage( float _damage )
{
    Building::Damage( _damage );

    if( m_fireDamage < 50.0f )
    {
        m_fireDamage += _damage * -1.0f;

        if( m_fireDamage > 50.0f )
        {
            if( m_onFire == 0.0f )
            {
                m_onFire = 0.01f;
            }
            else if( m_onFire < 0.0f )
            {
                m_onFire = m_onFire * -1.0f;
            }

            if( !m_burnSoundPlaying )
            {
                g_app->m_soundSystem->StopAllSounds( m_id, "Tree Create" );
                g_app->m_soundSystem->TriggerBuildingEvent( this, "Burn" );
                m_burnSoundPlaying = true;
            }
        }
    }
}

bool Tree::DoesSphereHit(Vector3 const &_pos, float _radius)
{
    if( SphereSphereIntersection( m_pos, 10.0f,
                                  _pos, _radius ) )
    {
        return true;
    }

    float actualHeight = GetActualHeight(0.0f);

    if( SphereSphereIntersection( m_pos+m_hitcheckCentre*actualHeight, m_hitcheckRadius*actualHeight,
                                  _pos, _radius ) )
    {
        return true;
    }

    return false;
}

bool Tree::DoesShapeHit(Shape *_shape, Matrix34 _transform)
{
    SpherePackage packageA( m_pos, 10.0f );
    if( _shape->SphereHit( &packageA, _transform ) )
    {
        return true;
    }

    float actualHeight = GetActualHeight(0.0f);

    SpherePackage packageB( m_pos+m_hitcheckCentre*actualHeight, m_hitcheckRadius*actualHeight);
    if( _shape->SphereHit( &packageB, _transform ) )
    {
        return true;
    }

    return false;
}

bool Tree::DoesRayHit(Vector3 const &_rayStart, Vector3 const &_rayDir,
                      float _rayLen, Vector3 *_pos, Vector3 *_norm)
{
    if( RaySphereIntersection( _rayStart, _rayDir, m_pos, 10.00f, _rayLen, _pos, _norm ) )
    {
        return true;
    }

    float actualHeight = GetActualHeight(0.0f);

    if( RaySphereIntersection( _rayStart, _rayDir,
                               m_pos+m_hitcheckCentre*actualHeight, m_hitcheckRadius*actualHeight,
                               _rayLen, _pos, _norm ) )
    {
        return true;
    }

    return false;
}


void Tree::RenderBranch( Vector3 _from, Vector3 _to, int _iterations,
                         bool _calcRadius, bool _renderBranch, bool _renderLeaf )
{
    if( _iterations == 0 ) return;
    _iterations--;

    if( _iterations == 0 )
    {
        if( _calcRadius )
        {
            float distToCentre = (_to - m_hitcheckCentre).Mag();
            if( distToCentre > m_hitcheckRadius ) m_hitcheckRadius = distToCentre;
        }
        else if( _renderLeaf )
        {
            m_hitcheckCentre += _to;
            m_numLeafs++;
        }
    }

    /*
    Make the tree seem more alive animation
    _to += Vector3(sinf(g_gameTime*(7-_iterations))*0.005f * _iterations,
                   sinf(g_gameTime) * 0.01f,
                   sinf(g_gameTime * (7-_iterations))*0.005f * _iterations );
    */

    Vector3 rightAngleA = ((_to - _from) ^ _to).Normalise();
    Vector3 rightAngleB = (rightAngleA ^ (_to - _from)).Normalise();

    Vector3 thisBranch = (_to - _from);

    float thickness = thisBranch.Mag();

    float budsize = 0.1f;

    if( _iterations == 0 )
    {
        budsize *= m_budsize;
    }

    Vector3 camRightA = rightAngleA * thickness * budsize;
    Vector3 camRightB = rightAngleB * thickness * budsize;


    if( (_iterations == 0 && _renderLeaf) ||
        (_iterations != 0 && _renderBranch) )
    {
        glTexCoord2f( 0.0f, 0.0f );             glVertex3fv( (_from - camRightA).GetData() );
        glTexCoord2f( 0.0f, 1.0f );             glVertex3fv( (_from + camRightA).GetData() );
        glTexCoord2f( 1.0f, 1.0f );             glVertex3fv( (_to + camRightA).GetData() );
        glTexCoord2f( 1.0f, 0.0f );             glVertex3fv( (_to - camRightA).GetData() );

        glTexCoord2f( 0.0f, 0.0f );             glVertex3fv( (_from - camRightB).GetData() );
        glTexCoord2f( 0.0f, 1.0f );             glVertex3fv( (_from + camRightB).GetData() );
        glTexCoord2f( 1.0f, 1.0f );             glVertex3fv( (_to + camRightB).GetData() );
        glTexCoord2f( 1.0f, 0.0f );             glVertex3fv( (_to - camRightB).GetData() );
    }

    int numBranches = 4;

    for( int i = 0; i < numBranches; ++i )
    {
        Vector3 thisRightAngle;
        if( i == 0 ) thisRightAngle = rightAngleA;
        if( i == 1 ) thisRightAngle = -rightAngleA;
        if( i == 2 ) thisRightAngle = rightAngleB;
        if( i == 3 ) thisRightAngle = -rightAngleB;

        float distance = 0.3f + frand(0.6f);
        Vector3 thisFrom = _from + thisBranch * distance;
        Vector3 thisTo = thisFrom + thisRightAngle * thickness * 0.4f * m_pushOut
                                  + thisBranch * (1.0f-distance) * m_pushUp;
        RenderBranch( thisFrom, thisTo, _iterations, _calcRadius, _renderBranch, _renderLeaf );
    }

}


void Tree::ListSoundEvents( LList<char *> *_list )
{
    Building::ListSoundEvents( _list );

    _list->PutData( "Burn" );
}


void Tree::Read( TextReader *_in, bool _dynamic )
{
    Building::Read( _in, _dynamic );

    m_height        = atof( _in->GetNextToken() );
    m_budsize       = atof( _in->GetNextToken() );
    m_pushOut       = atof( _in->GetNextToken() );
    m_pushUp        = atof( _in->GetNextToken() );
    m_iterations    = atoi( _in->GetNextToken() );
    m_seed          = atoi( _in->GetNextToken() );
    m_branchColour  = atoi( _in->GetNextToken() );
    m_leafColour    = atoi( _in->GetNextToken() );
	if( _in->TokenAvailable() )
	{
		m_leafDropRate = atoi( _in->GetNextToken() );
	}
	if( _in->TokenAvailable() )
	{
		m_spiritDropRate = atoi( _in->GetNextToken() );
	}
	if( _in->TokenAvailable() )
	{
		m_evil = atoi( _in->GetNextToken() );
	}
}

void Tree::Write( FileWriter *_out )
{
    Building::Write( _out );

    _out->printf( "%-8.2f", m_height);
    _out->printf( "%-8.2f", m_budsize);
    _out->printf( "%-8.2f", m_pushOut);
    _out->printf( "%-8.2f", m_pushUp);
    _out->printf( "%-8d", m_iterations);
    _out->printf( "%-8d", m_seed);
    _out->printf( "%-12d", m_branchColour);
    _out->printf( "%-12d", m_leafColour);
	_out->printf( "%-8d", m_leafDropRate);
	_out->printf( "%-8d", m_spiritDropRate);
	_out->printf( "%-8d", m_evil);
}

