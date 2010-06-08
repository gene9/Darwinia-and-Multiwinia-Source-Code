#include "lib/universal_include.h"
#include "lib/resource.h"
#include "lib/shape.h"
#include "lib/math_utils.h"
#include "lib/debug_render.h"
#include "lib/hi_res_time.h"
#include "lib/text_renderer.h"
#include "lib/preferences.h"
#include "lib/profiler.h"
#include "lib/math/random_number.h"

#include "lib/input/input.h"
#include "lib/input/input_types.h"

#include "worldobject/ai.h"
#include "worldobject/officer.h"
#include "worldobject/teleport.h"
#include "worldobject/darwinian.h"

#include "app.h"
#include "location.h"
#include "team.h"
#include "renderer.h"
#include "main.h"
#include "taskmanager.h"
#include "camera.h"
#include "particle_system.h"
#include "explosion.h"
#include "obstruction_grid.h"
#include "entity_grid.h"
#include "user_input.h"
#ifdef USE_SEPULVEDA_HELP_TUTORIAL
    #include "sepulveda.h"
#endif
#include "global_world.h"
#include "markersystem.h"

#include "sound/soundsystem.h"


Officer::Officer()
:   Entity(),
    m_demoted(false),
    m_ordersBuildingId(-1),
    m_wayPointTeleportId(-1),
    m_shield(0),
    m_absorb(false),
    m_lastOrdersSet(0.0),
    m_formation(false),
    m_absorbTimer(2.0),
    m_turnToTarget(false),
    m_orderRouteId(-1),
    m_lastOrderCreated(0.0),
    m_noFormations(false),
    m_formationAngleSet(false)
{
    m_type = TypeOfficer;
    m_state = StateIdle;
    m_orders = OrderNone;

    /*m_shape = g_app->m_resource->GetShape( "darwinian.shp" );
    AppReleaseAssert( m_shape, "Shape not found : officer.shp" );

    const char flagMarkerName[] = "MarkerFlag";
    m_flagMarker = m_shape->m_rootFragment->LookupMarker( flagMarkerName );
    AppReleaseAssert( m_flagMarker, "Officer: Can't get Marker(%s) from shape(%s), probably a corrupted file\n", flagMarkerName, m_shape->m_name );

    m_centrePos = m_shape->CalculateCentre(g_identityMatrix34);
    m_radius = m_shape->CalculateRadius(g_identityMatrix34, m_centrePos);*/
}


Officer::~Officer()
{
    if( m_id.GetTeamId() != 255 )
    {
        Team *team = g_app->m_location->m_teams[ m_id.GetTeamId() ];
        team->UnRegisterSpecial( m_id );
    }
}


void Officer::Begin()
{
    char filename[256];
    Team *team = g_app->m_location->m_teams[m_id.GetTeamId()];
    int colourId = team->m_lobbyTeam->m_colourId;
    int groupId = team->m_lobbyTeam->m_coopGroupPosition;
	if( g_app->IsSinglePlayer() )
	{
		sprintf( filename, "officer.shp" );
	}
	else
	{
		sprintf( filename, "officer.shp_%d_%d", colourId, groupId );
	}
    m_shape = g_app->m_resource->GetShape(filename, false);
    if( !m_shape )
    {
        m_shape = g_app->m_resource->GetShapeCopy( "officer.shp", false, false );
		if( !g_app->IsSinglePlayer() )
		{
			RGBAColour colour = team->m_colour;// * 0.3f;
			ConvertShapeColoursToTeam( m_shape, team->m_teamId, true);
		}

		g_app->m_resource->AddShape( m_shape, filename );
    }

    const char flagMarkerName[] = "MarkerFlag";
    m_flagMarker = m_shape->m_rootFragment->LookupMarker( flagMarkerName );
    AppReleaseAssert( m_flagMarker, "Officer: Can't get Marker(%s) from shape(%s), probably a corrupted file\n", flagMarkerName, m_shape->m_name );

    Entity::Begin();

    m_centrePos += g_upVector * 1;
    m_radius *= 2.0f;

    m_wayPoint = m_pos;
        
    m_flag.SetPosition( m_pos );
    m_flag.SetOrientation( m_front, g_upVector );
    m_flag.SetSize( 20.0 );
    m_flag.Initialise();

    if( m_id.GetTeamId() != 255 )
    {
        Team *team = g_app->m_location->m_teams[ m_id.GetTeamId() ];
        team->RegisterSpecial( m_id );
    }
}


bool Officer::ChangeHealth( int amount, int _damageType )
{
    bool dead = m_dead;
    
    if( amount < 0 && m_shield > 0 )
    {
        int shieldLoss = min( m_shield*10, -amount );
        for( int i = 0; i < shieldLoss/10.0; ++i )
        {
            Vector3 vel( SFRAND(40.0), 0.0, SFRAND(40.0) );
            g_app->m_location->SpawnSpirit( m_pos, vel, 0, WorldObjectId() );
        }
        m_shield -= shieldLoss/10.0;
        amount += shieldLoss;
    }

    Entity::ChangeHealth( amount );

    if( !dead && m_dead )
    {
        // We just died
        Matrix34 transform( m_front, g_upVector, m_pos );
        g_explosionManager.AddExplosion( m_shape, transform );         
    }

    return true;
}


void Officer::Render( double _predictionTime )
{
    RenderShape( _predictionTime );
    RenderShield( _predictionTime );


    //
    // Note by Chris
    // We don't render our flag here if this officer belongs to the players team.
    // Instead this officers flag is handled and rendered in the Marker system
    // Search for "officer->RenderFlag" in markersystem.cpp.

    if( m_enabled && !m_dead )
    {
        if( m_id.GetTeamId() != g_app->m_globalWorld->m_myTeamId ||
            g_app->m_hideInterface ) 
        {
            RenderFlag( _predictionTime );
        }
    }

    if( m_id.GetTeamId() == g_app->m_globalWorld->m_myTeamId &&
        m_orderRouteId != -1 )
    {
        //g_app->m_location->m_routingSystem.SetRouteIntensity( m_orderRouteId, 0.5 );
    }
}


void Officer::RenderSpirit( Vector3 const &_pos )
{
    Vector3 pos = _pos;

    int innerAlpha = 255;
    int outerAlpha = 40;
    int glowAlpha = 20;
    
    double spiritInnerSize = 0.5;
    double spiritOuterSize = 1.5;
    double spiritGlowSize = 10;
 
    double size = spiritInnerSize;
    glColor4ub(100, 250, 100, innerAlpha );            
    
    glDisable( GL_TEXTURE_2D );
    
    glBegin( GL_QUADS );
        glVertex3dv( (pos - g_app->m_camera->GetUp()*size).GetData() );
        glVertex3dv( (pos + g_app->m_camera->GetRight()*size).GetData() );
        glVertex3dv( (pos + g_app->m_camera->GetUp()*size).GetData() );
        glVertex3dv( (pos - g_app->m_camera->GetRight()*size).GetData() );
    glEnd();    

    size = spiritOuterSize;
    glColor4ub(100, 250, 100, outerAlpha );            
        
    glBegin( GL_QUADS );
        glVertex3dv( (pos - g_app->m_camera->GetUp()*size).GetData() );
        glVertex3dv( (pos + g_app->m_camera->GetRight()*size).GetData() );
        glVertex3dv( (pos + g_app->m_camera->GetUp()*size).GetData() );
        glVertex3dv( (pos - g_app->m_camera->GetRight()*size).GetData() );
    glEnd();    

    size = spiritGlowSize;
    glColor4ub(100,250,100, glowAlpha );

    glEnable( GL_TEXTURE_2D );
    glBindTexture( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/glow.bmp" ) );
    glBegin( GL_QUADS );
        glTexCoord2i(0,0);      glVertex3dv( (pos - g_app->m_camera->GetUp()*size).GetData() );
        glTexCoord2i(1,0);      glVertex3dv( (pos + g_app->m_camera->GetRight()*size).GetData() );
        glTexCoord2i(1,1);      glVertex3dv( (pos + g_app->m_camera->GetUp()*size).GetData() );
        glTexCoord2i(0,1);      glVertex3dv( (pos - g_app->m_camera->GetRight()*size).GetData() );
    glEnd();
    glDisable( GL_TEXTURE_2D );
}


void Officer::RenderShield( double _predictionTime )
{
    double timeIndex = g_gameTime + m_id.GetUniqueId() * 20;

    glDisable       ( GL_CULL_FACE );
    glDepthMask     ( false );
    glEnable        ( GL_BLEND );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );

    Vector3 predictedPos = m_pos + m_vel * _predictionTime;
    predictedPos.y += 10.0;

    for( int i = 0; i < m_shield; ++i )
    {
        Vector3 spiritPos = predictedPos;
        spiritPos.x += iv_sin( timeIndex * 1.8 + i * 2.0 ) * 10.0;
        spiritPos.y += iv_cos( timeIndex * 2.1 + i * 1.2 ) * 6.0;
        spiritPos.z += iv_sin( timeIndex * 2.4 + i * 1.4 ) * 10.0;

        RenderSpirit( spiritPos );
    }
    
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glDisable       ( GL_BLEND );
    glDepthMask     ( true );
    glEnable        ( GL_CULL_FACE );
}


void Officer::RenderFlag( double _predictionTime )
{
    double timeIndex = g_gameTime + m_id.GetUniqueId() * 10;
    double size = 20.0;
    bool flipped = false;

    Vector3 up = g_upVector;
    Vector3 front = m_front * -1;
    front.y = 0;
    front.Normalise();
    
    if( m_orders != OrderNone )
    {
        up.RotateAround( front * sinf(timeIndex*2) * 0.4f );
    }

    if( g_app->Multiplayer() )
    {
        double scaleFactor = g_prefsManager->GetFloat( "DarwinianScaleFactor" );
        double camDist = ( g_app->m_camera->GetPos() - m_pos ).Mag();
        if( camDist > 100 ) size *= 1.0f + scaleFactor * ( camDist - 100 ) / 1500;

        double factor = ( camDist ) / 200;            
        if( factor < 0.0 ) factor = 0.0;
        if( factor > 1.0 ) factor = 1.0;

        up = ( up * (1-factor) ) + ( g_app->m_camera->GetUp() * factor );     

        if( m_front * g_app->m_camera->GetRight() < 0 )
        {
            front = ( front * (1-factor) ) + ( g_app->m_camera->GetRight() * factor );
        }
        else
        {
            front = ( front * (1-factor) ) + ( -g_app->m_camera->GetRight() * factor );
            flipped = true;
        }
        
        up.Normalise();
        front.Normalise();
    }

    Vector3 entityUp = g_upVector;
    Vector3 entityRight(m_front ^ entityUp);
    Vector3 entityFront = entityUp ^ entityRight;        
    Matrix34 mat( entityFront, entityUp, m_pos + m_vel * _predictionTime );
   
    Vector3 flagPos = m_flagMarker->GetWorldMatrix(mat).pos;
    
    int texId = -1;
    char filename[256];
    
    if      ( m_orders == OrderNone )                   sprintf( filename, "icons/banner_none.bmp" );
    else if ( m_orders == OrderGoto )                   sprintf( filename, "icons/banner_none.bmp" );
    else if ( m_orders == OrderPrepareGoto )            sprintf( filename, "icons/banner_none.bmp" );
    else if ( m_orders == OrderFollow && m_absorb )     sprintf( filename, "icons/banner_absorb.bmp" );
    else if ( m_orders == OrderFollow && m_formation )  sprintf( filename, "icons/banner_formation.bmp" );
    else if ( m_orders == OrderFollow )                 sprintf( filename, "icons/banner_follow.bmp" );

    m_flag.SetTexture( g_app->m_resource->GetTexture( filename ) );
    //m_flag.SetPosition( flagPos );
    m_flag.SetOrientation( front, up );
    m_flag.SetSize( size );
    m_flag.SetViewFlipped( flipped );

    if( m_orders == OrderGoto ) 
    {
        m_flag.SetTarget( m_orderPosition );    
    }
    /*else if ( m_orders == OrderPrepareGoto )
    {
        Team *team = g_app->m_location->m_teams[m_id.GetTeamId()];
        if( team->m_currentEntityId == m_id.GetIndex() )
        {
            if( IsFormationToggle( team->m_currentMousePos ) )
            {
                m_flag.SetTarget( g_zeroVector );
                sprintf( filename, "icons/banner_formation.bmp" );
            }
            else
            {
                m_flag.SetTarget( team->m_currentMousePos );
            }
        }
        else
        {
            m_flag.SetTarget( m_pos + m_front * 100 );    
        }
    }*/
    else 
    {
        m_flag.SetTarget( g_zeroVector );
    }
    
    if( m_orders == OrderGoto && g_app->Multiplayer() )
    {
        if( m_orderRouteId != -1 )
        {
            Route *route = g_app->m_location->m_routingSystem.GetRoute(m_orderRouteId);
            if( route && route->m_wayPoints.Size() > 1 ) 
            {
                m_flag.SetTarget( route->m_wayPoints[1]->GetPos() );
            }
        }
    }

    m_flag.Render( g_app->Multiplayer() ? m_id.GetTeamId() : -1, filename );
}


bool Officer::RenderPixelEffect( double _predictionTime )
{
    if( g_app->Multiplayer() )
    {
        return false;
    }

    return RenderShape( _predictionTime );
}


void Officer::CalculateBoundingSphere( Vector3 &centre, double &radius )
{
    centre = m_pos + Vector3(0,4,0);
    radius = 8;

    double amountToScaleBy = 1.0;
    double scaleFactor = g_prefsManager->GetFloat( "DarwinianScaleFactor" );

    if( scaleFactor > 0.0 )
    {
        double camDist = ( g_app->m_camera->GetPos() - m_pos ).Mag();        
        if( camDist > 100 ) amountToScaleBy *= 1.0 + scaleFactor * ( camDist - 100 ) / 1000;
    }

    double changeInRadius = ( radius * amountToScaleBy ) - radius;
    radius *= amountToScaleBy;
    centre.y += changeInRadius * 0.5;
}


bool Officer::RenderShape( double _predictionTime )
{    
    if( !m_enabled || m_dead ) return false;
    
    //
    // Calculate where we are

    Vector3 predictedPos = m_pos + m_vel * _predictionTime;
    if( m_onGround && m_inWater==-1 )
    {
        predictedPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( predictedPos.x, predictedPos.z );
    }
    Vector3 entityUp = g_upVector;
    Vector3 entityRight(m_front ^ entityUp);
        

    //
    // Scale based on distance

    double amountToScaleBy = 1.0f;
    double scaleFactor = g_prefsManager->GetFloat( "DarwinianScaleFactor" );
    if( scaleFactor > 0.0f )
    {
        double camDist = ( g_app->m_camera->GetPos() - m_pos ).Mag();        
        if( camDist > 100 ) amountToScaleBy *= 1.0 + scaleFactor * ( camDist - 100 ) / 1000;
         
        if( camDist > 300 )
        {
            double factor = ( camDist - 300 ) / 1000;
            if( factor < 0.0 ) factor = 0.0;
            if( factor > 1.0 ) factor = 1.0;

            entityUp = ( entityUp * (1-factor) ) + ( g_app->m_camera->GetUp() * factor );                                                       
            if( m_front * g_app->m_camera->GetFront() < 0 )
            {
                entityRight = ( entityRight * (1-factor) ) + ( g_app->m_camera->GetRight() * factor );
            }
            else
            {
                entityRight = ( entityRight * (1-factor) ) + ( -g_app->m_camera->GetRight() * factor );
            }
        }
    }

    Vector3 entityFront = entityUp ^ entityRight;
    Matrix34 mat( entityFront, entityUp, predictedPos );

    mat.f *= amountToScaleBy;
    mat.u *= amountToScaleBy;
    mat.r *= amountToScaleBy;


    Vector3 flagPos = m_flagMarker->GetWorldMatrix(mat).pos;
    m_flag.SetPosition( flagPos );

    //
    // If we are damaged, flicked in and out based on our health

    if( m_renderDamaged )
    {
        double timeIndex = g_gameTime + m_id.GetUniqueId() * 10;
        double thefrand = frand();
        if      ( thefrand > 0.7 ) mat.f *= ( 1.0 - iv_sin(timeIndex) * 0.5 );
        else if ( thefrand > 0.4 ) mat.u *= ( 1.0 - iv_sin(timeIndex) * 0.2 );    
        else                        mat.r *= ( 1.0 - iv_sin(timeIndex) * 0.5 );
        glEnable( GL_BLEND );
        glBlendFunc( GL_ONE, GL_ONE );
    }
  

    //
    // Render our shape

    g_app->m_renderer->SetObjectLighting();
    glDisable       (GL_TEXTURE_2D);
    glShadeModel    (GL_SMOOTH);
    glEnable        (GL_NORMALIZE);

    m_shape->Render( _predictionTime, mat );

    glDisable       (GL_NORMALIZE);
    glShadeModel    (GL_FLAT);
//    glEnable        (GL_TEXTURE_2D);
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    g_app->m_renderer->UnsetObjectLighting();


    g_app->m_renderer->MarkUsedCells(m_shape, mat);
    
    return true;
}


int Officer::GetFormationIndex( int _uniqueId )
{
    int positionIndex = -1;

    //
    // Is this entity already in our formation?

    for( int i = 0; i < m_formationEntities.Size(); ++i )
    {
        if( m_formationEntities.ValidIndex(i) )
        {
            OfficerFormation *formation = m_formationEntities.GetPointer(i);
            if( formation->m_entityUniqueId == _uniqueId )
            {
                positionIndex = i;
                break;
            }
        }   
    }

    return positionIndex;
}



Vector3 Officer::GetFormationPosition( int _uniqueId )
{
    int positionIndex = GetFormationIndex( _uniqueId );

    //
    // If he is not in our formation, add him now

    if( positionIndex == -1 )
    {
        if( !FormationFull() )
        {
            OfficerFormation formation;
            formation.m_entityUniqueId = _uniqueId;
            formation.m_timer = 5.0;
            positionIndex = m_formationEntities.PutData( formation );
        }
        else
        {
            return Vector3();
        }
    }

    return GetFormationPositionFromIndex( m_pos, positionIndex, m_front, m_formationEntities.Size() );   
}


Vector3 Officer::GetFormationPositionFromIndex( Vector3 const &pos, int positionIndex, Vector3 const &front, int count )
{
    //
    // Work out his desired position, based on his formation index

    int rowSize = 6;    
    if( count > 24 ) rowSize = 9;
    if( count >= 48 ) rowSize = 12;
    if( count >= 90 ) rowSize = 15;
    if( count >= 200 ) rowSize = 25;

    int column = positionIndex % rowSize;
    int row = positionIndex / rowSize;

    double columnSpread = 6;
    double rowSpread = 6;

    Vector3 right = front ^ g_upVector;
    right.Normalise();

    Vector3 targetPos = pos + front * row * rowSpread
                        + right * column * columnSpread;

    targetPos += front * rowSpread * 2.0;
    targetPos -= right * rowSize * columnSpread * 0.5;

    targetPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( targetPos.x, targetPos.z );

    return targetPos;
}

bool Officer::FormationFull()
{
	if( g_app->IsSinglePlayer() ) return true;
    if( g_app->m_multiwiniaTutorial && g_app->m_multiwiniaTutorialType == App::MultiwiniaTutorial1 ) return true;

	int numGaps = 0;
    Team *team = g_app->m_location->m_teams[m_id.GetTeamId()];
    for( int i = 0; i < m_formationEntities.Size(); ++i )
    {
        if( !m_formationEntities.ValidIndex(i) )
        {
            numGaps++;
        }
    }
    return (m_formationEntities.Size() - numGaps >=  team->m_evolutions[Team::EvolutionFormationSize] );
}

void Officer::RegisterWithFormation( int _uniqueId )
{
    for( int i = 0; i < m_formationEntities.Size(); ++i )
    {
        if( m_formationEntities.ValidIndex(i) )
        {
            OfficerFormation *formation = m_formationEntities.GetPointer(i);
            if( formation->m_entityUniqueId == _uniqueId )
            {
                formation->m_timer += SERVER_ADVANCE_PERIOD;
                break;
            }
        }   
    }
}


bool Officer::IsInFormationMode()
{
    return( m_orders == OrderFollow && m_formation );
}


bool Officer::IsInFormation( int _uniqueId )
{
    for( int i = 0; i < m_formationEntities.Size(); ++i )
    {
        if( m_formationEntities.ValidIndex(i) )
        {
            if( m_formationEntities[i].m_entityUniqueId == _uniqueId ) return true;
        }
    }
    return false;
}


bool Officer::AdvanceIdle()
{
    if( m_orders != OrderNone )
    {
        m_state = StateGivingOrders;
        return false;
    }
    
    m_vel.Zero();
    
    return false;
}


bool Officer::AdvanceToWaypoint()
{
    bool arrived = false;

    if( IsInFormationMode() && !m_formationAngleSet )
    {
        arrived = AdvanceToTargetPositionInFormation();
    }
    else
    {
        arrived = AdvanceToTargetPosition();
    }

    if( m_formation ) m_orderPosition = m_pos + m_front * 100;

    if( arrived )
    {
        m_state = StateIdle;
    }

    return false;
}


bool Officer::AdvanceToTargetPositionInFormation()
{
    //
    // Turn to face the waypoint first

    double amountToTurn = SERVER_ADVANCE_PERIOD * 0.6;

    Vector3 targetDir = (m_wayPoint - m_pos);
    targetDir.HorizontalAndNormalise();

    m_front = m_front * (1.0 - amountToTurn) + targetDir * amountToTurn;
    m_front.HorizontalAndNormalise();
    
    m_vel.Zero();


    //
    // Once we are facing the target, move towards it

    double angleToTarget = ( targetDir ^ m_front ).Mag();

    if( angleToTarget < 0.1 )
    {
        m_formationAngleSet = true;
    }


    return false;
}


bool Officer::AdvanceToTargetPosition()
{
    //
    // Work out where we want to be next

    double threshold = 10.0;
    if( IsInFormationMode() ) threshold *= 0.3;

    double distance = (m_wayPoint - m_pos).Mag();
    
    if( distance > threshold )
    {    
        double speed = m_stats[StatSpeed];
        if( m_state == StateIdle ) speed *= 0.2;
        
        if( distance < 10.0f ) speed *= distance/10.0;

        double amountToTurn = SERVER_ADVANCE_PERIOD * 6.0;

        if( m_formation )
        {
            speed *= 0.4;
            amountToTurn *= 0.1;
        }
        
        Vector3 targetDir = (m_wayPoint- m_pos).Normalise();
        Vector3 actualDir = m_front * (1.0 - amountToTurn) + targetDir * amountToTurn;
        actualDir.Normalise();

        Vector3 oldPos = m_pos;
        Vector3 newPos = m_pos + actualDir * speed * SERVER_ADVANCE_PERIOD;


        //
        // Slow us down if we're going up hill
        // Speed up if going down hill
    
        double currentHeight = g_app->m_location->m_landscape.m_heightMap->GetValue( oldPos.x, oldPos.z );
        double nextHeight = g_app->m_location->m_landscape.m_heightMap->GetValue( newPos.x, newPos.z );
        double factor = 1.0 - (currentHeight - nextHeight) / -3.0;
        if( factor < 0.1 ) factor = 0.1;
        if( factor > 2.0 ) factor = 2.0;
        newPos = m_pos + actualDir * speed * factor * SERVER_ADVANCE_PERIOD;
        newPos = PushFromObstructions( newPos );
        
        m_pos = newPos;       
        m_vel = ( m_pos - oldPos ) / SERVER_ADVANCE_PERIOD;
        m_front = ( newPos - oldPos ).Normalise();
    }
    else
    {
        m_vel.Zero();
        return true;
    }    

    return false;    
}

bool Officer::AdvanceGivingOrders()
{
    if( m_orders == OrderGoto )
    {
        m_front = ( m_orderPosition - m_pos ).Normalise();
        if( m_orderRouteId != -1 )
        {
            Route *route = g_app->m_location->m_routingSystem.GetRoute(m_orderRouteId);
            if( route && route->m_wayPoints.Size() > 1 ) 
            {
                m_front = (route->m_wayPoints[1]->GetPos() - m_pos).Normalise();
            }
        }
    }

    if( m_orders == OrderPrepareGoto )
    {
        Team *team = g_app->m_location->m_teams[m_id.GetTeamId()];
        if( team->m_currentEntityId == m_id.GetIndex() )
        {
            // Look at the mouse
            //m_front = ( team->m_currentMousePos - m_pos ).Normalise();
        }
        else
        {
            m_orders = OrderNone;
        }
    }


    if( m_orders == OrderFollow )
    {
        if( m_orderPosition != g_zeroVector )
        {
            double amountToTurn = SERVER_ADVANCE_PERIOD * 3.0;
            if( m_formation ) amountToTurn *= 0.2;
        
            Vector3 targetDir = (m_orderPosition - m_pos).Normalise();
            Vector3 actualDir = m_front * (1.0 - amountToTurn) + targetDir * amountToTurn;
            actualDir.Normalise();       
            m_front = actualDir;

            double angleDiff = ( m_front * targetDir );
            if( angleDiff > 0.99 )
            {
                m_orderPosition = g_zeroVector;
            }
        }
    }
    
    m_vel.Zero();

    return false;
}


bool Officer::SearchForRandomPosition()
{
    double distance = 30.0;
    double angle = syncsfrand(2.0 * M_PI);

    m_wayPoint = m_pos + Vector3( iv_sin(angle) * distance,
                                   0.0,
                                   iv_cos(angle) * distance );

    m_wayPoint = PushFromObstructions( m_wayPoint );    
    m_wayPoint.y = g_app->m_location->m_landscape.m_heightMap->GetValue( m_wayPoint.x, m_wayPoint.z );    

    return true;
}


void Officer::Absorb()
{
    int numFound;
    g_app->m_location->m_entityGrid->GetFriends( s_neighbours, m_pos.x, m_pos.z, OFFICER_ABSORBRANGE, &numFound, m_id.GetTeamId() );

    WorldObjectId nearestId;
    double nearestDistance = 99999.9;

    for( int i = 0; i < numFound; ++i )
    {
        WorldObjectId id = s_neighbours[i];
        Entity *entity = g_app->m_location->GetEntity( id );
        if( entity && entity->m_type == Entity::TypeDarwinian && !entity->m_dead )
        {
            double distance = ( entity->m_pos - m_pos ).Mag();
            if( distance < nearestDistance )
            {
                nearestDistance = distance;
                nearestId = id;
            }
        }
    }

    if( nearestId.IsValid() )
    {
        m_absorbTimer -= SERVER_ADVANCE_PERIOD;
        if( m_absorbTimer < 0.0 ) 
        {
            Entity *entity = g_app->m_location->GetEntity( nearestId );

            entity->m_destroyed = true;
            ++m_shield;
            m_absorbTimer = 1.0;
        }
    }
}


bool Officer::Advance( Unit *_unit )
{
    if( !m_onGround ) AdvanceInAir(_unit);   
    bool amIDead = Entity::Advance(_unit);
    if( m_inWater != -1.0 ) AdvanceInWater(_unit);

    if( m_onGround && !m_dead ) m_pos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( m_pos.x, m_pos.z );    


    if( g_app->Multiplayer() && m_orders == OrderNone )
    {
        m_orders = OrderPrepareGoto;
    }

    
    //
    // Advance in whatever state we are in

    if( !amIDead && m_onGround && m_inWater == -1.0 )
    {
        switch( m_state )
        {
            case StateIdle :                amIDead = AdvanceIdle();            break;
            case StateToWaypoint :          amIDead = AdvanceToWaypoint();      break;
            case StateGivingOrders :        amIDead = AdvanceGivingOrders();    break;
        }
    }

    if( m_dead ) 
    {
        m_vel.y -= 20.0;
        m_pos.y += m_vel.y * SERVER_ADVANCE_PERIOD;
    }
    

    //
    // If we are giving orders, render them

    if( m_orders == OrderGoto )
    {
        bool triggerOrder = false;

        if( g_app->IsSinglePlayer() )
        {
            if( syncfrand(1) < 0.05 ) triggerOrder = true;
        }
        else
        {
            double timeSinceLastOrder = GetNetworkTime() - m_lastOrderCreated;
            if( timeSinceLastOrder > 2.0 ) triggerOrder = true;
        }
                

        if( triggerOrder )
        {
            if( g_app->IsSinglePlayer() )
            {
                OfficerOrders *orders = new OfficerOrders();
                orders->m_pos = m_pos + Vector3(0,2,0);
                orders->m_wayPoint = m_orderPosition;
                int index = g_app->m_location->m_effects.PutData( orders );
                orders->m_id.Set( m_id.GetTeamId(), UNIT_EFFECTS, index, -1 );
                orders->m_id.GenerateUniqueId();
            }
            else
            {
                MultiwiniaOfficerOrders *orders = new MultiwiniaOfficerOrders();
                orders->m_pos = m_pos + Vector3(0,2,0);
                orders->m_wayPoint = m_orderPosition;
                orders->m_routeId = m_orderRouteId;
                int index = g_app->m_location->m_effects.PutData( orders );
                orders->m_id.Set( m_id.GetTeamId(), UNIT_EFFECTS, index, -1 );
                orders->m_id.GenerateUniqueId();
            }

            m_lastOrderCreated = GetNetworkTime();
        }
    }


    //
    // If we are absorbing, look around for Darwinians

    if( m_absorb ) Absorb();


    //
    // Advance our formation
    
    if( m_formation )
    {
        int gapSize = 0;
        bool inGap = false;
        for( int i = 0; i < m_formationEntities.Size(); ++i )
        {
            if( m_formationEntities.ValidIndex(i) )
            {
                inGap = false;
                OfficerFormation *formation = m_formationEntities.GetPointer(i);
                formation->m_timer -= SERVER_ADVANCE_PERIOD;
                if( formation->m_timer <= 0.0 )
                {
                    m_formationEntities.RemoveData( i );
                }
            }
            else
            {
                if( inGap ) ++gapSize;
                inGap = true;
            }
        }

        if( gapSize > m_formationEntities.Size() * 0.5 )
        {
            m_formationEntities.Empty();
        }
    }


    //
    // Attack anything nearby with our "shield"
    
    if( m_shield > 0 )
    {
        WorldObjectId id = g_app->m_location->m_entityGrid->GetBestEnemy( m_pos.x, m_pos.z, 0.0, OFFICER_ATTACKRANGE, m_id.GetTeamId() );
        if( id.IsValid() )
        {
            Entity *entity = g_app->m_location->GetEntity( id );
            entity->ChangeHealth( -10 );
            m_shield --;

            Vector3 themToUs = m_pos - entity->m_pos;
            g_app->m_location->SpawnSpirit( m_pos, themToUs, 0, WorldObjectId() );
        }
    }


    //
    // Use teleports.  Remember which teleport we entered,
    // As there may be people following us

    if( m_wayPointTeleportId != -1 )
    {
        int teleportId = EnterTeleports(m_wayPointTeleportId);
        if( teleportId != -1 )
        {
            m_ordersBuildingId = teleportId;
            Teleport *teleport = (Teleport *) g_app->m_location->GetBuilding( teleportId );
            Vector3 exitPos, exitFront;
            bool exitFound = teleport->GetExit( exitPos, exitFront );
            if( exitFound ) m_wayPoint = exitPos + exitFront * 30.0;
			if( m_orders == OrderGoto ) m_orders = OrderNone;
            m_wayPointTeleportId = -1;
        }
    }
    

    if( m_targetFront != g_zeroVector &&
        m_state != StateToWaypoint )
    {
        double amountToTurn = SERVER_ADVANCE_PERIOD * 3.0 * 0.2;
        Vector3 targetDir = m_targetFront;
        Vector3 actualDir = m_front * (1.0 - amountToTurn) + targetDir * amountToTurn;
        actualDir.Normalise();
        m_front = actualDir;
        if( (targetDir ^ actualDir).Mag() < 0.1 )
        {
            m_turnToTarget = false;
        }
    }


    //
    // If we are maintaining an Order Route then update it
    // If we have moved

    if( m_orderRouteId != -1 && 
        m_orders == OrderGoto )
    {
        Route *route = g_app->m_location->m_routingSystem.GetRoute(m_orderRouteId);
        if( route && route->m_wayPoints.Size() )
        {
            Vector3 firstPos = route->m_wayPoints[0]->GetPos();
            double distance = ( firstPos - m_pos ).Mag();

            if( distance > 50 )
            {
                Vector3 routeTarget = route->GetEndPoint();
                m_orderRouteId = g_app->m_location->m_routingSystem.GenerateRoute( m_pos, routeTarget );
                if( m_id.GetTeamId() == g_app->m_globalWorld->m_myTeamId )
                {
                    g_app->m_location->m_routingSystem.HighlightRoute( m_orderRouteId, m_id.GetTeamId() );
                }
            }
        }
    }


    return amIDead || m_demoted;
}

void Officer::RunAI( AI *_ai )
{
    m_aiTimer -= SERVER_ADVANCE_PERIOD;
    if( m_aiTimer > 0.0 ) return;
    m_aiTimer = 1.0;

    if( m_orders == OrderGoto )
    {
        m_aiTimer = 10.0;
        int currentTarget = _ai->FindNearestTarget( m_pos );
        AITarget *thisTarget = (AITarget *)g_app->m_location->GetBuilding( currentTarget );
        if( thisTarget )
        {
            int targetBuildingId = _ai->FindTargetBuilding( thisTarget->m_id.GetUniqueId(), m_id.GetTeamId() );

            AITarget *targetBuilding = (AITarget *)g_app->m_location->GetBuilding( targetBuildingId );

            if( targetBuilding )
            {
                Vector3 targetPos = targetBuilding->m_pos;

                if( (m_orderPosition - targetPos).Mag() > 60.0f )
                {
                    double positionError = 20.0;
                    double radius = 20.0+syncfrand(positionError);
                    double theta = syncfrand(M_PI * 2);                
                    targetPos.x += radius * iv_sin(theta);
                    targetPos.z += radius * iv_cos(theta);
                    targetPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue(targetPos.x, targetPos.z);

                    if( targetBuilding->m_radarTarget )
		            {
			            LList<int> *nearbyBuildings = g_app->m_location->m_obstructionGrid->GetBuildings( targetBuilding->m_pos.x, targetBuilding->m_pos.z );
			            for( int i = 0; i < nearbyBuildings->Size(); ++i )
			            {
				            int buildingId = nearbyBuildings->GetData(i);
				            Building *building = g_app->m_location->GetBuilding( buildingId );
				            if( building->m_type == Building::TypeRadarDish  )
				            {
                                Vector3 entrancePos, entranceFront;
                                ((Teleport *)building)->GetEntrance( entrancePos, entranceFront );

					            targetPos = entrancePos;
				            }
			            }
			            //darwinian->GiveOrders( pos );
		            }

                    SetOrders( targetPos );
                }
            }

            return;
        }
    }

    if( !m_formation )
    {
        SetFormation( m_pos + m_front * 50 );
    }

    // check our immediate area for threats first - we dont want to be walking right by a huge group of darwinians at the wrong angle
    int numFound;
    g_app->m_location->m_entityGrid->GetEnemies( s_neighbours, m_pos.x, m_pos.z, DARWINIAN_SEARCHRANGE_THREATS * 3.0, &numFound, m_id.GetTeamId() );

    if( numFound > 10 )
    {
        Vector3 pos;
        for( int i = 0; i < numFound; ++i )
        {
            Entity *e = g_app->m_location->GetEntity( s_neighbours[i] );
            if( e )
            {
                pos += e->m_pos;
            }
        }
        pos /= numFound;
        SetWaypoint( m_pos );
        m_targetFront = (pos - m_pos).Normalise();
        m_turnToTarget = true;
        return;
    }

    if( m_state == Officer::StateToWaypoint) return;

    AITarget *t = (AITarget *)g_app->m_location->GetBuilding( _ai->FindNearestTarget( m_pos ) );
    if( t )
    {
        bool moveOn = true;
        if( t->IsImportant() )
        {
            if( (m_pos - t->m_pos).Mag() > 20.0 )
            {
                SetWaypoint( t->m_pos );
            }

            Vector3 targetDir = g_zeroVector;
            if( t->NeedsDefending(m_id.GetTeamId()) )
            {
                moveOn = false;
                for( int j = 0; j < t->m_neighbours.Size(); ++ j )
                {
                    AITarget *thisTarget = (AITarget *)g_app->m_location->GetBuilding( t->m_neighbours[j]->m_neighbourId );
                    if( thisTarget &&
                        thisTarget->m_id.GetTeamId() != m_id.GetTeamId() &&
                        thisTarget->m_id.GetTeamId() != 255 )
                    {
                        targetDir = ( t->m_pos - thisTarget->m_pos ).Normalise();
                    }
                }
            }
            else
            {
                // we dont need to defend this, so just pick a nice position to face while we wait to fill up
                for( int j = 0; j < t->m_neighbours.Size(); ++ j )
                {
                    AITarget *thisTarget = (AITarget *)g_app->m_location->GetBuilding( t->m_neighbours[j]->m_neighbourId );
                    if( thisTarget )
                    {
                        targetDir = (  thisTarget->m_pos - t->m_pos ).Normalise();
                        break;
                    }
                }
            }

            if( targetDir != g_zeroVector )
            {
                m_targetFront = targetDir;
                m_turnToTarget = true;
            }
        }
        
        if( moveOn && FormationFull() ) // no sense going anywhere else with a half full formation
        {
            // we dont need to hang around here, go attack someone
            double nearest = FLT_MAX;
            int id = -1;

            for( int j = 0; j < g_app->m_location->m_buildings.Size(); ++ j )
            {
                if( g_app->m_location->m_buildings.ValidIndex(j) )
                {
                    AITarget *thisTarget = (AITarget *)g_app->m_location->m_buildings[j];
                    if( !thisTarget || thisTarget->m_type != Building::TypeAITarget ) continue;
                    if( !g_app->m_location->IsWalkable( m_pos, thisTarget->m_pos, true  ) ) continue;

                    bool valid = false;
                    if( thisTarget->m_id.GetTeamId() != m_id.GetTeamId() &&
                        thisTarget->m_id.GetTeamId() != 255 )
                    {
                        // owned by the enemy
                        valid = true;
                    }

                    if( thisTarget->IsImportant() &&
                        thisTarget->m_id.GetTeamId() == 255 )
                    {
                        // valuable, uncaptured building (spawn point, multiwinia zone etc)
                        valid = true;
                    }

                    if( valid &&
                        ( thisTarget->m_pos - m_pos).Mag() < nearest )
                    {
                        nearest = ( thisTarget->m_pos - m_pos).Mag();
                        id = j;
                    }
                }
            }

            if( id != -1 )
            {
                Building *building = g_app->m_location->GetBuilding( id );
                if( building ) SetWaypoint( building->m_pos );
            }
        }
    }
}


void Officer::SetWaypoint( Vector3 const &_wayPoint )
{
	m_wayPoint = _wayPoint;
	m_state = StateToWaypoint; 

    if( IsInFormationMode() )
    {
        m_formationAngleSet = false;

        if( ( _wayPoint - m_pos ).Mag() < 0.1f )
        {
            // We just set a waypoint on ourselves
            // So dont try to rotate
            m_formationAngleSet = true;
        }
    }

	//
	// If we clicked near a teleport, tell the officer to go into it

    m_wayPointTeleportId = -1;
	LList<int> *nearbyBuildings = g_app->m_location->m_obstructionGrid->GetBuildings( _wayPoint.x, _wayPoint.z );
	for( int i = 0; i < nearbyBuildings->Size(); ++i )
	{
		int buildingId = nearbyBuildings->GetData(i);
		Building *building = g_app->m_location->GetBuilding( buildingId );
		if( building->m_type == Building::TypeRadarDish ||
			building->m_type == Building::TypeBridge )
		{
			double distance = ( building->m_pos - _wayPoint ).Mag();
			Teleport *teleport = (Teleport *) building;
			if( distance < 5.0 && teleport->Connected() )
			{
				m_wayPointTeleportId = building->m_id.GetUniqueId();
				Vector3 entrancePos, entranceFront;
				teleport->GetEntrance( entrancePos, entranceFront );
				m_wayPoint = entrancePos;
				break;
			}
		}
	}
}


void Officer::ListSoundEvents( LList<char *> *_list )
{
    Entity::ListSoundEvents( _list );

    _list->PutData( "SetOrderNone" );
    _list->PutData( "SetOrderGoto" );
    _list->PutData( "SetOrderFollow" );
    _list->PutData( "SetOrderAbsorb" );
    _list->PutData( "SetOrderFormation" );
}


void Officer::CancelOrderSounds()
{
    g_app->m_soundSystem->StopAllSounds( m_id, "Officer SetOrderNone" );
    g_app->m_soundSystem->StopAllSounds( m_id, "Officer SetOrderGoto" );
    g_app->m_soundSystem->StopAllSounds( m_id, "Officer SetOrderFollow" );
    g_app->m_soundSystem->StopAllSounds( m_id, "Officer SetOrderAbsorb" );
    g_app->m_soundSystem->StopAllSounds( m_id, "Officer SetOrderFormation" );
}


bool Officer::IsThereATeleportClose(Vector3 const &_orders)
{
	bool foundTeleport = false;

	LList<int> *nearbyBuildings = g_app->m_location->m_obstructionGrid->GetBuildings( _orders.x, _orders.z );
	for( int i = 0; i < nearbyBuildings->Size(); ++i )
	{
		int buildingId = nearbyBuildings->GetData(i);
		Building *building = g_app->m_location->GetBuilding( buildingId );
		if( building->m_type == Building::TypeRadarDish )
		{
			Teleport *teleport = (Teleport *) building;
			Vector3 entrancePos, entranceFront;
			teleport->GetEntrance( entrancePos, entranceFront );

			double distance = ( entrancePos - _orders ).MagSquared();                    
			if( distance < 5.0 )
			{
				foundTeleport = true;
				break;
			}
		}
	}

	return foundTeleport;
}


void Officer::SetOrders( Vector3 const &_orders, bool directRoute )
{    
    if( g_app->IsSinglePlayer() &&
        !g_app->m_location->IsWalkable( m_pos, _orders ) )
    {
#ifdef USE_SEPULVEDA_HELP_TUTORIAL
        g_app->m_sepulveda->Say( "officer_notwalkable" );
#endif
    }
    else
	{
        bool formationToggle = IsFormationToggle(_orders);
        
        if( IsInFormationMode() ) 
        {
            // Already in formation mode, so move here
            Vector3 targetPos = _orders;
            if( formationToggle )
            {
                targetPos = m_pos + (_orders-m_pos).Normalise();
            }

            SetWaypoint( targetPos);
            SetFormation( targetPos );
            g_app->m_location->m_teams[ m_id.GetTeamId() ]->SelectUnit(-1,-1,-1);
        }
        else if( formationToggle )
        {
            // Going in to formation mode
            if( g_app->Multiplayer() )
            {
                Vector3 targetPos = m_pos + (_orders-m_pos).Normalise();
                SetWaypoint( targetPos );
                SetFormation( targetPos );
                //g_app->m_location->m_teams[ m_id.GetTeamId() ]->SelectUnit(-1,-1,-1);
            }
            else
            {
                double timeNow = GetNetworkTime();
                int researchLevel = g_app->m_globalWorld->m_research->CurrentLevel( GlobalResearch::TypeOfficer );

                if( timeNow > m_lastOrdersSet + 0.3f )
                {
                    m_lastOrdersSet = timeNow;

                    if( researchLevel > 2 )
                    {
                        m_orders = OrderFollow;
                        CancelOrderSounds();
                        g_app->m_soundSystem->TriggerEntityEvent( this, "SetOrderFollow" );
                    }

                    m_buildingId = -1;
                }
            }
        }     
        else if( !formationToggle )
        {
            m_orderPosition = _orders;        
            m_orders = OrderGoto;
            m_absorb = false;
            m_formation = false;

            m_orderRouteId = g_app->m_location->m_routingSystem.GenerateRoute( m_pos, (Vector3 &)_orders, directRoute );
            if( m_id.GetTeamId() == g_app->m_globalWorld->m_myTeamId )
            {
                //g_app->m_location->m_routingSystem.SetRouteIntensity( m_orderRouteId, 1.0 );
                g_app->m_location->m_routingSystem.HighlightRoute( m_orderRouteId, m_id.GetTeamId() );
            }


            //
            // If there is a teleport nearby, 
            // assume he wants us to go in it

            bool foundTeleport = false;

            m_ordersBuildingId = -1;
            LList<int> *nearbyBuildings = g_app->m_location->m_obstructionGrid->GetBuildings( m_orderPosition.x, m_orderPosition.z );
            for( int i = 0; i < nearbyBuildings->Size(); ++i )
            {
                int buildingId = nearbyBuildings->GetData(i);
                Building *building = g_app->m_location->GetBuilding( buildingId );
                if( building->m_type == Building::TypeRadarDish )
                {
                    Teleport *teleport = (Teleport *) building;
                    Vector3 entrancePos, entranceFront;
                    teleport->GetEntrance( entrancePos, entranceFront );

                    double distance = ( entrancePos - _orders ).MagSquared();                    
                    if( distance < 5.0 )
                    {
                        m_ordersBuildingId = building->m_id.GetUniqueId();
                        m_orderPosition = entrancePos;
                        foundTeleport = true;
                        break;
                    }
                }
            }

            if( !foundTeleport )
            {
                m_orderPosition = PushFromObstructions( m_orderPosition );
            }
        

            //
            // Create the line using particles immediately, 
            // so the player can see what he's just done

            if( m_id.GetTeamId() == g_app->m_globalWorld->m_myTeamId )
            {
                double timeNow = GetNetworkTime();
                if( timeNow > m_lastOrdersSet + 1.0 )
                {
                    m_lastOrdersSet = timeNow;

                    if( g_app->IsSinglePlayer() )
                    {
                        OfficerOrders orders;
                        orders.m_pos = m_pos;
                        orders.m_wayPoint = m_orderPosition;

                        RGBAColour colour( 255, 100, 255, 255 );
                        if( g_app->Multiplayer() )
                        {
                            Team *team = g_app->m_location->m_teams[m_id.GetTeamId()];
                            colour = team->m_colour;            
                        }

                        int sanity = 0;
                        while( sanity < 1000 )
                        {   
                            sanity++;
                            if( orders.m_arrivedTimer < 0.0 )
                            {
                                g_app->m_particleSystem->CreateParticle( orders.m_pos, g_upVector*10, Particle::TypeMuzzleFlash, 75.0, colour );
                                g_app->m_particleSystem->CreateParticle( orders.m_pos, g_upVector*10, Particle::TypeMuzzleFlash, 60.0, colour );
                            }
                            if( orders.Advance() ) break;
                        }
                    }
                }
            }

            CancelOrderSounds();
            g_app->m_soundSystem->TriggerEntityEvent( this, "SetOrderGoto" );

            //
            // Deselect the officer

			if( !g_app->IsSinglePlayer() )
			{
				g_app->m_location->m_teams[ m_id.GetTeamId() ]->SelectUnit(-1,-1,-1);
			}
		}
    }
}


bool Officer::IsFormationToggle( Vector3 const &mousePos )
{
    if( g_app->m_multiwiniaTutorial && g_app->m_multiwiniaTutorialType == App::MultiwiniaTutorial1 ) return false; 
    if( m_noFormations ) return false;

	if(IsThereATeleportClose(mousePos))
	{
		return false;
	}

    double distance = ( mousePos - m_pos ).Mag();
    return( distance < 50.0f );
}


void Officer::SetFormation( Vector3 const &targetPos )
{
	// No formation in tutorial!
    if( g_app->m_multiwiniaTutorial && g_app->m_multiwiniaTutorialType == App::MultiwiniaTutorial1 ) return;
    //
    // Set the order

    if( m_formation ) 
    {
        m_orderPosition = targetPos;
    }
    else 
    {
        m_orderPosition = g_zeroVector;
        m_front = ( targetPos  - m_pos ).Normalise();                
    }

    m_orders = OrderFollow;
    m_formation = true;
    m_formationAngleSet = false;


    //
    // Visual effects

    if( m_id.GetTeamId() == g_app->m_globalWorld->m_myTeamId )
    {
        Team *myTeam = g_app->m_location->GetMyTeam();

        int routeId = g_app->m_location->m_routingSystem.GenerateRoute( m_pos, targetPos, true );
        g_app->m_location->m_routingSystem.HighlightRoute( routeId, m_id.GetTeamId() );

        Vector3 front = ( targetPos - m_pos ).Normalise();
        
        int formationSize = myTeam->m_evolutions[Team::EvolutionFormationSize];

        for( int i = 0; i < formationSize; ++i )
        {
            Vector3 up = g_upVector;
            Vector3 formationPos = GetFormationPositionFromIndex( targetPos, i, front, formationSize);
            formationPos += g_upVector * 2;

            g_app->m_particleSystem->CreateParticle( formationPos, g_zeroVector, Particle::TypeMuzzleFlash, 40.0, myTeam->m_colour );            
            g_app->m_particleSystem->CreateParticle( formationPos, g_zeroVector, Particle::TypeMuzzleFlash, 40.0, myTeam->m_colour );            
        }
    }

    
    //
    // Sound effects

    CancelOrderSounds();
    g_app->m_soundSystem->TriggerEntityEvent( this, "SetOrderFormation" );
}

int Officer::GetNextMode()
{
	int orders = -1;
	int researchLevel = g_app->m_globalWorld->m_research->CurrentLevel( GlobalResearch::TypeOfficer );


	switch( researchLevel )
	{
		case 0 :
		case 1 :
		case 2 :
			AppDebugOut("Research mode 2\n");
			orders = OrderNone;
			break;

		case 3 :
			AppDebugOut("Research mode 3\n");
			/*if      ( m_orders == OrderNone )                   m_orders = OrderPrepareGoto;
			else*/ if ( m_orders == OrderPrepareGoto )            orders = OrderFollow;
			else if ( g_app->IsSinglePlayer() )
			{
				if ( m_orders == OrderFollow )					orders = OrderNone;
				else if( m_orders == OrderNone ) 
				{
					orders = OrderGoto;
				}
				else if ( m_orders == OrderGoto )                   orders = OrderFollow;
			}
			else 
			{
				if( m_orders == OrderNone ) orders = OrderPrepareGoto;
				else if ( m_orders == OrderFollow && m_formation )
				{
					orders = OrderNone;
				}
				else if ( m_orders == OrderGoto )                   orders = OrderNone;					
			}
			break;

		case 4 :
			AppDebugOut("Research mode 4\n");
			if      ( m_orders == OrderNone )                   orders = OrderFollow;
			else if ( m_orders == OrderFollow && !m_absorb ) 
			{
				m_absorb = true;
				m_absorbTimer = 2.0f;
			}
			else if ( m_orders == OrderFollow && m_absorb ) 
			{
																m_orders = OrderNone;
																m_absorb = false;
			}
			else if ( m_orders == OrderGoto )                   m_orders = OrderNone;
			break;
	}
	return orders;
}

void Officer::SetNextMode()
{
	AppDebugOut("Setting next mode\n");
    if( g_app->Multiplayer() ) return;

    static double lastOrderSet = 0.0;

    double timeNow = GetNetworkTime();
    int researchLevel = g_app->m_globalWorld->m_research->CurrentLevel( GlobalResearch::TypeOfficer );

    if( timeNow > lastOrderSet + 0.3 )
    {
        lastOrderSet = timeNow;

        switch( researchLevel )
        {
            case 0 :
            case 1 :
            case 2 :
				AppDebugOut("Research mode 2\n");
                m_orders = OrderNone;
                break;

            case 3 :
				AppDebugOut("Research mode 3\n");
                /*if      ( m_orders == OrderNone )                   m_orders = OrderPrepareGoto;
                else*/ if ( m_orders == OrderPrepareGoto )            m_orders = OrderFollow;
				else if ( g_app->IsSinglePlayer() )
				{
					if ( m_orders == OrderFollow )					m_orders = OrderNone;
					else if( m_orders == OrderNone ) 
					{
						m_orderPosition = g_app->m_userInput->GetMousePos3d();
						m_orders = OrderGoto;
					}
					else if ( m_orders == OrderGoto )                   m_orders = OrderFollow;
				}
				else 
				{
					if( m_orders == OrderNone ) m_orders = OrderPrepareGoto;
					else if ( m_orders == OrderFollow && !m_formation ) m_formation = true;
					else if ( m_orders == OrderFollow && m_formation )
					{
						m_orders = OrderNone;
						m_formation = false;
						m_formationEntities.Empty();
					}
					else if ( m_orders == OrderGoto )                   m_orders = OrderNone;					
				}
                break;

            case 4 :
				AppDebugOut("Research mode 4\n");
                if      ( m_orders == OrderNone )                   m_orders = OrderFollow;
                else if ( m_orders == OrderFollow && !m_absorb ) 
                {
                    m_absorb = true;
                    m_absorbTimer = 2.0;
                }
                else if ( m_orders == OrderFollow && m_absorb ) 
                {
                                                                    m_orders = OrderNone;
                                                                    m_absorb = false;
                }
                else if ( m_orders == OrderGoto )                   m_orders = OrderNone;
                break;
        }
       
        CancelOrderSounds();

        switch( m_orders )
        {
            case OrderNone:     g_app->m_soundSystem->TriggerEntityEvent( this, "SetOrderNone" );       break;
            case OrderGoto:     g_app->m_soundSystem->TriggerEntityEvent( this, "SetOrderGoto" );       break;
            case OrderFollow:   
                if( m_absorb )  g_app->m_soundSystem->TriggerEntityEvent( this, "SetOrderAbsorb" );     
                else            g_app->m_soundSystem->TriggerEntityEvent( this, "SetOrderFollow" );     
                break;
        }

        m_buildingId = -1;
    }
}

int Officer::GetPreviousMode()
{
	int orders = -1;
	int researchLevel = g_app->m_globalWorld->m_research->CurrentLevel( GlobalResearch::TypeOfficer );


	switch( researchLevel )
        {
            case 0 :
            case 1 :
            case 2 :
				AppDebugOut("Research mode 2\n");
                orders = OrderNone;
                break;

            case 3 :
				AppDebugOut("Research mode 3\n");
                /*if      ( m_orders == OrderNone )                   m_orders = OrderPrepareGoto;
                else */if ( m_orders == OrderPrepareGoto )            orders = OrderFollow;
                else if ( m_orders == OrderGoto )                   orders = OrderNone;
                else if ( g_app->IsSinglePlayer() )
				{
					if      ( m_orders == OrderNone )
					{
						orders = OrderFollow;
					}
					else if ( m_orders == OrderFollow )	
					{
						orders = OrderGoto;
					}
				}
				else 
				{
					if      ( m_orders == OrderNone )                   orders = OrderPrepareGoto;
					else if ( m_orders == OrderFollow && m_formation )
					{
						orders = OrderNone;
					}
				}
                break;

            case 4 :
				AppDebugOut("Research mode 4\n");
                if      ( m_orders == OrderNone )                   
                {
                    orders = OrderFollow;
                }
                else if ( m_orders == OrderFollow && !m_absorb )    orders = OrderNone;
                else if ( m_orders == OrderFollow && m_absorb ) 
                {
                                                                    orders = OrderFollow;
                }
                else if ( m_orders == OrderGoto )                   orders = OrderNone;
                break;
        }
	return orders;
}

void Officer::CancelOrders()
{
	m_orders = OrderNone;
    m_formation = false;
    m_wayPoint = m_pos;

	CancelOrderSounds();
}

void Officer::SetFollowMode()
{
	int researchLevel = g_app->m_globalWorld->m_research->CurrentLevel( GlobalResearch::TypeOfficer );
	if( researchLevel > 2 )
	{
		m_orders = OrderFollow;
		CancelOrderSounds();
	}
}

void Officer::SetPreviousMode()
{
	AppDebugOut("Setting previous mode\n");
    if( g_app->Multiplayer() ) return;

    static double lastOrderSet = 0.0;

    double timeNow = GetNetworkTime();
    int researchLevel = g_app->m_globalWorld->m_research->CurrentLevel( GlobalResearch::TypeOfficer );

    if( timeNow > lastOrderSet + 0.3 )
    {
        lastOrderSet = timeNow;

        switch( researchLevel )
        {
            case 0 :
            case 1 :
            case 2 :
				AppDebugOut("Research mode 2\n");
                m_orders = OrderNone;
                break;

            case 3 :
				AppDebugOut("Research mode 3\n");
                /*if      ( m_orders == OrderNone )                   m_orders = OrderPrepareGoto;
                else */if ( m_orders == OrderPrepareGoto )            m_orders = OrderFollow;
                else if ( m_orders == OrderGoto )                   m_orders = OrderNone;
                else if ( g_app->IsSinglePlayer() )
				{
					if      ( m_orders == OrderNone )
					{
						m_orders = OrderFollow;
					}
					else if ( m_orders == OrderFollow )	
					{
						m_orderPosition = g_app->m_userInput->GetMousePos3d();
						m_orders = OrderGoto;
					}
				}
				else 
				{
					if      ( m_orders == OrderNone )                   m_orders = OrderPrepareGoto;
					else if ( m_orders == OrderFollow && !m_formation ) m_formation = true;
					else if ( m_orders == OrderFollow && m_formation )
					{
						m_orders = OrderNone;
						m_formation = false;
						m_formationEntities.Empty();
					}
				}
                break;

            case 4 :
				AppDebugOut("Research mode 4\n");
                if      ( m_orders == OrderNone )                   
                {
                    m_orders = OrderFollow;
                    m_absorb = true;
                }
                else if ( m_orders == OrderFollow && !m_absorb )    m_orders = OrderNone;
                else if ( m_orders == OrderFollow && m_absorb ) 
                {
                                                                    m_orders = OrderFollow;
                                                                    m_absorb = false;
                }
                else if ( m_orders == OrderGoto )                   m_orders = OrderNone;
                break;
        }
       
        CancelOrderSounds();

        switch( m_orders )
        {
            case OrderNone:     g_app->m_soundSystem->TriggerEntityEvent( this, "SetOrderNone" );       break;
            case OrderGoto:     g_app->m_soundSystem->TriggerEntityEvent( this, "SetOrderGoto" );       break;
            case OrderFollow:   
                if( m_absorb )  g_app->m_soundSystem->TriggerEntityEvent( this, "SetOrderAbsorb" );     
                else            g_app->m_soundSystem->TriggerEntityEvent( this, "SetOrderFollow" );     
                break;
        }

        m_buildingId = -1;
    }
}

bool Officer::IsSelectable()
{
    return true;
}

char *Officer::GetOrderType( int _orderType )
{
    static char *orders[] = {   "None",
                                "PrepareGoto",
                                "Goto",
                                "Follow"
                            };

    return orders[ _orderType ];
}

char *HashDouble( double value, char *buffer );



char *Officer::LogState( char *_message )
{
    static char s_result[1024];

    static char buf1[32], buf2[32], buf3[32], buf4[32], buf5[32], buf6[32];

    sprintf( s_result, "%sENT OFFICER Id[%d] state[%d] pos[%s,%s,%s] vel[%s] front[%s] waypoint[%s]",
                        (_message)?_message:"",
                        m_id.GetUniqueId(),
                        m_state,
                        HashDouble( m_pos.x, buf1 ),
                        HashDouble( m_pos.y, buf2 ),
                        HashDouble( m_pos.z, buf3 ),
                        HashDouble( m_vel.x + m_vel.y + m_vel.z, buf4 ),
                        HashDouble( m_front.x + m_front.y + m_front.z, buf5 ),
                        HashDouble( m_wayPoint.x + m_wayPoint.y + m_wayPoint.z, buf6 ) );

    return s_result;

}


// ============================================================================

OfficerOrders::OfficerOrders()
:   WorldObject(),
    m_arrivedTimer(-1.0)
{
    m_type = EffectOfficerOrders;
}


bool OfficerOrders::Advance()
{
    if( m_arrivedTimer >= 0.0 )
    {
        // We are already here, so just fade out
        m_vel.Zero();
        m_arrivedTimer += SERVER_ADVANCE_PERIOD;        
        if( m_arrivedTimer > 1.0 ) return true;        
    }
    else
    {
        double speed = m_vel.Mag();
        speed *= 1.1;
        speed = max( speed, 30.0 );
        speed = min( speed, 150.0 );

        Vector3 toWaypoint = ( m_wayPoint - m_pos );
        toWaypoint.y = 0.0;
        double distance = toWaypoint.Mag();

        toWaypoint.Normalise();
        m_vel = toWaypoint * speed;
        Vector3 oldPos = m_pos;
        m_pos += m_vel * SERVER_ADVANCE_PERIOD;

        double landHeight = g_app->m_location->m_landscape.m_heightMap->GetValue( m_pos.x, m_pos.z );
        m_pos.y = landHeight + 2.0;

        m_vel = ( m_pos - oldPos ) / SERVER_ADVANCE_PERIOD;

        if( g_app->m_globalWorld->m_myTeamId == m_id.GetTeamId() )
        {
			Team *team = g_app->m_location->m_teams[m_id.GetTeamId()];
			RGBAColour colour = team->m_colour;

            g_app->m_particleSystem->CreateParticle( oldPos, g_zeroVector, Particle::TypeMuzzleFlash, 30.0, colour );
        }

        if( m_vel.Mag() * SERVER_ADVANCE_PERIOD > distance ) m_arrivedTimer = 0.0;
        if( distance < 10.0 ) m_arrivedTimer = 0.0;
    }
    
    return false;
}


void OfficerOrders::Render( double _time )
{
    if( g_app->m_globalWorld->m_myTeamId != m_id.GetTeamId() ) return;

    double size = 15.0;
    Vector3 predictedPos = m_pos + m_vel * _time;

    double alpha = 0.7;
    if( m_arrivedTimer >= 0.0 )
    {
        double fraction = 1.0 - ( m_arrivedTimer + _time );
        fraction = max( fraction, 0.0 );
        fraction = min( fraction, 1.0 );
        //alpha = 0.7 * fraction;
        size *= fraction;
    }

    Vector3 camUp = g_app->m_camera->GetUp() * size;
    Vector3 camRight = g_app->m_camera->GetRight() * size;

	if( g_app->Multiplayer() )
	{
		Team *team = g_app->m_location->m_teams[m_id.GetTeamId()];
		RGBAColour colour = team->m_colour;
		colour.a *= alpha;

		glColor4ubv( colour.GetData() );
	}
	else
	{
		glColor4f( 1.0, 0.3, 1.0, alpha );
	}

    glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/starburst.bmp" ) );
    glDisable       ( GL_DEPTH_TEST );

    glBegin( GL_QUADS );
        glTexCoord2i( 0, 0 );       glVertex3dv( (predictedPos - camRight + camUp).GetData() );
        glTexCoord2i( 1, 0 );       glVertex3dv( (predictedPos + camRight + camUp).GetData() );
        glTexCoord2i( 1, 1 );       glVertex3dv( (predictedPos + camRight - camUp).GetData() );
        glTexCoord2i( 0, 1 );       glVertex3dv( (predictedPos - camRight - camUp).GetData() );
    glEnd();

    glEnable        ( GL_DEPTH_TEST );
    glDisable       ( GL_TEXTURE_2D );
}


// ============================================================================

MultiwiniaOfficerOrders::MultiwiniaOfficerOrders()
:   WorldObject(),
    m_arrivedTimer(-1.0),
    m_routeId(-1),
    m_routeWayPointId(-1),
    m_dropArrowTimer(0.0)
{
    m_type = EffectOfficerOrders;
}


bool MultiwiniaOfficerOrders::Advance()
{
    if( m_arrivedTimer >= 0.0 )
    {
        // We are already here, so just fade out
        m_vel.Zero();
        m_arrivedTimer += SERVER_ADVANCE_PERIOD;        
        if( m_arrivedTimer > 1.0 ) return true;        
    }
    else
    {
        FollowRoute();
        if( m_routeId == -1 ) m_arrivedTimer = 0.0;

        double speed = m_vel.Mag();
        speed *= (1.0 + SERVER_ADVANCE_PERIOD);
        speed = max( speed, 30.0 );
        speed = min( speed, 100.0 );

        Vector3 toWaypoint = ( m_wayPoint - m_pos );
        toWaypoint.y = 0.0;
        double distance = toWaypoint.Mag();

        toWaypoint.Normalise();
        m_vel = toWaypoint * speed;
        Vector3 oldPos = m_pos;
        m_pos += m_vel * SERVER_ADVANCE_PERIOD;

        double landHeight = g_app->m_location->m_landscape.m_heightMap->GetValue( m_pos.x, m_pos.z );
        m_pos.y = landHeight + 2.0;
        m_vel = ( m_pos - oldPos ) / SERVER_ADVANCE_PERIOD;

        double timeNow = g_gameTimer.GetGameTime();
        
        //if( g_app->m_globalWorld->m_myTeamId == m_id.GetTeamId() )
        if( timeNow >= m_dropArrowTimer )
        {
            m_dropArrowTimer = timeNow + 0.2;

            Team *team = g_app->m_location->m_teams[m_id.GetTeamId()];
            RGBAColour colour = team->m_colour;

            //g_app->m_particleSystem->CreateParticle( oldPos, g_zeroVector, Particle::TypeMuzzleFlash, 30.0, colour );

            Vector3 landUp = g_app->m_location->m_landscape.m_normalMap->GetValue( oldPos.x, oldPos.z );

            OfficerOrderTrail *orders = new OfficerOrderTrail();
            orders->m_pos = oldPos -  g_upVector * 1;
            orders->m_front = m_vel;
            orders->m_front.Normalise();
            orders->m_right = orders->m_front ^ landUp;
            orders->m_birthTime = GetNetworkTime();
            int index = g_app->m_location->m_effects.PutData( orders );
            orders->m_id.Set( m_id.GetTeamId(), UNIT_EFFECTS, index, -1 );
            orders->m_id.GenerateUniqueId();
        }
    }

    return false;
}


void MultiwiniaOfficerOrders::Render( double _time )
{
    if( !g_app->m_location->IsFriend(g_app->m_globalWorld->m_myTeamId, m_id.GetTeamId()) ) return;

    double size = 15.0;
    Vector3 predictedPos = m_pos + m_vel * _time;
    Vector3 predictedUp = g_app->m_location->m_landscape.m_normalMap->GetValue( predictedPos.x, predictedPos.z );

    Vector3 landUp = g_app->m_location->m_landscape.m_normalMap->GetValue( predictedPos.x, predictedPos.z );
    if( m_routeId != -1 )
    {
        OfficerOrderTrail orders;
        orders.m_pos = predictedPos;
        orders.m_front = m_vel;
        orders.m_front.Normalise();
        orders.m_right = orders.m_front ^ landUp;
        orders.m_birthTime = GetNetworkTime();    
        orders.m_id.SetTeamId( m_id.GetTeamId() );
        orders.Render(0.0f);
    }

    double alpha = 0.9;
    if( m_arrivedTimer >= 0.0 )
    {
        double fraction = 1.0 - ( m_arrivedTimer + _time );
        fraction = max( fraction, 0.0 );
        fraction = min( fraction, 1.0 );
        //alpha = 0.7 * fraction;
        size *= fraction;
    }

    Vector3 camUp = g_app->m_camera->GetUp();    
    Vector3 camRight = g_app->m_camera->GetRight();

    camUp.SetLength( size );
    camRight.SetLength( size );

    if( g_app->Multiplayer() )
    {
        Team *team = g_app->m_location->m_teams[m_id.GetTeamId()];
        RGBAColour colour = team->m_colour;
        colour.a *= alpha;

        glColor4ubv( colour.GetData() );
    }
    else
    {
        glColor4f( 1.0, 0.3, 1.0, alpha );
    }
       

    glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/starburst.bmp" ) );    
    //glDisable       ( GL_DEPTH_TEST );

    glBegin( GL_QUADS );
        glTexCoord2i( 0, 1 );       glVertex3dv( (predictedPos - camRight + camUp).GetData() );
        glTexCoord2i( 1, 1 );       glVertex3dv( (predictedPos + camRight + camUp).GetData() );
        glTexCoord2i( 1, 0 );       glVertex3dv( (predictedPos + camRight - camUp).GetData() );
        glTexCoord2i( 0, 0 );       glVertex3dv( (predictedPos - camRight - camUp).GetData() );
    glEnd();

    //glEnable        ( GL_DEPTH_TEST );
    glDisable       ( GL_TEXTURE_2D );
}


void MultiwiniaOfficerOrders::FollowRoute()
{
    if(m_routeId == -1 )
    {
        return;
    }

    Route *route = g_app->m_location->m_routingSystem.GetRoute( m_routeId );
    if(!route)
    {
        m_routeId = -1;
        return;
    }

    bool waypointChange = false;

    if (m_routeWayPointId == -1)
    {
        m_routeWayPointId = 1;
        waypointChange = true;
    }
    else
    {
        double distToWaypoint = ( m_pos - m_wayPoint ).Mag();
        if( distToWaypoint < 10 )
        {
            ++m_routeWayPointId;    
            waypointChange = true;
        }
    }


    if( waypointChange )
    {
        if (m_routeWayPointId >= route->m_wayPoints.Size())
        {
            m_routeWayPointId = -1;
            m_routeId = -1;
            return;
        }

        WayPoint *waypoint = route->m_wayPoints.GetData(m_routeWayPointId);
        if( !waypoint )
        {
            m_routeId = -1;
            return;
        }

        m_wayPoint = waypoint->GetPos();
        m_wayPoint.y = g_app->m_location->m_landscape.m_heightMap->GetValue( m_wayPoint.x, m_wayPoint.z );            
    }   
}


// ============================================================================


OfficerOrderTrail::OfficerOrderTrail()
:   WorldObject(),
    m_birthTime(0.0)
{
    m_type = EffectOfficerOrderTrail;
}


bool OfficerOrderTrail::Advance()
{
    if( GetNetworkTime() > m_birthTime + 1.0 )
    {
        return true;
    }

    return false;
}


void OfficerOrderTrail::Render( double _time )
{
    if( g_app->m_globalWorld->m_myTeamId != m_id.GetTeamId() ) return;

    Vector3 toCam = g_app->m_camera->GetPos() - m_pos;
    double toCamMag = toCam.Mag();

    double size = 20.0 * toCamMag / 2000.0;
    if( size < 5 ) size = 5;

    double age = GetNetworkTime() - m_birthTime;
    double alpha = 1.0 - age;
    alpha *= 0.6;

    if( alpha < 0.0 ) alpha = 0.0;
    if( alpha > 1.0 ) alpha = 1.0;

    Vector3 camUp = m_front;    
    Vector3 camRight = m_right;
    //camRight.Normalise();

    //Vector3 camUp = g_app->m_camera->GetUp();
    //Vector3 camRight = g_app->m_camera->GetRight();

    camUp.SetLength( size );
    camRight.SetLength( size );

    Team *team = g_app->m_location->m_teams[m_id.GetTeamId()];
    RGBAColour colour = team->m_colour;
    colour.a *= alpha;
    
    glColor4ubv( colour.GetData() );

    glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "icons/widearrow.bmp" ) );    
    //glDisable       ( GL_DEPTH_TEST );

    glEnable        ( GL_BLEND );
    glDisable       ( GL_CULL_FACE );

    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );  
    glBegin( GL_QUADS );
        glTexCoord2i( 0, 1 );       glVertex3dv( (m_pos - camRight + camUp).GetData() );
        glTexCoord2i( 1, 1 );       glVertex3dv( (m_pos + camRight + camUp).GetData() );
        glTexCoord2i( 1, 0 );       glVertex3dv( (m_pos + camRight - camUp).GetData() );
        glTexCoord2i( 0, 0 );       glVertex3dv( (m_pos - camRight - camUp).GetData() );
    glEnd();

    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );
    //glEnable        ( GL_DEPTH_TEST );
    glDisable       ( GL_TEXTURE_2D );
}





