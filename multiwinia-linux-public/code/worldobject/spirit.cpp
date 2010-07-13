#include "lib/universal_include.h"

#include <math.h>

#include "lib/math_utils.h"
#include "lib/debug_utils.h"
#include "lib/debug_render.h"
#undef TRACK_SYNC_RAND
#include "lib/math/random_number.h"
#include "lib/profiler.h"

#include "app.h"
#include "camera.h"
#include "entity_grid.h"
#include "obstruction_grid.h"
#include "location.h"
#include "team.h"
#include "global_world.h"
#include "particle_system.h"
#include "main.h"

#include "sound/soundsystem.h"

#include "worldobject/spirit.h"
#include "worldobject/virii.h"
#include "worldobject/egg.h"
#include "worldobject/harvester.h"


static std::vector<WorldObjectId> s_neighbours;


Spirit::Spirit()
:   WorldObject(),
    m_teamId(255),
    m_positionOffset(0.0),
    m_state(StateUnknown),
    m_eggSearchTimer(0.0),
    m_numNearbyEggs(0),
    m_pushFromBuildings(true),
    m_broken(-1.0)
{   
}

Spirit::~Spirit()
{
}

void Spirit::Begin()
{    
    m_timeSync = 6.0;
    m_state = StateBirth;
    m_broken = -1.0;

    m_positionOffset = syncfrand(10.0);
    m_xaxisRate = syncfrand(2.0);
    m_yaxisRate = syncfrand(2.0);
    m_zaxisRate = syncfrand(2.0);

    m_numNearbyEggs = 0;
    m_eggSearchTimer = 0.0;

    m_groundHeight = g_app->m_location->m_landscape.m_heightMap->GetValue( m_pos.x, m_pos.z );

    //g_app->m_soundSystem->TriggerOtherEvent( this, "Create", SoundSourceBlueprint::TypeSpirit );
}

bool Spirit::Advance()
{   
    m_vel *= ( 1.0 - SERVER_ADVANCE_PERIOD );

    if( m_state != StateAttached &&
        m_state != StateInEgg &&
        m_state != StateShaman &&
        m_state != StateHarvested )
    {
        //
        // Make me double around slowly

        if( m_state != StateCollected )
        {
            m_timeSync -= SERVER_ADVANCE_PERIOD;
            m_positionOffset += SERVER_ADVANCE_PERIOD;
            m_xaxisRate += syncsfrand(1.0) * g_gameTimer.GetGameSpeed();
            m_yaxisRate += syncsfrand(1.0) * g_gameTimer.GetGameSpeed();
            m_zaxisRate += syncsfrand(1.0) * g_gameTimer.GetGameSpeed();
            if( m_xaxisRate > 2.0 ) m_xaxisRate = 2.0;
            if( m_xaxisRate < 0.0 ) m_xaxisRate = 0.0;
            if( m_yaxisRate > 2.0 ) m_yaxisRate = 2.0;
            if( m_yaxisRate < 0.0 ) m_yaxisRate = 0.0;
            if( m_zaxisRate > 2.0 ) m_zaxisRate = 2.0;
            if( m_zaxisRate < 0.0 ) m_zaxisRate = 0.0;
            m_hover.x = iv_sin( m_positionOffset ) * m_xaxisRate;
            m_hover.y = iv_sin( m_positionOffset ) * m_yaxisRate;
            m_hover.z = iv_sin( m_positionOffset ) * m_zaxisRate;            
        }
       
        switch( m_state )
        {
            case StateBirth:                 
            {
                m_hover.y = max( m_hover.y, 0.0 + syncfrand(0.5) );
                double heightAboveGround = m_pos.y - m_groundHeight;
                if( heightAboveGround > 10.0 )
                {
                    double fractionAboveGround = heightAboveGround / 100.0;                    
                    fractionAboveGround = min( fractionAboveGround, 1.0 );
                    m_hover.y = (-10.0 - syncfrand(10.0)) * fractionAboveGround;
                }
                else if( m_timeSync <= 0.0 )
                {
                    m_state = StateFloating;
                    m_timeSync = 120.0 + syncsfrand(60.0); 
                    if( g_app->Multiplayer() ) m_timeSync /= 3.0;
                }
                break;
            }

            case StateFloating:
                if( m_timeSync <= 0.0 )
                {
                    m_state = StateDeath;
                    //g_app->m_soundSystem->TriggerOtherEvent( this, "BeginAscent", SoundSourceBlueprint::TypeSpirit );
                    m_timeSync = 180.0;
                    if( g_app->Multiplayer() ) m_timeSync /= 2.0;
                    if( !g_app->Multiplayer() ) AddToGlobalWorld();
                }
                break;

            case StateInStore:
                break;
                
            case StateDeath:            
                m_hover.y = max(m_hover.y, 2.0 + syncfrand(2.0));
                if( m_timeSync <= 0.0 )
                {
                    // We are now dead
                    return true;
                }
                break;    

            case StateShaman:
                m_timeSync = 0.0;
                break;

            case StateCollected:
                Harvester *h = (Harvester *)g_app->m_location->GetEntity( m_harvesterId );
                if( h && h->m_type == Entity::TypeHarvester )
                {
                    Vector3 oldPos = m_pos;
                    m_vel = h->m_vel;// * SERVER_ADVANCE_PERIOD;
                    //m_vel = (oldPos - m_pos) / SERVER_ADVANCE_PERIOD;
                }
                else
                {
                    m_state = StateFloating;
                }
                break;
        };
    }

    Vector3 oldPos = m_pos;

    if( m_pushFromBuildings && 
        m_state != StateInStore && 
        m_state != StateDeath )
    {
        PushFromBuildings();
    }

    m_pos += m_vel * SERVER_ADVANCE_PERIOD;
    m_pos += m_hover * SERVER_ADVANCE_PERIOD;
    double worldSizeX = g_app->m_location->m_landscape.GetWorldSizeX();
    double worldSizeZ = g_app->m_location->m_landscape.GetWorldSizeZ();
    if( m_pos.x < 0.0 ) m_pos.x = 0.0;
    if( m_pos.z < 0.0 ) m_pos.z = 0.0;
    if( m_pos.x >= worldSizeX ) m_pos.x = worldSizeX;
    if( m_pos.z >= worldSizeZ ) m_pos.z = worldSizeZ;


    if( m_state != StateInEgg && 
        m_state != StateDeath &&
        m_state != StateAttached )
    {        
        if( m_pos.y < m_groundHeight + 2.0 ) m_pos.y = m_groundHeight + 2.0;
    }


    //m_vel = (m_pos - oldPos) / SERVER_ADVANCE_PERIOD;
    
    if( m_broken != -1.0 )
    {
        m_broken += SERVER_ADVANCE_PERIOD;
    }

    return false;
}


void Spirit::SearchForNearbyEggs()
{
    m_numNearbyEggs = 0;

    int numNeighbours;
    g_app->m_location->m_entityGrid->GetNeighbours( s_neighbours, 
                            m_pos.x, m_pos.z, VIRII_MAXSEARCHRANGE, &numNeighbours );

    for( int i = 0; i < numNeighbours; ++i )
    {
        WorldObjectId id = s_neighbours[i];
        Egg *egg = (Egg *) g_app->m_location->GetEntitySafe( id, Entity::TypeEgg );
        
        if( egg &&
            egg->m_state == Egg::StateDormant && 
            egg->m_onGround )
        {
            m_nearbyEggs[ m_numNearbyEggs ] = id;
            ++m_numNearbyEggs;
            
            if( m_numNearbyEggs == SPIRIT_MAXNEARBYEGGS )
            {
                break;
            }
        }
    }
}


void Spirit::SetShamanMode()
{
    m_state = StateShaman;
    m_timeSync = 5.0;
}

void Spirit::PushFromBuildings()
{
    START_PROFILE( "PushFromBuildings" );

    LList<int> *obstructions = g_app->m_location->m_obstructionGrid->GetBuildings( m_pos.x, m_pos.z );

    bool hitFound = false;

    for( int i = 0; i < obstructions->Size(); ++i )
    {
        int buildingId = obstructions->GetData(i);
        Building *building = g_app->m_location->GetBuilding( buildingId );
        if( building && building->DoesSphereHit( m_pos, 5.0 ) )
        {
            hitFound = true;
            Vector3 hitVector = ( m_pos - building->m_pos );
            m_vel += hitVector * 0.1;
            m_vel.y = 0.0;
            double speed = m_vel.Mag();
            speed = min( speed, 10.0 );
            m_vel.SetLength( speed );
        }
    }

    if( !hitFound && m_vel.Mag() < 1.0 )
    {
        // Once we have cleared any buildings we are assumed to be safe
        m_pushFromBuildings = false;
    }

    END_PROFILE(  "PushFromBuildings" );
}


void Spirit::SkipStage()
{
    m_timeSync = 0.0;
}


void Spirit::AddToGlobalWorld()
{
    int locationId = g_app->m_locationId;
    GlobalLocation *location = g_app->m_globalWorld->GetLocation( locationId );
    if( location ) location->AddSpirits(1);    
}


void Spirit::CollectorArrives()
{
    m_state = StateAttached;
    g_app->m_soundSystem->TriggerOtherEvent( this, "PickedUp", SoundSourceBlueprint::TypeSpirit );
}

void Spirit::CollectorDrops()
{
    m_vel.Zero();
    m_hover.Zero();
    m_state = StateFloating;
    m_pushFromBuildings = true;
    m_groundHeight = g_app->m_location->m_landscape.m_heightMap->GetValue( m_pos.x, m_pos.z );

//    Begin();
//    g_app->m_soundSystem->TriggerOtherEvent( this, "Dropped", SoundSourceBlueprint::TypeSpirit );
}

void Spirit::Harvest(WorldObjectId _id)
{
    m_state = StateHarvested;
    m_harvesterId = _id;
    m_hover.Zero();
}

void Spirit::InEgg()
{
    m_state = StateInEgg;
    m_vel.Zero();
    m_hover.Zero();
    //g_app->m_soundSystem->TriggerOtherEvent( this, "PlacedInEgg", SoundSourceBlueprint::TypeSpirit );
}

void Spirit::EggDestroyed()
{
    m_vel.Zero();
    m_hover.Zero();
    m_state = StateFloating;
    m_groundHeight = g_app->m_location->m_landscape.m_heightMap->GetValue( m_pos.x, m_pos.z );

//    Begin();
    //g_app->m_soundSystem->TriggerOtherEvent( this, "EggDestroyed", SoundSourceBlueprint::TypeSpirit );
}

void Spirit::Render( double predictionTime )
{       
	bool isMonsterTeam = m_teamId == g_app->m_location->GetMonsterTeamId();

	if( isMonsterTeam )
		glBlendFunc		( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR );
	
	glBegin( GL_QUADS );
    RenderWithoutGlBegin( predictionTime );
    glEnd();

	if( isMonsterTeam )
		glBlendFunc		( GL_SRC_ALPHA, GL_ONE );
}

void Spirit::RenderWithoutGlBegin( double predictionTime )
{
    int innerAlpha = 255;
    int outerAlpha = 100;
    double spiritInnerSize = 1.0;
    double spiritOuterSize = 3.0;

    if( g_app->Multiplayer() ) 
    {
        innerAlpha /= 2.0;
        outerAlpha /= 2.0;
        spiritInnerSize /= 2.0;
        spiritOuterSize /= 2.0;
    }

    if( m_state == StateBirth )
    {
        double fractionBorn = 1.2 - (m_timeSync / 6.0);
        if( fractionBorn > 1.0 ) fractionBorn = 1.0;
        //innerAlpha *= fractionBorn;
        //outerAlpha *= fractionBorn;
        spiritInnerSize *= fractionBorn;
        spiritOuterSize *= fractionBorn;
    }

    if( m_state == StateDeath )
    {
        double totalLife = 180;
        if( g_app->Multiplayer() ) totalLife /= 2.0;
        double fractionAlive = m_timeSync / totalLife;
        innerAlpha *= fractionAlive;
        outerAlpha *= fractionAlive;
        spiritInnerSize *= fractionAlive;
        spiritOuterSize *= fractionAlive;
    }

    RGBAColour colour;
    if( m_teamId != 255 )
    {
        colour = g_app->m_location->m_teams[ m_teamId ]->m_colour;      
    }
    else
    {
        colour.Set( 255, 255, 255 );
    }
    
    if( m_teamId == g_app->m_location->GetMonsterTeamId() )
    {
        colour.Set(100,100,100,0);
        innerAlpha = 0;
        outerAlpha = 0;
    }

    predictionTime -= SERVER_ADVANCE_PERIOD;
    
    Vector3 predictedPos = m_pos + predictionTime * m_vel;
    predictedPos += predictionTime * m_hover;
        
    double size = spiritInnerSize;
    glColor4ub(colour.r, colour.g, colour.b, innerAlpha );            
    
    bool renderInner = true;
    if( m_state != StateShaman && m_broken != -1.0 ) renderInner = false;

    if( renderInner )
    {
        //glBegin( GL_QUADS );
            glVertex3dv( (predictedPos - g_app->m_camera->GetUp()*size).GetData() );
            glVertex3dv( (predictedPos + g_app->m_camera->GetRight()*size).GetData() );
            glVertex3dv( (predictedPos + g_app->m_camera->GetUp()*size).GetData() );
            glVertex3dv( (predictedPos - g_app->m_camera->GetRight()*size).GetData() );
        //glEnd();    
    }

    size = spiritOuterSize;
    if( m_broken >= 0.0 )
    {
        predictedPos = m_shellPos;
        predictedPos.y -= m_broken;
        double landHeight = g_app->m_location->m_landscape.m_heightMap->GetValue(m_pos.x, m_pos.z );
        if( predictedPos.y < landHeight ) predictedPos.y = landHeight;
        outerAlpha -= m_broken * 10;
        outerAlpha = max(0, outerAlpha);
    }

    glColor4ub(colour.r, colour.g, colour.b, outerAlpha );            
    //glBegin( GL_QUADS );
        glVertex3dv( (predictedPos - g_app->m_camera->GetUp()*size).GetData() );
        glVertex3dv( (predictedPos + g_app->m_camera->GetRight()*size).GetData() );
        glVertex3dv( (predictedPos + g_app->m_camera->GetUp()*size).GetData() );
        glVertex3dv( (predictedPos - g_app->m_camera->GetRight()*size).GetData() );
    //glEnd();    
}

int Spirit::NumNearbyEggs()
{
    //
    // Search for nearby eggs if its time to do so

    double timeNow = GetNetworkTime();
    if( timeNow > m_eggSearchTimer )
    {
        SearchForNearbyEggs();
        m_eggSearchTimer = timeNow + 3.0;
    }


    return m_numNearbyEggs;
}

void Spirit::BreakSpirit()
{
    if( m_broken == -1.0 )
    {
        m_broken = 0.0;
        m_shellPos = m_pos;
        if( m_id.GetTeamId() != 255 )
        {
            for( int i = 0; i < 5; ++i )
            {
                Vector3 vel = m_vel + Vector3( FRAND(5.0), FRAND(5.0), FRAND(5.0) );
                g_app->m_particleSystem->CreateParticle(m_pos, vel, Particle::TypeControlFlash, -1.0, g_app->m_location->m_teams[ m_id.GetTeamId() ]->m_colour );
            }
        }
    }
}

WorldObjectId *Spirit::GetNearbyEggs()
{
    return m_nearbyEggs;
}

