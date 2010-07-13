#include "lib/universal_include.h"
#include "lib/resource.h"
#include "lib/math/random_number.h"
#include "lib/language_table.h"
#include "lib/debug_render.h"

#include "camera.h"

#include "explosion.h"
#include "app.h"
#include "level_file.h"
#include "location.h"
#include "main.h"
#include "global_world.h"
#include "multiwiniahelp.h"
#include "obstruction_grid.h"

#include "worldobject/ai.h"
#include "worldobject/wallbuster.h"
#include "worldobject/pulsebomb.h"
#include "worldobject/aiobjective.h"


WallBuster::WallBuster()
:   CarryableBuilding(),
    m_waypointCheck(0.0),
	m_aiTarget(NULL),
    m_destructTimer(10.0)
{
    m_type = TypeWallBuster;
        
    m_minLifters = 10;
    m_maxLifters = 30;
    m_speedScale = 0.6;

    SetShape( g_app->m_resource->GetShape( "wallbuster.shp" ) );
    m_scale = 10.0;
}


void WallBuster::Initialise( Building *_template )
{
    CarryableBuilding::Initialise( _template );    

    m_carryRadius = 13.0;
    m_radius = m_carryRadius;
}


bool WallBuster::Advance()
{
    bool ownedByAttackers = false;
    if( m_id.GetTeamId() != 255 ) ownedByAttackers = g_app->m_location->m_levelFile->m_attacking[ g_app->m_location->GetTeamPosition( m_id.GetTeamId() ) ];
    if( !ownedByAttackers && m_id.GetTeamId() != 255 &&
        m_numLifters >= m_minLifters )
    {
        m_destructTimer -= SERVER_ADVANCE_PERIOD;
        if( m_destructTimer <= 0.0 )
        {
            Destroy();

            g_app->m_location->Bang( m_pos, 50.0, 500.0 );
        }
    }
    else
    {
        m_destructTimer = 10.0;
    }

	/*if( !m_aiTarget && !m_destroyed )
    {
        m_aiTarget = (AITarget *)Building::CreateBuilding( Building::TypeAITarget );

        Matrix34 mat( m_front, g_upVector, m_pos );

        AITarget targetTemplate;
        targetTemplate.m_pos       = m_pos;
        targetTemplate.m_pos.y     = g_app->m_location->m_landscape.m_heightMap->GetValue(m_pos.x, m_pos.z);

        m_aiTarget->Initialise( (Building *)&targetTemplate );
        m_aiTarget->m_id.SetUnitId( UNIT_BUILDINGS );
        m_aiTarget->m_id.SetUniqueId( g_app->m_globalWorld->GenerateBuildingId() );   
        //m_aiTarget->m_priorityModifier = .0f;
		//m_aiTarget->m_statueTarget = true;

        g_app->m_location->m_buildings.PutData( m_aiTarget );

        g_app->m_location->RecalculateAITargets();
    }*/

    if( m_aiTarget )
    {
	    if( (m_pos - m_aiTarget->m_pos).Mag() > m_carryRadius * 1.5 )
	    {
		    m_aiTarget->m_pos = m_pos;
		    g_app->m_location->RecalculateAITargets();
	    }
    }

    LList<int> *buildings = g_app->m_location->m_obstructionGrid->GetBuildings( m_pos.x, m_pos.z );

    for( int i = 0; i < buildings->Size(); ++i )
    {
        Building *b = g_app->m_location->GetBuilding( buildings->GetData(i) );        
        if( b && b->m_type == Building::TypeWall )
        {
            if( b->DoesSphereHit( m_pos, 20 ) )
            {
                ExplodeShape(1.0);
                BreakWalls();

                g_app->m_location->Bang( m_pos, 50.0, 150.0 );
                Destroy();

                g_app->m_location->SetCurrentMessage( LANGUAGEPHRASE("dialog_wallbreach"));

                break;
            }
        }
    }


    return CarryableBuilding::Advance();
}

void WallBuster::RunAI( AI *_ai)
{
    double dist = ( m_waypoint - m_pos ).Mag();

    bool badWaypoint = ( dist < 1.0 || m_waypoint == g_zeroVector );

    if( badWaypoint )
    {
        double timeNow = GetNetworkTime();

        if( timeNow >= m_waypointCheck )
        {
            m_waypointCheck = timeNow + 1.0;


            double nearest = 999999.0;
            int nearestId = -1;
            
            AIObjective *currentObjective = (AIObjective *)g_app->m_location->GetBuilding( _ai->m_currentObjective );
            for( int b = 0; b < g_app->m_location->m_buildings.Size(); ++b )
            {
                if( g_app->m_location->m_buildings.ValidIndex(b) )
                {
                    Building *building = g_app->m_location->m_buildings[b];
                    if( building &&
                        building->m_type == Building::TypeWall )
                    {
                        double distance = ( building->m_pos - m_pos ).Mag();
                        if( distance < nearest )
                        {
                            bool closeEnough = false;
                            for( int j = 0; j < currentObjective->m_objectiveMarkers.Size(); ++j )
                            {
                                Building *marker = g_app->m_location->GetBuilding( currentObjective->m_objectiveMarkers[j] );
                                if( (building->m_pos - marker->m_pos).Mag() < 500.0 ) closeEnough = true;
                            }

                            if( closeEnough )
                            {
                                nearest = distance;
                                nearestId = building->m_id.GetUniqueId();
                            }
                        }
                    }
                }
            }

            if( nearestId != -1 )
            {
                Building *building = g_app->m_location->GetBuilding( nearestId );

                m_routeId = g_app->m_location->m_routingSystem.GenerateRoute( m_pos, building->m_pos, false, true );
                m_routeWayPointId = -1;
            }
        }
    }    
}

void WallBuster::BreakWalls()
{
    for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
    {
        if( g_app->m_location->m_buildings.ValidIndex(i) )
        {
            Building *b = g_app->m_location->m_buildings[i];
            if( b->m_type == Building::TypeWall )
            {
                if( (m_pos - b->m_pos).Mag() <= 200.0 )
                {
                    b->Damage(-1000);
                }
            }
        }
    }
}

void WallBuster::Render( double _predictionTime )
{
    g_app->m_location->SetupSpecialLights();

    CarryableBuilding::Render( _predictionTime );

    g_app->m_location->SetupLights();

    glEnable        (GL_LIGHTING);
    glEnable        (GL_LIGHT0);
    glEnable        (GL_LIGHT1);
}


void WallBuster::Destroy()
{
    ExplodeShape( 1.0 );

    m_destroyed = true;
	if( m_aiTarget )
    {
        m_aiTarget->m_destroyed = true;
        m_aiTarget->m_neighbours.Empty();
        m_aiTarget = NULL;

        // Note by chris - you dont need to call this here
        // Because Location::AdvanceBuildings will take care of it later
        //g_app->m_location->RecalculateAITargets();
    }
}


void WallBuster::ExplodeShape( double _fraction )
{
    Matrix34 transform( m_front, m_up, m_pos );
    transform.f *= m_scale;
    transform.u *= m_scale;
    transform.r *= m_scale;

    g_explosionManager.AddExplosion( m_shape, transform, _fraction );
}
