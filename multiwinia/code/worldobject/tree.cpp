#include "lib/universal_include.h"

#include "lib/debug_render.h"
#include "lib/filesys/text_file_writer.h"
#include "lib/math_utils.h"
#include "lib/profiler.h"
#include "lib/resource.h"
#include "lib/filesys/text_stream_readers.h"
#include "lib/debug_utils.h"
#include "lib/shape.h"
#include "lib/hi_res_time.h"
#include "lib/text_renderer.h"
#include "lib/preferences.h"
#include "lib/math/random_number.h"

#include "worldobject/tree.h"
#include "worldobject/darwinian.h"
#include "worldobject/souldestroyer.h"
#include "worldobject/anthill.h"

#include "sound/soundsystem.h"

#include "app.h"
#include "globals.h"
#include "camera.h"
#include "particle_system.h"
#include "location.h"
#include "obstruction_grid.h"
#include "global_world.h"
#include "main.h"
#include "entity_grid.h"

#ifdef USE_DIRECT3D
#include "lib/opengl_directx_internals.h"
#endif

char *HashDouble( double value, char *buffer );

#define USE_DISPLAY_LISTS


Tree::Tree()
:   Building(),
    m_branchDisplayListId(-1),
    m_leafDisplayListId(-1),
    m_height(50.0),
    m_iterations(6),
    m_budsize(1.0),
    m_pushOut(1.0),
    m_pushUp(1.0),
    m_seed(0),
    m_leafColour(0xffffffff),
    m_branchColour(0xffffffff),
    m_onFire(0.0),
    m_fireDamage(0.0),
    m_burnSoundPlaying(false),
	m_leafDropRate(0),
    m_spawnsRemaining(0),
    m_spawnTimer(-1.0),
    m_destroyable(false),
    m_spiritDropRate(0),
    m_evil(false),
    m_corruptCheckTimer(5.0f),
    m_renderCorruptShadow(false),
    m_corrupted(false)
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

    AppDebugAssert( _template->m_type == TypeTree );
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
    m_spawnsRemaining = tree->m_spawnsRemaining;
    m_destroyable = tree->m_destroyable;
    m_spiritDropRate = tree->m_spiritDropRate;
    m_evil = tree->m_evil;  


    bool magic = ( m_spiritDropRate > 0 );

    if      ( m_evil )  g_app->m_soundSystem->TriggerBuildingEvent( this, "CreateEvil" );
    else if ( magic )   g_app->m_soundSystem->TriggerBuildingEvent( this, "CreateMagic" );
    else                g_app->m_soundSystem->TriggerBuildingEvent( this, "CreateNormal" );
}


void Tree::Clone( Tree *tree )
{
    m_height = tree->m_height;
    m_iterations = tree->m_iterations;
    m_budsize = tree->m_budsize;
    m_pushOut = tree->m_pushOut;
    m_pushUp = tree->m_pushUp;
    m_seed = tree->m_seed;
    m_branchColour = tree->m_branchColour;
    m_leafColour = tree->m_leafColour;  
    m_leafDropRate = tree->m_leafDropRate;
    m_spawnsRemaining = tree->m_spawnsRemaining;
    m_destroyable = tree->m_destroyable;
    m_spiritDropRate = tree->m_spiritDropRate;
    m_evil = tree->m_evil;  
}


void Tree::SetDetail( int _detail )
{
    AppAssert( _detail == 1 );
    // Detail must be 1, otherwise we get different centre/radius values
    // which causes a sync error

    Building::SetDetail( _detail );

    int oldIterations = m_iterations;
    if( _detail == 0 ) m_iterations = 0;
    else m_iterations -= ( _detail - 1 );
    m_iterations = max( m_iterations, 3 );

    Generate();

    m_iterations = oldIterations;
    m_centrePos = m_pos + m_hitcheckCentre * m_height;
    m_radius = m_hitcheckRadius * m_height * 1.5;
}


bool Tree::Advance()
{
    m_fireDamage -= SERVER_ADVANCE_PERIOD;
    if( m_fireDamage < 0.0 ) m_fireDamage = 0.0;
    if( m_fireDamage > 100.0 ) m_fireDamage = 100.0;

    if( m_onFire > 0.0 ) 
    {
        //
        // Spawn fire particle

        double actualHeight = GetActualHeight(0.0);
        int numFire = actualHeight / 12;
        int buildingDetail = g_prefsManager->GetInt( "RenderBuildingDetail", 1 );
        numFire /= buildingDetail;

        for( int i = 0; i < numFire; ++i )
        {
            double distance = frand( actualHeight * 0.6);
            double height = 0.7 * actualHeight + frand(actualHeight) * 0.3;
            Vector3 fireSpawn = Vector3(distance,height,0);
            fireSpawn.RotateAroundY( frand(2.0 * M_PI) );
            fireSpawn += m_pos;
            double fireSize = actualHeight*2.0;
            fireSize *= (1.0 + sfrand(0.5) );
            g_app->m_particleSystem->CreateParticle( fireSpawn, g_zeroVector, Particle::TypeFire, fireSize );
        }
        
        
        //
        // Burn down

        m_onFire += SERVER_ADVANCE_PERIOD * 0.02;
        if( m_onFire > 1.0 )
        {
            m_onFire = -1.0;
            if( m_corrupted && m_evil )
            {
                m_evil = false;
                m_corrupted = false;
            }
            if( m_destroyable )
            {
                if( m_burnSoundPlaying )
                {
                    g_app->m_soundSystem->StopAllSounds( m_id, "Tree Burn" );
                    m_burnSoundPlaying = false;
                }

                return true;
            }
        }

        //
        // Spread to nearby trees
        // Also burn other burnable buildings down

        Vector3 hitCentre = m_pos + m_hitcheckCentre*actualHeight;
        double hitRadius = m_hitcheckRadius * actualHeight;

        for( int b = 0; b < g_app->m_location->m_buildings.Size(); ++b )
        {            
            if( g_app->m_location->m_buildings.ValidIndex(b) )
            {
                Building *building = g_app->m_location->m_buildings[b];
                if( building != this &&
                    building->m_type == TypeTree )
                {
                    Tree *tree = (Tree *) building;
                    double distance = (tree->m_pos - m_pos).Mag();
                    double theirActualHeight = tree->GetActualHeight(0.0);
                    double theirRadius = theirActualHeight * tree->m_hitcheckRadius * 1.5;
                    double ourRadius = actualHeight * m_hitcheckRadius * 1.5;

                    if( theirRadius + ourRadius >= distance )
                    {
                        tree->Damage(-1.0);
                    }
                }

                if( building->m_type == TypeAntHill )
                {
                    double distance = (building->m_pos - m_pos).Mag();
                    double ourRadius = actualHeight * m_hitcheckRadius * 1.5;
                    double theirRadius = 100;

                    if( theirRadius + ourRadius >= distance )
                    {
                        ((AntHill *)building)->Burn();
                    }
                }
            }
        }

		int num = 0;
		g_app->m_location->m_entityGrid->GetNeighbours(s_neighbours, m_pos.x, m_pos.z, GetActualHeight(0.0) / 2.0, &num );
		for( int i = 0; i < num; ++i )
		{
			Entity *e = g_app->m_location->GetEntity( s_neighbours[i] );
			if( e )
            {
                if( e->m_type == Entity::TypeDarwinian )
			    {
				    ((Darwinian *) e)->SetFire();
			    }
                else
                {
                    e->ChangeHealth(-1, Entity::DamageTypeFire);
                }
            }
		}
        
    }
    else if( m_onFire < 0.0 )
    {
        if( m_burnSoundPlaying )
        {
            g_app->m_soundSystem->StopAllSounds( m_id, "Tree Burn" );
            m_burnSoundPlaying = false;
        }

        //
        // Regrow

        double regrowAmount = 0.01;
        if( g_app->Multiplayer() ) regrowAmount = 0.05;

        m_onFire += SERVER_ADVANCE_PERIOD * regrowAmount;
        if( m_onFire > 0.0 )
        {
            m_onFire = 0.0;
        }
    }

	if( m_onFire > -0.3 && m_onFire <= 0.0 )
	{
        if( m_leafDropRate > 0 )
        {
		    // drop some leaves
		    if( rand() % (51-m_leafDropRate) == 0 )
		    {
			    double actualHeight = GetActualHeight(0.0);
			    Vector3 fireSpawn = m_pos + Vector3(0,actualHeight,0);
			    fireSpawn += Vector3( sfrand(actualHeight*1.0), 
								      sfrand(actualHeight*0.25),
								      sfrand(actualHeight*1.0) );
			    g_app->m_particleSystem->CreateParticle( fireSpawn, g_zeroVector, Particle::TypeLeaf, -1.0, RGBAColour (m_leafColourArray[0], m_leafColourArray[1], m_leafColourArray[2] ) );
		    }
        }

        if( !m_evil && m_spiritDropRate > 0 )
        {
            // drop some spirits
            if( syncrand() % (51-m_spiritDropRate) == 0 )
            {
                double actualHeight = GetActualHeight(0.0);
			    Vector3 pos = Vector3(FRAND(actualHeight),actualHeight,0);
                pos.RotateAroundY( syncfrand(2.0 * M_PI) );
                pos += m_pos;

                int teamId = -1;
                while( teamId == -1 || teamId == g_app->m_location->GetMonsterTeamId() ||
                    teamId == g_app->m_location->GetFuturewinianTeamId() )
                {
                    teamId = syncfrand( NUM_TEAMS ); 
                }

                g_app->m_location->SpawnSpirit( pos, g_zeroVector, teamId, WorldObjectId() );
            }
        }

        //
        // Potentially create another tree

        if( m_spawnsRemaining )
        {
            double timeNow = GetNetworkTime();

            if( m_spawnTimer < 0.0 )
            {
                m_spawnTimer = timeNow + 5 + syncfrand(5);                
            }
            else if( m_spawnTimer > 0.0 )
            {
                if( timeNow >= m_spawnTimer )
                {
                    --m_spawnsRemaining;
                    m_spawnTimer = -1.0;
                    CreateAnotherTree();
                }
            }
        }
	}

    if( m_corruptCheckTimer < 0.0f && !m_evil )
    {
        m_corruptCheckTimer += SERVER_ADVANCE_PERIOD;
        if( m_corruptCheckTimer > 0.0f )
        {
            m_corruptCheckTimer = 5.0f;
            m_evil = true;
        }
    }
    
    if( m_evil )
    {
        double range = GetActualHeight(0.0f) / 2.0;
        if( syncfrand(10.0) < 0.2 )
        {
            int myTeam = g_app->m_location->GetMonsterTeamId();
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
                        g_app->m_location->m_spirits.RemoveData( spiritIndex );
                    
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
                }                            
            }
        }

        if( GetActualHeight(0.0f) == m_height )
        {
            m_corruptCheckTimer -= SERVER_ADVANCE_PERIOD;
            if( m_corruptCheckTimer <= 0.0f )
            {
                m_corruptCheckTimer = 10.0f + syncfrand(5.0f);
                // we've grown to our full height, so check to see if i can find some normal trees nearby, and corrupt them
                for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
                {
                    if( g_app->m_location->m_buildings.ValidIndex(i) )
                    {
                        Tree *t = (Tree *)g_app->m_location->m_buildings[i];
                        if( t && t->m_type == Building::TypeTree )
                        {
                            if( !t->m_evil && !t->m_corrupted &&
                                (m_pos - t->m_pos).Mag() < GetActualHeight(0.0f) )
                            {
                                //t->m_evil = true;
                                t->m_spiritDropRate = 0;
                                t->m_corruptCheckTimer = -10.0f;
                                t->m_corrupted = true;
                                break;
                            }
                        }
                    }
                }
            }
        }
    }

    return false;
}


Vector3 PushFromObstructions( Vector3 _pos )
{
    Vector3 result = _pos;    
    result.y = g_app->m_location->m_landscape.m_heightMap->GetValue( result.x, result.z );

    Matrix34 transform( Vector3(1,0,0), g_upVector, result );    

    //
    // Push from buildings

    LList<int> *buildings = g_app->m_location->m_obstructionGrid->GetBuildings( result.x, result.z );

    for( int b = 0; b < buildings->Size(); ++b )
    {
        int buildingId = buildings->GetData(b);
        Building *building = g_app->m_location->GetBuilding( buildingId );
        if( building )
        {        
            bool hit = false;
            if( building->DoesSphereHit( result, 1.0 ) ) hit = true;

            if( hit )
            {                      
                Vector3 pushForce = (building->m_pos - result).SetLength(2.0);
                while( building->DoesSphereHit( result, 1.0 ) )
                {
                    result -= pushForce;                
                    //result.y = g_app->m_location->m_landscape.m_heightMap->GetValue( result.x, result.z );
                }
            }
        }
    }

    return result;
}


void Tree::CreateAnotherTree()
{
    //
    // Find all trees nearby and push from them

    Vector3 nearbyTreesPos;
    int numNearbyTrees = 0;

    for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
    {
        if( g_app->m_location->m_buildings.ValidIndex(i) )
        {
            Building *building = g_app->m_location->m_buildings.GetData(i);
            if( building->m_id != m_id &&
                building->m_type == Building::TypeTree )
            {
                double range = ( building->m_pos - m_pos ).Mag();

                if( range < 100 )
                {
                    nearbyTreesPos += building->m_pos;
                    numNearbyTrees ++;
                }
            }
        }
    }

    double fullHeight = GetActualHeight(0.0);
    double range = fullHeight + syncfrand( fullHeight );

    Vector3 pos = m_pos;

    if( numNearbyTrees > 0 )
    {
        nearbyTreesPos /= numNearbyTrees;
        Vector3 pushVector = (m_pos - nearbyTreesPos);
        pushVector.SetLength( fullHeight );
        pushVector.RotateAroundY( syncsfrand( 0.25 * M_PI ) );
        pos += pushVector;
    }
    else
    {
        Vector3 posDiff(range,0,0);
        posDiff.RotateAroundY( syncfrand(2.0 * M_PI) );
        pos += posDiff;
    }

    pos = PushFromObstructions( pos );
    pos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( pos.x, pos.z );

    if( pos.y < 0 ) return;
   
    Building *newBuilding = Building::CreateBuilding( Building::TypeTree );
    int id = g_app->m_location->m_buildings.PutData(newBuilding);

    Tree treeTemplate;
    treeTemplate.Clone( this );
    treeTemplate.m_id.Set( 255, UNIT_BUILDINGS, id, g_app->m_globalWorld->GenerateBuildingId() );
    treeTemplate.m_pos = pos;

    Tree *newTree = (Tree *) newBuilding;    

    newBuilding->Initialise( &treeTemplate );
    
    newTree->m_seed = (int) syncfrand(99999);    
	// Order of evaluation is important to avoid different rounding errors
	volatile double rand0 = syncsfrand(0.2);
	volatile double temp0	  = newTree->m_height * 0.75;
	volatile double temp1	  = 1.0 + rand0;
	volatile double newHeight1 = temp0 * temp1;
	newTree->m_height = newHeight1;

    newTree->SetFireAmount(-1.0); // this sets the tree to its min height, without actually being on fire and burning nearby darwinians
    newBuilding->SetDetail( 1 );    

    g_app->m_location->m_obstructionGrid->CalculateAll();
}


double Tree::GetActualHeight( double _predictionTime )
{
    double predictedOnFire = m_onFire;
    if( predictedOnFire < 0.0 )
    {
        double regrowAmount = 0.01;
        if( g_app->Multiplayer() ) regrowAmount = 0.05;

        predictedOnFire += _predictionTime * regrowAmount;
    }

    double actualHeight = m_height * (1.0-iv_abs(predictedOnFire));
    if( actualHeight < m_height * 0.01 ) actualHeight = m_height * 0.01;

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
		g_app->m_resource->DeleteDisplayListAsync( m_branchDisplayListId );
        m_branchDisplayListId = -1;
    }

    if( m_leafDisplayListId != -1 )
    {
        g_app->m_resource->DeleteDisplayListAsync( m_leafDisplayListId );
        m_leafDisplayListId = -1;
    }
}

void Tree::GenerateBranches()
{
    AppSeedRandom( m_seed );
    glBegin         ( GL_QUADS );	
#ifdef USE_DIRECT3D
	// The colour is not supposed to be specified here so that it can be changed
	// easily in the editor (when experimenting).
	// Really, the Trees should be rewritten for Direct3D to use Meshes
    glColor4ubv     ( m_branchColourArray ); // Direct3D hack
#endif
    RenderBranch    ( g_zeroVector, g_upVector, m_iterations, false, true, false );
    glEnd           ();
}

void Tree::GenerateLeaves()
{
    AppSeedRandom( m_seed );
#ifdef USE_DIRECT3D
    glColor4ubv     ( m_leafColourArray );	// Direct3D hack
#endif
    glBegin         ( GL_QUADS );
    RenderBranch    ( g_zeroVector, g_upVector, m_iterations, false, false, true );
    glEnd           ();
}
	
void Tree::Generate()
{
    double timeNow = GetHighResTime();

    m_hitcheckCentre.Zero();
    m_hitcheckRadius = 0.0;
    m_numLeafs = 0;

	DeleteDisplayLists();
	
	intToArray(m_branchColour, m_branchColourArray);
	intToArray(m_leafColour, m_leafColourArray);	

    int treeDetail = g_prefsManager->GetInt( "RenderTreeDetail", 1 );
    if( treeDetail > 1 )
    {
        int alpha = m_leafColourArray[3];
        alpha *= iv_pow( 1.3, treeDetail );
        alpha = min( alpha, 255 );
        m_leafColourArray[3] = alpha;
    }

#ifdef USE_DISPLAY_LISTS
	m_branchDisplayListId = 
		g_app->m_resource->CreateDisplayListAsync( 
			NULL,
			Method<Tree>( &Tree::GenerateBranches, this ) );

	m_leafDisplayListId = 
		g_app->m_resource->CreateDisplayListAsync( 
			NULL,
			Method<Tree>( &Tree::GenerateLeaves, this ) );
#endif
    
    //
    // We now have all the leaf positions accumulated in m_hitcheckCentre
    // So we can calculate the actual centre position, then the radius

    m_hitcheckCentre /= (double) m_numLeafs;
    AppSeedRandom( m_seed );
    RenderBranch( g_zeroVector, g_upVector, m_iterations, true, false, false );
    m_hitcheckRadius *= 0.8;

    double totalTime = GetHighResTime() - timeNow;
    AppDebugOut( "Tree generated in %dms\n", int(totalTime * 1000.0) );
}

void Tree::Render( double _predictionTime )
{
    //RenderHitCheck();
}


bool Tree::PerformDepthSort( Vector3 &_centrePos )
{
    _centrePos = m_pos+m_hitcheckCentre*m_height;
    return true;
}


void Tree::RenderAlphas( double _predictionTime )
{            
    if( m_branchDisplayListId == -1 ||
        m_leafDisplayListId == -1 )
    {
        Generate();
    }
    
    //if( g_app->m_editing )
    {
	    intToArray(m_branchColour, m_branchColourArray);
	    intToArray(m_leafColour, m_leafColourArray);	
    }
        
    double actualHeight = GetActualHeight( _predictionTime );
        
    glEnable        ( GL_TEXTURE_2D );   
    glEnable        ( GL_BLEND );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTextureWithAlpha( "textures/tree.bmp" ) );
	glTexParameteri	( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri	( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );    
    glDisable       ( GL_CULL_FACE );
    glDepthMask     ( false );

    if( g_app->m_location->ChristmasModEnabled() )
    {
        m_branchColourArray[0] = 180;
        m_branchColourArray[1] = 100;
        m_branchColourArray[2] = 50;
    }

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

        actualHeight *= ( 1.0 + iv_sin((GetHighResTime() + m_id.GetUniqueId() * 2) * 2) * 0.01 );
    }
    else
    {
        actualHeight *= ( 1.0 + iv_sin((GetHighResTime() + m_id.GetUniqueId() * 2) * 2) * 0.003 );
    }
    
    if( m_spiritDropRate > 0 )
    {
        double value = m_leafColourArray[2];
        double factor = iv_abs(iv_sin(GetHighResTime() + m_id.GetUniqueId() * 10));
        value = value * factor + 255 * (1-factor);
        if( value < 0 ) value = 0;
        if( value > 255 ) value = 255;

        m_leafColourArray[2] = value;        
    }

    bool renderAgain = false;
    if( m_corruptCheckTimer <= 0.0f && !m_evil )
    {
        float factor = (10.0f + (m_corruptCheckTimer+_predictionTime)) / 10.0f;
        if( m_renderCorruptShadow )
        {
            m_renderCorruptShadow = false;
            glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

            m_leafColourArray[0] = 0;
            m_leafColourArray[1] = 0;
            m_leafColourArray[2] = 0;
            m_leafColourArray[3] = 0;

            m_branchColourArray[0] = 0;
            m_branchColourArray[1] = 0;
            m_branchColourArray[2] = 0;
            m_branchColourArray[3] = 200.0f * factor;
        }
        else
        {
            m_branchColourArray[0] -= (float)m_branchColourArray[0] * ( factor );
            m_branchColourArray[1] -= (float)m_branchColourArray[1] * ( factor );
            m_branchColourArray[2] -= (float)m_branchColourArray[2] * ( factor );
            m_branchColourArray[3] *= (1.0f - factor);
            renderAgain = true;
            m_renderCorruptShadow = true;

            m_leafColourArray[3] *= ( 1.0f - factor );
        }
    }

    if( m_onFire > 0.0f )
    {        
        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        
        double fireFactor = m_onFire * 10.0;
        fireFactor = min( fireFactor, 1.0 );
        m_branchColourArray[0] -= (float)m_branchColourArray[0] * ( fireFactor );
        m_branchColourArray[1] -= (float)m_branchColourArray[1] * ( fireFactor );
        m_branchColourArray[2] -= (float)m_branchColourArray[2] * ( fireFactor );
        m_branchColourArray[3] = 255;

        m_leafColourArray[0] = 0;
        m_leafColourArray[1] = 0;
        m_leafColourArray[2] = 0;
        m_leafColourArray[3] *= ( 1.0 - m_onFire );
    }
    else if( m_onFire < 0.0 )
    {
        if( iv_abs(m_onFire) > 0.5 )
        {
            m_leafColourArray[3] = 0;
        }
        else
        {
            m_leafColourArray[3] *= ( 1.0 - iv_abs(m_onFire) * 2.0 );
        }
    }

    glMatrixMode    ( GL_MODELVIEW );
    glPushMatrix    ();
    Matrix34 mat    ( m_front, g_upVector, m_pos );    
    glMultMatrixd   ( mat.ConvertToOpenGLFormat());
    glScalef        ( actualHeight, actualHeight, actualHeight );

    glColor4ubv     ( m_branchColourArray );

    
#ifdef USE_DISPLAY_LISTS
    glCallList      ( m_branchDisplayListId );

    if( !g_app->m_location->ChristmasModEnabled() && !m_evil )
    {
        glColor4ubv     ( m_leafColourArray );
        glCallList      ( m_leafDisplayListId );
    }
#endif

    glBlendFunc( GL_SRC_ALPHA, GL_ONE );

    glPopMatrix     ();                
    

    if( m_onFire > 0.0 )
    {
        glBindTexture( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/starburst.bmp" ) );
        glDisable( GL_DEPTH_TEST );

        double alpha = 0.5 * actualHeight / m_height;
        
        glColor4f( 0.8, 0.2, 0.1, alpha);        

        Vector3 camUp = g_app->m_camera->GetUp();
        Vector3 camRight = g_app->m_camera->GetRight();
        double timeIndex = g_gameTime * 2;

        for( int i = 0; i < 4; ++i )
        {
            Vector3 pos = m_pos;
            pos.y += actualHeight * 0.8;
            pos.x += iv_sin(timeIndex+i) * i * 1.7;
            pos.y += iv_cos(timeIndex+i);
            pos.z += iv_cos(timeIndex+i) * i * 1.7;

            double size = actualHeight * 2;
            size = max( size, 5.0 );

            glBegin( GL_QUADS );
                glTexCoord2i(0,0);      glVertex3dv( (pos - camUp * size - camRight * size).GetData() );
                glTexCoord2i(1,0);      glVertex3dv( (pos + camUp * size - camRight * size).GetData() );
                glTexCoord2i(1,1);      glVertex3dv( (pos + camUp * size + camRight * size).GetData() );
                glTexCoord2i(0,1);      glVertex3dv( (pos - camUp * size + camRight * size).GetData() );
            glEnd();        
        }

        glEnable( GL_DEPTH_TEST );
    }
 

    glDepthMask     ( true );
    glEnable        ( GL_CULL_FACE );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );    
    glDisable       ( GL_TEXTURE_2D );
    glDisable       ( GL_BLEND );

    if( renderAgain )
    {
        RenderAlphas(_predictionTime);
    }
}

void Tree::RenderHitCheck ()
{
#ifdef DEBUG_RENDER_ENABLED
    double actualHeight = GetActualHeight( 0.0 );

    RenderSphere( m_pos, 10.0 );
    RenderSphere( m_pos+m_hitcheckCentre*actualHeight, m_hitcheckRadius*actualHeight);
#endif
}

void Tree::Damage( double _damage )
{
    Building::Damage( _damage );

    if( m_fireDamage < 50.0 )
    {
        m_fireDamage += _damage * -1.0;

        if( m_fireDamage > 50.0 )
        {
            if( m_onFire == 0.0 )
            {
                m_onFire = 0.01;
            }
            else if( m_onFire < 0.0 )
            {
                m_onFire = m_onFire * -1.0;
            }

            if( !m_burnSoundPlaying )
            {
                g_app->m_soundSystem->TriggerBuildingEvent( this, "Burn" );
                m_burnSoundPlaying = true;
            }
        }
    }
}

bool Tree::DoesSphereHit(Vector3 const &_pos, double _radius)
{
    if( SphereSphereIntersection( m_pos, 10.0, 
                                  _pos, _radius ) )
    {
        return true;
    }

    double actualHeight = GetActualHeight(0.0);

    if( SphereSphereIntersection( m_pos+m_hitcheckCentre*actualHeight, m_hitcheckRadius*actualHeight,
                                  _pos, _radius ) )
    {
        return true;
    }

    return false;
}

bool Tree::DoesShapeHit(Shape *_shape, Matrix34 _transform)
{
    SpherePackage packageA( m_pos, 10.0 );
    if( _shape->SphereHit( &packageA, _transform ) )
    {
        return true;
    }

    double actualHeight = GetActualHeight(0.0);

    SpherePackage packageB( m_pos+m_hitcheckCentre*actualHeight, m_hitcheckRadius*actualHeight);
    if( _shape->SphereHit( &packageB, _transform ) )
    {
        return true;
    }

    return false;
}

bool Tree::DoesRayHit(Vector3 const &_rayStart, Vector3 const &_rayDir, 
                      double _rayLen, Vector3 *_pos, Vector3 *_norm)
{
    if( RaySphereIntersection( _rayStart, _rayDir, m_pos, 10.00, _rayLen, _pos, _norm ) )
    {
        return true;
    }

    double actualHeight = GetActualHeight(0.0);

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
            double distToCentre = (_to - m_hitcheckCentre).Mag();
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
    _to += Vector3(iv_sin(g_gameTime*(7-_iterations))*0.005 * _iterations,
                   iv_sin(g_gameTime) * 0.01,
                   iv_sin(g_gameTime * (7-_iterations))*0.005 * _iterations );
    */

    Vector3 rightAngleA = ((_to - _from) ^ _to).Normalise();
    Vector3 rightAngleB = (rightAngleA ^ (_to - _from)).Normalise();    

    Vector3 thisBranch = (_to - _from);       

    double thickness = thisBranch.Mag();
        
    double budsize = 0.1;
    
    if( _iterations == 0 )
    {
        budsize *= m_budsize; 
    }
    
    Vector3 camRightA = rightAngleA * thickness * budsize;
    Vector3 camRightB = rightAngleB * thickness * budsize;
    

    if( (_iterations == 0 && _renderLeaf) ||
        (_iterations != 0 && _renderBranch) )
    {
        glTexCoord2f( 0.0, 0.0 );             glVertex3dv( (_from - camRightA).GetData() );
        glTexCoord2f( 0.0, 1.0 );             glVertex3dv( (_from + camRightA).GetData() );
        glTexCoord2f( 1.0, 1.0 );             glVertex3dv( (_to + camRightA).GetData() );
        glTexCoord2f( 1.0, 0.0 );             glVertex3dv( (_to - camRightA).GetData() );

        glTexCoord2f( 0.0, 0.0 );             glVertex3dv( (_from - camRightB).GetData() );
        glTexCoord2f( 0.0, 1.0 );             glVertex3dv( (_from + camRightB).GetData() );
        glTexCoord2f( 1.0, 1.0 );             glVertex3dv( (_to + camRightB).GetData() );
        glTexCoord2f( 1.0, 0.0 );             glVertex3dv( (_to - camRightB).GetData() );        
    }
    
    int numBranches = 4;

    for( int i = 0; i < numBranches; ++i )
    {
        Vector3 thisRightAngle;
        if( i == 0 ) thisRightAngle = rightAngleA;
        if( i == 1 ) thisRightAngle = -rightAngleA;
        if( i == 2 ) thisRightAngle = rightAngleB;
        if( i == 3 ) thisRightAngle = -rightAngleB;

        double distance = 0.3 + frand(0.6);        
        Vector3 thisFrom = _from + thisBranch * distance;
        Vector3 thisTo = thisFrom + thisRightAngle * thickness * 0.4 * m_pushOut 
                                  + thisBranch * (1.0-distance) * m_pushUp;
        RenderBranch( thisFrom, thisTo, _iterations, _calcRadius, _renderBranch, _renderLeaf );
    }

}


void Tree::ListSoundEvents( LList<char *> *_list )
{
    Building::ListSoundEvents( _list );

    _list->PutData( "CreateNormal" );
    _list->PutData( "CreateEvil" );
    _list->PutData( "CreateMagic" );

    _list->PutData( "Burn" );
}


void Tree::Read( TextReader *_in, bool _dynamic )
{
    Building::Read( _in, _dynamic );
    
    m_height        = iv_atof( _in->GetNextToken() );
    m_budsize       = iv_atof( _in->GetNextToken() );
    m_pushOut       = iv_atof( _in->GetNextToken() );
    m_pushUp        = iv_atof( _in->GetNextToken() );
    m_iterations    = atoi( _in->GetNextToken() );
    m_seed          = atoi( _in->GetNextToken() );
    m_branchColour  = atoi( _in->GetNextToken() );
    m_leafColour    = atoi( _in->GetNextToken() );
	if( _in->TokenAvailable() )
	{
		m_leafDropRate = atoi( _in->GetNextToken() );
        if( _in->TokenAvailable() )
        {
            m_spiritDropRate = atoi( _in->GetNextToken() );
            if( _in->TokenAvailable() )
            {
                m_evil = atoi( _in->GetNextToken() ) == 1 ? true : false;
            }
        }
	}
}

void Tree::Write( TextWriter *_out )
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
    _out->printf( "%-8d", m_spiritDropRate );
    _out->printf( "%-8d", m_evil ? 1 : 0 );
}

void Tree::SetFireAmount( double _amount )
{
    m_onFire = _amount;
}

double Tree::GetFireAmount()
{
    return m_onFire;
}

bool Tree::IsOnFire()
{
    return (m_onFire > 0.0);
}

double Tree::GetBurnRange()
{
    return GetActualHeight(0.0) / 2.0;
}

bool Tree::IsInView()
{
    return( g_app->m_camera->SphereInViewFrustum( m_pos, m_height ) );
}


char *Tree::LogState( char *message )
{
    static char s_result[1024];

    static char buf1[32], buf2[32], buf3[32], buf4[32], buf5[32], buf6[32], buf7[32], buf7b[32], buf8[32], buf9[32];

    sprintf( s_result, "%sTREE Id[%d] pos[%s,%s,%s], vel[%s] front[%s] centre[%s] radius[%s] height[%s] hitcheckCentre[%s] hitcheckRadius[%s]",
                        (message)?message:"",
                        m_id.GetUniqueId(),
                        HashDouble( m_pos.x, buf1 ),
                        HashDouble( m_pos.y, buf2 ),
                        HashDouble( m_pos.z, buf3 ),
                        HashDouble( m_vel.x + m_vel.y + m_vel.z, buf4 ),
                        HashDouble( m_front.x + m_front.y + m_front.z, buf5 ),
                        HashDouble( m_centrePos.x + m_centrePos.y + m_centrePos.z, buf6 ), 
                        HashDouble( m_radius, buf7 ),
						HashDouble( m_height, buf7b ), // radius is a function of hitCheckRadius and height
                        HashDouble( m_hitcheckCentre.x + m_hitcheckCentre.y + m_hitcheckCentre.z, buf8 ),
                        HashDouble( m_hitcheckRadius, buf9 ) );

    return s_result;
}

