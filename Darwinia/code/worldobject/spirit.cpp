#include "lib/universal_include.h"

#include <math.h>

#include "lib/math_utils.h"
#include "lib/debug_utils.h"
#include "lib/debug_render.h"

#include "app.h"
#include "camera.h"
#include "entity_grid.h"
#include "obstruction_grid.h"
#include "location.h"
#include "team.h"
#include "global_world.h"

#include "sound/soundsystem.h"

#include "worldobject/spirit.h"
#include "worldobject/virii.h"
#include "worldobject/egg.h"

// #################
// ### CARRYABLE ###
// #################

Carryable::Carryable()
:   WorldObject(),
	m_state(0),
	m_teamId(255),
    m_positionOffset(0.0f),
    m_pushFromBuildings(true)
{
}

Carryable::~Carryable()
{
}

void Carryable::Begin()
{
    m_timeSync = 6.0f;

    m_positionOffset = syncfrand(10.0f);
    m_xaxisRate = syncfrand(2.0f);
    m_yaxisRate = syncfrand(2.0f);
    m_zaxisRate = syncfrand(2.0f);

    g_app->m_soundSystem->TriggerOtherEvent( this, "Create", SoundSourceBlueprint::TypeSpirit );
}

void Carryable::CollectorArrives()
{
    g_app->m_soundSystem->TriggerOtherEvent( this, "PickedUp", SoundSourceBlueprint::TypeSpirit );
}

void Carryable::CollectorDrops()
{
    m_vel.Zero();
    m_hover.Zero();
    m_pushFromBuildings = true;
	m_timeSync = 120.0f + syncsfrand(60.0f);
    g_app->m_soundSystem->TriggerOtherEvent( this, "Dropped", SoundSourceBlueprint::TypeSpirit );

}

void Carryable::PushFromBuildings()
{
    LList<int> *obstructions = g_app->m_location->m_obstructionGrid->GetBuildings( m_pos.x, m_pos.z );

    bool hitFound = false;

    for( int i = 0; i < obstructions->Size(); ++i )
    {
        int buildingId = obstructions->GetData(i);
        Building *building = g_app->m_location->GetBuilding( buildingId );
        if( building && building->DoesSphereHit( m_pos, 5.0f ) )
        {
            hitFound = true;
            Vector3 hitVector = ( m_pos - building->m_pos );
            m_vel += hitVector * 0.1f;
            m_vel.y = 0.0f;
            float speed = m_vel.Mag();
            speed = min( speed, 10.0f );
            m_vel.SetLength( speed );
        }
    }

    if( !hitFound && m_vel.Mag() < 1.0f )
    {
        // Once we have cleared any buildings we are assumed to be safe
        m_pushFromBuildings = false;
    }
}


void Carryable::SkipStage()
{
    m_timeSync = 0.0f;
}

// ##############
// ### SPIRIT ###
// ##############

Spirit::Spirit()
:   Carryable(),
    m_eggSearchTimer(0.0f),
    m_numNearbyEggs(0)
{
	m_state = StateUnknown;
	m_type = TYPE_SPIRIT;
}

Spirit::~Spirit()
{
}

void Spirit::Begin()
{
	Carryable::Begin();

	m_state = StateBirth;

	m_numNearbyEggs = 0;
    m_eggSearchTimer = 0.0f;

}

bool Spirit::Advance()
{
    m_vel *= 0.9f;

    if( m_state != StateAttached &&
        m_state != StateInEgg )
    {
        //
        // Make me float around slowly

        m_timeSync -= SERVER_ADVANCE_PERIOD;
        m_positionOffset += SERVER_ADVANCE_PERIOD;
        m_xaxisRate += syncsfrand(1.0f);
        m_yaxisRate += syncsfrand(1.0f);
        m_zaxisRate += syncsfrand(1.0f);
        if( m_xaxisRate > 2.0f ) m_xaxisRate = 2.0f;
        if( m_xaxisRate < 0.0f ) m_xaxisRate = 0.0f;
        if( m_yaxisRate > 2.0f ) m_yaxisRate = 2.0f;
        if( m_yaxisRate < 0.0f ) m_yaxisRate = 0.0f;
        if( m_zaxisRate > 2.0f ) m_zaxisRate = 2.0f;
        if( m_zaxisRate < 0.0f ) m_zaxisRate = 0.0f;
        m_hover.x = sinf( m_positionOffset ) * m_xaxisRate;
        m_hover.y = sinf( m_positionOffset ) * m_yaxisRate;
        m_hover.z = sinf( m_positionOffset ) * m_zaxisRate;

        switch( m_state )
        {
            case StateBirth:
            {
                m_hover.y = max( m_hover.y, 0.0f + syncfrand(0.5f) );
                float heightAboveGround = m_pos.y - g_app->m_location->m_landscape.m_heightMap->GetValue( m_pos.x, m_pos.z );
                if( heightAboveGround > 10.0f )
                {
                    float fractionAboveGround = heightAboveGround / 100.0f;
                    fractionAboveGround = min( fractionAboveGround, 1.0f );
                    m_hover.y = (-10.0f - syncfrand(10.0f)) * fractionAboveGround;
                }
                else if( m_timeSync <= 0.0f )
                {
                    m_state = StateFloating;
                    m_timeSync = 120.0f + syncsfrand(60.0f);
                }
                break;
            }

            case StateFloating:
                if( m_timeSync <= 0.0f )
                {
                    m_state = StateDeath;
                    g_app->m_soundSystem->TriggerOtherEvent( this, "BeginAscent", SoundSourceBlueprint::TypeSpirit );
                    m_timeSync = 180.0f;
                    AddToGlobalWorld();
                }
                break;

            case StateInStore:
                break;

            case StateDeath:
                m_hover.y = max(m_hover.y, 2.0f + syncfrand(2.0f));
                if( m_timeSync <= 0.0f )
                {
                    // We are now dead
                    return true;
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
    float worldSizeX = g_app->m_location->m_landscape.GetWorldSizeX();
    float worldSizeZ = g_app->m_location->m_landscape.GetWorldSizeZ();
    if( m_pos.x < 0.0f ) m_pos.x = 0.0f;
    if( m_pos.z < 0.0f ) m_pos.z = 0.0f;
    if( m_pos.x >= worldSizeX ) m_pos.x = worldSizeX;
    if( m_pos.z >= worldSizeZ ) m_pos.z = worldSizeZ;

    if( m_state != StateInEgg && m_state != StateDeath )
    {
        float landHeight = g_app->m_location->m_landscape.m_heightMap->GetValue( m_pos.x, m_pos.z );
        if( m_pos.y < landHeight + 2.0f ) m_pos.y = landHeight + 2.0f;
    }

    //m_vel = (m_pos - oldPos) / SERVER_ADVANCE_PERIOD;

    //
    // Update nearby eggs

    if( m_state == StateFloating )
    {
        m_eggSearchTimer -= SERVER_ADVANCE_PERIOD;

        if( m_eggSearchTimer <= 0.0f )
        {
            m_eggSearchTimer = 3.0f;
            m_numNearbyEggs = 0;

            int numNeighbours;
            WorldObjectId *ids = g_app->m_location->m_entityGrid->GetNeighbours(
								    m_pos.x, m_pos.z, VIRII_MAXSEARCHRANGE, &numNeighbours );

            for( int i = 0; i < numNeighbours; ++i )
            {
                WorldObjectId id = ids[i];
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
    }

    return false;
}

void Spirit::AddToGlobalWorld()
{
    int locationId = g_app->m_locationId;
    GlobalLocation *location = g_app->m_globalWorld->GetLocation( locationId );
    DarwiniaDebugAssert(location);
    location->AddSpirits(1);
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
	m_timeSync = 120.0f + syncsfrand(60.0f);
//    Begin();
    g_app->m_soundSystem->TriggerOtherEvent( this, "Dropped", SoundSourceBlueprint::TypeSpirit );
}

void Spirit::InEgg()
{
    m_state = StateInEgg;
    m_vel.Zero();
    m_hover.Zero();
    g_app->m_soundSystem->TriggerOtherEvent( this, "PlacedInEgg", SoundSourceBlueprint::TypeSpirit );
}

void Spirit::EggDestroyed()
{
    m_vel.Zero();
    m_hover.Zero();
    m_state = StateFloating;
//    Begin();
    g_app->m_soundSystem->TriggerOtherEvent( this, "EggDestroyed", SoundSourceBlueprint::TypeSpirit );
}

void Spirit::Render( float predictionTime )
{
    int innerAlpha = 255;
    int outerAlpha = 100;
    float spiritInnerSize = 1.0;
    float spiritOuterSize = 3.0;

    if( m_state == StateBirth )
    {
        float fractionBorn = 1.2f - (m_timeSync / 6.0f);
        if( fractionBorn > 1.0f ) fractionBorn = 1.0f;
        //innerAlpha *= fractionBorn;
        //outerAlpha *= fractionBorn;
        spiritInnerSize *= fractionBorn;
        spiritOuterSize *= fractionBorn;
    }

    if( m_state == StateDeath )
    {
        float fractionAlive = m_timeSync / 180.0f;
        innerAlpha *= fractionAlive;
        outerAlpha *= fractionAlive;
        spiritInnerSize *= fractionAlive;
        spiritOuterSize *= fractionAlive;
    }

    RGBAColour colour;
    if( m_teamId != 255 )
    {
        colour = g_app->m_location->m_teams[ m_teamId ].m_colour;
    }
    else
    {
        colour.Set( 255, 255, 255 );
    }

    predictionTime -= SERVER_ADVANCE_PERIOD;

    Vector3 predictedPos = m_pos + predictionTime * m_vel;
    predictedPos += predictionTime * m_hover;

    float size = spiritInnerSize;
    glColor4ub(colour.r, colour.g, colour.b, innerAlpha );

    glBegin( GL_QUADS );
        glVertex3fv( (predictedPos - g_app->m_camera->GetUp()*size).GetData() );
        glVertex3fv( (predictedPos + g_app->m_camera->GetRight()*size).GetData() );
        glVertex3fv( (predictedPos + g_app->m_camera->GetUp()*size).GetData() );
        glVertex3fv( (predictedPos - g_app->m_camera->GetRight()*size).GetData() );
    glEnd();

    size = spiritOuterSize;
    glColor4ub(colour.r, colour.g, colour.b, outerAlpha );
    glBegin( GL_QUADS );
        glVertex3fv( (predictedPos - g_app->m_camera->GetUp()*size).GetData() );
        glVertex3fv( (predictedPos + g_app->m_camera->GetRight()*size).GetData() );
        glVertex3fv( (predictedPos + g_app->m_camera->GetUp()*size).GetData() );
        glVertex3fv( (predictedPos - g_app->m_camera->GetRight()*size).GetData() );
    glEnd();
}

int Spirit::NumNearbyEggs()
{
    return m_numNearbyEggs;
}

WorldObjectId *Spirit::GetNearbyEggs()
{
    return m_nearbyEggs;
}

// ##############
// ### SPIRIT ###
// ##############

LoosePolygon::LoosePolygon()
:   Carryable()
{
	m_state = StateUnknown;
	m_type = TYPE_POLYGON;
}

LoosePolygon::~LoosePolygon()
{
}

void LoosePolygon::Begin()
{
	Carryable::Begin();

	m_state = StateBirth;

}

bool LoosePolygon::Advance()
{
    m_vel *= 0.9f;

    if( m_state != StateAttached )
    {
        //
        // Make me float around slowly

        m_timeSync -= SERVER_ADVANCE_PERIOD;
        m_positionOffset += SERVER_ADVANCE_PERIOD;
        m_xaxisRate += syncsfrand(1.0f);
        m_yaxisRate += syncsfrand(1.0f);
        m_zaxisRate += syncsfrand(1.0f);
        if( m_xaxisRate > 2.0f ) m_xaxisRate = 2.0f;
        if( m_xaxisRate < 0.0f ) m_xaxisRate = 0.0f;
        if( m_yaxisRate > 2.0f ) m_yaxisRate = 2.0f;
        if( m_yaxisRate < 0.0f ) m_yaxisRate = 0.0f;
        if( m_zaxisRate > 2.0f ) m_zaxisRate = 2.0f;
        if( m_zaxisRate < 0.0f ) m_zaxisRate = 0.0f;
        m_hover.x = sinf( m_positionOffset ) * m_xaxisRate;
        m_hover.y = sinf( m_positionOffset ) * m_yaxisRate;
        m_hover.z = sinf( m_positionOffset ) * m_zaxisRate;

        switch( m_state )
        {
            case StateBirth:
            {
                m_hover.y = max( m_hover.y, 0.0f + syncfrand(0.5f) );
                float heightAboveGround = m_pos.y - g_app->m_location->m_landscape.m_heightMap->GetValue( m_pos.x, m_pos.z );
                if( heightAboveGround > 10.0f )
                {
                    float fractionAboveGround = heightAboveGround / 100.0f;
                    fractionAboveGround = min( fractionAboveGround, 1.0f );
                    m_hover.y = (-10.0f - syncfrand(10.0f)) * fractionAboveGround;
                }
                else if( m_timeSync <= 0.0f )
                {
                    m_state = StateFloating;
                    m_timeSync = 120.0f + syncsfrand(60.0f);
                }
                break;
            }

            case StateFloating:
                if( m_timeSync <= 0.0f )
                {
                    m_state = StateDeath;
                    g_app->m_soundSystem->TriggerOtherEvent( this, "BeginAscent", SoundSourceBlueprint::TypeSpirit );
                    m_timeSync = 180.0f;
                }
                break;

            case StateDeath:
				return true;
                break;
        };
    }

    Vector3 oldPos = m_pos;

    if( m_pushFromBuildings &&
        m_state != StateDeath )
    {
        PushFromBuildings();
    }

    m_pos += m_vel * SERVER_ADVANCE_PERIOD;
    m_pos += m_hover * SERVER_ADVANCE_PERIOD;
    float worldSizeX = g_app->m_location->m_landscape.GetWorldSizeX();
    float worldSizeZ = g_app->m_location->m_landscape.GetWorldSizeZ();
    if( m_pos.x < 0.0f ) m_pos.x = 0.0f;
    if( m_pos.z < 0.0f ) m_pos.z = 0.0f;
    if( m_pos.x >= worldSizeX ) m_pos.x = worldSizeX;
    if( m_pos.z >= worldSizeZ ) m_pos.z = worldSizeZ;

    if( m_state != StateDeath )
    {
        float landHeight = g_app->m_location->m_landscape.m_heightMap->GetValue( m_pos.x, m_pos.z );
        if( m_pos.y < landHeight + 2.0f ) m_pos.y = landHeight + 2.0f;
    }

    //m_vel = (m_pos - oldPos) / SERVER_ADVANCE_PERIOD;
    return false;
}

void LoosePolygon::CollectorArrives()
{
    m_state = StateAttached;
    g_app->m_soundSystem->TriggerOtherEvent( this, "PickedUp", SoundSourceBlueprint::TypeSpirit );
}

void LoosePolygon::CollectorDrops()
{
    m_vel.Zero();
    m_hover.Zero();
    m_state = StateFloating;
    m_pushFromBuildings = true;
	m_timeSync = 120.0f + syncsfrand(60.0f);
//    Begin();
    g_app->m_soundSystem->TriggerOtherEvent( this, "Dropped", SoundSourceBlueprint::TypeSpirit );
}

void LoosePolygon::Render( float predictionTime )
{
    int innerAlpha = 255;
    int outerAlpha = 100;
    float spiritInnerSize = 1.0;
    float spiritOuterSize = 3.0;

    if( m_state == StateBirth )
    {
        float fractionBorn = 1.2f - (m_timeSync / 6.0f);
        if( fractionBorn > 1.0f ) fractionBorn = 1.0f;
        //innerAlpha *= fractionBorn;
        //outerAlpha *= fractionBorn;
        spiritInnerSize *= fractionBorn;
        spiritOuterSize *= fractionBorn;
    }

    if( m_state == StateDeath )
    {
        float fractionAlive = m_timeSync / 180.0f;
        innerAlpha *= fractionAlive;
        outerAlpha *= fractionAlive;
        spiritInnerSize *= fractionAlive;
        spiritOuterSize *= fractionAlive;
    }

    RGBAColour colour;
    if( m_teamId != 255 )
    {
        colour = g_app->m_location->m_teams[ m_teamId ].m_colour;
    }
    else
    {
        colour.Set( 255, 255, 255 );
    }

    predictionTime -= SERVER_ADVANCE_PERIOD;

    Vector3 predictedPos = m_pos + predictionTime * m_vel;
    predictedPos += predictionTime * m_hover;

    float size = spiritInnerSize;
/*    glColor4ub(colour.r, colour.g, colour.b, innerAlpha );

    glBegin( GL_QUADS );
        glVertex3fv( (predictedPos - g_app->m_camera->GetUp()*size).GetData() );
        glVertex3fv( (predictedPos + g_app->m_camera->GetRight()*size).GetData() );
        glVertex3fv( (predictedPos + g_app->m_camera->GetUp()*size).GetData() );
        glVertex3fv( (predictedPos - g_app->m_camera->GetRight()*size).GetData() );
    glEnd();
*/
    size = spiritOuterSize;
    glColor4ub(colour.r, colour.g, colour.b, outerAlpha );
    glBegin( GL_TRIANGLES );
//    glBegin( GL_QUADS );
        glVertex3fv( (predictedPos - g_app->m_camera->GetUp()*size).GetData() );
        glVertex3fv( (predictedPos + g_app->m_camera->GetRight()*size).GetData() );
        glVertex3fv( (predictedPos + g_app->m_camera->GetUp()*size).GetData() );
//        glVertex3fv( (predictedPos - g_app->m_camera->GetRight()*size).GetData() );
    glEnd();
}


