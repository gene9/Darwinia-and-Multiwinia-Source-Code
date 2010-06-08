#include "lib/universal_include.h"
#include "lib/resource.h"
#include "lib/math/random_number.h"

#include "statue.h"

#include "explosion.h"
#include "app.h"
#include "location.h"
#include "main.h"
#include "global_world.h"
#include "multiwinia.h"
#include "multiwiniahelp.h"

#include "worldobject/ai.h"

#define STATUE_SPECIALFILENAME "carryspecialshape.shp"


Statue::Statue()
:   CarryableBuilding(),
    m_waypointCheck(0.0),
	m_aiTarget(NULL),
    m_renderSpecial(false)
{
    m_type = TypeStatue;
        
    m_minLifters = 20;
    m_maxLifters = 50;
}


void Statue::Initialise( Building *_template )
{
    CarryableBuilding::Initialise( _template );

    int randomShape = syncrand() % 8;
            
    switch(randomShape)
    {
        case 0:     SetShape( g_app->m_resource->GetShape("carrystatue.shp") );     break;
        case 1:     SetShape( g_app->m_resource->GetShape("carrysquaddie.shp") );   break;
        case 2:     SetShape( g_app->m_resource->GetShape("carryglobe.shp") );      break;
        case 3:     SetShape( g_app->m_resource->GetShape("carrytelescope.shp") );  break;
        case 4:     SetShape( g_app->m_resource->GetShape("carryweights.shp") );    break;
        case 5:     SetShape( g_app->m_resource->GetShape("carrycube.shp" ) );      break;
        case 6:     SetShape( g_app->m_resource->GetShape("carryteapot.shp") );     break;
        case 7:     SetShape( g_app->m_resource->GetShape("carrybillboard.shp") ); 
                    m_renderSpecial = true;
                    break;

        //case 4:     SetShape( g_app->m_resource->GetShape("carryspider.shp") );     break;
        //case 5:     SetShape( g_app->m_resource->GetShape("carrydollar.shp") );     break;        
    }
        

    m_carryRadius = 33.0;
    m_radius = m_carryRadius;

    if( !g_app->m_multiwinia->GameInGracePeriod() )
    {
        ExplodeShape( 0.5 );
    }
}


bool Statue::Advance()
{
#ifdef INCLUDEGAMEMODE_CTS

    if( m_lifted && m_id.GetTeamId() == g_app->m_globalWorld->m_myTeamId )
    {
        g_app->m_multiwiniaHelp->NotifyLiftedStatue();
    }


    //
    // Create an AI target if we need one

  	if( !m_aiTarget && !m_destroyed )
    {
        m_aiTarget = (AITarget *)Building::CreateBuilding( Building::TypeAITarget );

        Matrix34 mat( m_front, g_upVector, m_pos );

        AITarget targetTemplate;
        targetTemplate.m_pos       = m_pos;
        targetTemplate.m_pos.y     = g_app->m_location->m_landscape.m_heightMap->GetValue(m_pos.x, m_pos.z);

        m_aiTarget->Initialise( (Building *)&targetTemplate );
        m_aiTarget->m_id.SetUnitId( UNIT_BUILDINGS );
        m_aiTarget->m_id.SetUniqueId( g_app->m_globalWorld->GenerateBuildingId() );   
        m_aiTarget->m_priorityModifier = 1.0;
		m_aiTarget->m_statueTarget = m_id.GetUniqueId();

        g_app->m_location->m_buildings.PutData( m_aiTarget );

        g_app->m_location->RecalculateAITargets();
    }

    if( m_aiTarget )
    {
	    if( (m_pos - m_aiTarget->m_pos).Mag() > m_carryRadius * 1.5 )
	    {
		    m_aiTarget->m_pos = m_pos;
		    g_app->m_location->RecalculateAITargets();
	    }
    }
    

    //
    // If we don't have a waypoint, try to pick one intelligently

    double dist = ( m_waypoint - m_pos ).Mag();

    bool badWaypoint = ( dist < 1.0 || m_waypoint == g_zeroVector );

    if( m_id.GetTeamId() != 255 && badWaypoint )
    {
        AppDebugOut( "Statue bad waypoint\n" );
        double timeNow = GetNetworkTime();

        if( timeNow >= m_waypointCheck )
        {
            AppDebugOut ( "Looking for new waypoint, team = %d\n", m_id.GetTeamId() );
            m_waypointCheck = timeNow + 1.0;

            int requiredTeamId = m_id.GetTeamId();
            if( m_id.GetTeamId() == g_app->m_location->GetMonsterTeamId() ||
                m_id.GetTeamId() == g_app->m_location->GetFuturewinianTeamId() ) 
            {
                requiredTeamId = 255;
            }


            double nearest = 999999.0;
            int nearestId = -1;
            
            for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
            {
                if( g_app->m_location->m_buildings.ValidIndex(i) )
                {
                    Building *building = g_app->m_location->m_buildings[i];
                    if( building &&
                        building->m_type == Building::TypeMultiwiniaZone &&
                        building->m_id.GetTeamId() == requiredTeamId )
                    {
                        double distance = ( building->m_pos - m_pos ).Mag();
                        if( distance < nearest )
                        {
                            nearest = distance;
                            nearestId = building->m_id.GetUniqueId();
                        }
                    }
                }
            }

            if( nearestId != -1 )
            {
                Building *building = g_app->m_location->GetBuilding( nearestId );

                m_routeId = g_app->m_location->m_routingSystem.GenerateRoute( m_pos, building->m_pos, false, false );

                if( m_id.GetTeamId() == g_app->m_globalWorld->m_myTeamId )
                {
                    //g_app->m_location->m_routingSystem.SetRouteIntensity( m_routeId, 1.0 );
                    g_app->m_location->m_routingSystem.HighlightRoute( m_routeId, m_id.GetTeamId() );
                }

                AppDebugOut( "Statue set waypoint to building with teamId %d\n", building->m_id.GetTeamId() );
                m_routeWayPointId = -1;
            }
        }
    }    

    //
    // Fall and explode if we are tilted too much
    // Or if we're in the water

    if( m_up.y < 0.7 ||
        m_pos.y < 0 )
    {
        Destroy();
    }

#endif
    return CarryableBuilding::Advance();
}


void Statue::Render( double _predictionTime )
{
    g_app->m_location->SetupSpecialLights();

    if( m_renderSpecial && g_app->m_resource->DoesShapeExist( STATUE_SPECIALFILENAME ) )
    {
        Vector3 predictedPos = m_pos + m_vel * _predictionTime;
        if( m_lifted ) predictedPos.y += 5.0;

        Vector3 predictedUp = g_app->m_location->m_landscape.m_normalMap->GetValue(predictedPos.x, predictedPos.z);
        predictedUp.y *= 1.5;
        predictedUp.Normalise();

        Vector3 predictedRight = m_front ^ g_upVector;
        predictedRight.Normalise();
        Vector3 predictedFront = predictedUp ^ predictedRight;

        if( m_shape )
        {
	        Matrix34 mat(predictedFront, predictedUp, predictedPos);
            mat.f *= m_scale;
            mat.u *= m_scale;
            mat.r *= m_scale;

            glEnable( GL_NORMALIZE );
            glDisable( GL_CULL_FACE );

            Shape *shape = g_app->m_resource->GetShape( STATUE_SPECIALFILENAME );
            if( shape )
            {
                shape->Render(_predictionTime, mat);
            }


            glEnable( GL_CULL_FACE );
            glDisable( GL_NORMALIZE );
        }
    }
    else
    {
        CarryableBuilding::Render( _predictionTime );
    }

    g_app->m_location->SetupLights();

    glEnable        (GL_LIGHTING);
    glEnable        (GL_LIGHT0);
    glEnable        (GL_LIGHT1);
}


void Statue::Destroy()
{
    ExplodeShape( 1.0 );

    m_destroyed = true;
	if( m_aiTarget )
    {
        m_aiTarget->m_destroyed = true;
        m_aiTarget->m_neighbours.EmptyAndDelete();
        m_aiTarget = NULL;

        // Note by chris - you dont need to call this here
        // Because Location::AdvanceBuildings will take care of it later
        //g_app->m_location->RecalculateAITargets();
    }
}


void Statue::ExplodeShape( double _fraction )
{
    Matrix34 transform( m_front, m_up, m_pos );
    transform.f *= m_scale;
    transform.u *= m_scale;
    transform.r *= m_scale;

    g_explosionManager.AddExplosion( m_shape, transform, _fraction );
}
