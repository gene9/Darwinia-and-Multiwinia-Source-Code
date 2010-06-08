#include "lib/universal_include.h"
#include "lib/resource.h"
#include "lib/shape.h"
#include "lib/math_utils.h"
#include "lib/debug_render.h"
#include "lib/hi_res_time.h"
#include "lib/text_renderer.h"
#include "lib/preferences.h"
#include "lib/profiler.h"

#include "lib/input/input.h"
#include "lib/input/input_types.h"

#include "worldobject/officer.h"
#include "worldobject/teleport.h"

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
#include "sepulveda.h"
#include "global_world.h"

#include "sound/soundsystem.h"


Officer::Officer()
:   Entity(),
    m_demoted(false),
    m_ordersBuildingId(-1),
    m_wayPointTeleportId(-1),
    m_shield(0),
    m_absorb(false),
    m_absorbTimer(2.0f)
{
    m_type = TypeOfficer;
    m_state = StateIdle;
    m_orders = OrderNone;

    m_shape = g_app->m_resource->GetShape( "darwinian.shp" );
    DarwiniaReleaseAssert( m_shape, "Shape not found : officer.shp" );

    m_flagMarker = m_shape->m_rootFragment->LookupMarker( "MarkerFlag" );

    m_centrePos = m_shape->CalculateCentre(g_identityMatrix34);
    m_radius = m_shape->CalculateRadius(g_identityMatrix34, m_centrePos);
}


Officer::~Officer()
{
    if( m_id.GetTeamId() != 255 )
    {
        Team *team = &g_app->m_location->m_teams[ m_id.GetTeamId() ];
        team->UnRegisterSpecial( m_id );
    }
}


void Officer::Begin()
{
    Entity::Begin();

    m_wayPoint = m_pos;
        
    m_flag.SetPosition( m_pos );
    m_flag.SetOrientation( m_front, g_upVector );
    m_flag.SetSize( 20.0f );
    m_flag.Initialise();

    if( m_id.GetTeamId() != 255 )
    {
        Team *team = &g_app->m_location->m_teams[ m_id.GetTeamId() ];
        team->RegisterSpecial( m_id );
    }
}


void Officer::ChangeHealth( int amount )
{
    bool dead = m_dead;
    
    if( amount < 0 && m_shield > 0 )
    {
        int shieldLoss = min( m_shield*10, -amount );
        for( int i = 0; i < shieldLoss/10.0f; ++i )
        {
            Vector3 vel( syncsfrand(40.0f), 0.0f, syncsfrand(40.0f) );
            g_app->m_location->SpawnSpirit( m_pos, vel, 0, WorldObjectId() );
        }
        m_shield -= shieldLoss/10.0f;
        amount += shieldLoss;
    }

    Entity::ChangeHealth( amount );

    if( !dead && m_dead )
    {
        // We just died
        Matrix34 transform( m_front, g_upVector, m_pos );
        g_explosionManager.AddExplosion( m_shape, transform );         
    }
}


void Officer::Render( float _predictionTime )
{
    RenderPixelEffect( _predictionTime );
    RenderShield( _predictionTime );

    if( m_enabled )
    {
        RenderFlag( _predictionTime );
    }
}


void Officer::RenderSpirit( Vector3 const &_pos )
{
    Vector3 pos = _pos;

    int innerAlpha = 255;
    int outerAlpha = 40;
    int glowAlpha = 20;
    
    float spiritInnerSize = 0.5f;
    float spiritOuterSize = 1.5f;
    float spiritGlowSize = 10;
 
    float size = spiritInnerSize;
    glColor4ub(100, 250, 100, innerAlpha );            
    
    glDisable( GL_TEXTURE_2D );
    
    glBegin( GL_QUADS );
        glVertex3fv( (pos - g_app->m_camera->GetUp()*size).GetData() );
        glVertex3fv( (pos + g_app->m_camera->GetRight()*size).GetData() );
        glVertex3fv( (pos + g_app->m_camera->GetUp()*size).GetData() );
        glVertex3fv( (pos - g_app->m_camera->GetRight()*size).GetData() );
    glEnd();    

    size = spiritOuterSize;
    glColor4ub(100, 250, 100, outerAlpha );            
        
    glBegin( GL_QUADS );
        glVertex3fv( (pos - g_app->m_camera->GetUp()*size).GetData() );
        glVertex3fv( (pos + g_app->m_camera->GetRight()*size).GetData() );
        glVertex3fv( (pos + g_app->m_camera->GetUp()*size).GetData() );
        glVertex3fv( (pos - g_app->m_camera->GetRight()*size).GetData() );
    glEnd();    

    size = spiritGlowSize;
    glColor4ub(100,250,100, glowAlpha );

    glEnable( GL_TEXTURE_2D );
    glBindTexture( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/glow.bmp" ) );
    glBegin( GL_QUADS );
        glTexCoord2i(0,0);      glVertex3fv( (pos - g_app->m_camera->GetUp()*size).GetData() );
        glTexCoord2i(1,0);      glVertex3fv( (pos + g_app->m_camera->GetRight()*size).GetData() );
        glTexCoord2i(1,1);      glVertex3fv( (pos + g_app->m_camera->GetUp()*size).GetData() );
        glTexCoord2i(0,1);      glVertex3fv( (pos - g_app->m_camera->GetRight()*size).GetData() );
    glEnd();
    glDisable( GL_TEXTURE_2D );
}


void Officer::RenderShield( float _predictionTime )
{
    float timeIndex = g_gameTime + m_id.GetUniqueId() * 20;

    glDisable       ( GL_CULL_FACE );
    glDepthMask     ( false );
    glEnable        ( GL_BLEND );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );

    Vector3 predictedPos = m_pos + m_vel * _predictionTime;
    predictedPos.y += 10.0f;

    for( int i = 0; i < m_shield; ++i )
    {
        Vector3 spiritPos = predictedPos;
        spiritPos.x += sinf( timeIndex * 1.8f + i * 2.0f ) * 10.0f;
        spiritPos.y += cosf( timeIndex * 2.1f + i * 1.2f ) * 6.0f;
        spiritPos.z += sinf( timeIndex * 2.4f + i * 1.4f ) * 10.0f;

        RenderSpirit( spiritPos );
    }
    
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glDisable       ( GL_BLEND );
    glDepthMask     ( true );
    glEnable        ( GL_CULL_FACE );
}


void Officer::RenderFlag( float _predictionTime )
{
    float timeIndex = g_gameTime + m_id.GetUniqueId() * 10;
    float size = 20.0f;

    Vector3 up = g_upVector;
    Vector3 front = m_front * -1;
    front.y = 0;
    front.Normalise();
    
    if( m_orders != OrderNone )
    {
        up.RotateAround( front * sinf(timeIndex*2) * 0.3f );
    }

    Vector3 entityUp = g_upVector;
    Vector3 entityRight(m_front ^ entityUp);
    Vector3 entityFront = entityUp ^ entityRight;        
    Matrix34 mat( entityFront, entityUp, m_pos + m_vel * _predictionTime );
    Vector3 flagPos = m_flagMarker->GetWorldMatrix(mat).pos;
    
    int texId = -1;
    if      ( m_orders == OrderNone )                   texId = g_app->m_resource->GetTexture( "icons/banner_none.bmp" );
    else if ( m_orders == OrderGoto )                   texId = g_app->m_resource->GetTexture( "icons/banner_goto.bmp" );
    else if ( m_orders == OrderFollow && !m_absorb )    texId = g_app->m_resource->GetTexture( "icons/banner_follow.bmp" );
    else if ( m_orders == OrderFollow && m_absorb )     texId = g_app->m_resource->GetTexture( "icons/banner_absorb.bmp" );

    m_flag.SetTexture( texId );
    m_flag.SetPosition( flagPos );
    m_flag.SetOrientation( front, up );
    m_flag.SetSize( size );
    m_flag.Render();
}


bool Officer::RenderPixelEffect( float _predictionTime )
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
    Vector3 entityFront = entityUp ^ entityRight;
        
    Matrix34 mat( entityFront, entityUp, predictedPos );

    //
    // If we are damaged, flicked in and out based on our health

    if( m_renderDamaged )
    {
        float timeIndex = g_gameTime + m_id.GetUniqueId() * 10;
        float thefrand = frand();
        if      ( thefrand > 0.7f ) mat.f *= ( 1.0f - sinf(timeIndex) * 0.5f );
        else if ( thefrand > 0.4f ) mat.u *= ( 1.0f - sinf(timeIndex) * 0.2f );    
        else                        mat.r *= ( 1.0f - sinf(timeIndex) * 0.5f );
        glEnable( GL_BLEND );
        glBlendFunc( GL_ONE, GL_ONE );
    }

    
    //
    // Render our shape

    g_app->m_renderer->SetObjectLighting();
    glDisable       (GL_TEXTURE_2D);
    glShadeModel    (GL_SMOOTH);

    m_shape->Render( _predictionTime, mat );

    glShadeModel    (GL_FLAT);
    glEnable        (GL_TEXTURE_2D);
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    g_app->m_renderer->UnsetObjectLighting();


    g_app->m_renderer->MarkUsedCells(m_shape, mat);
    
    return true;
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
    bool arrived = AdvanceToTargetPosition();
    if( arrived )
    {
        m_state = StateIdle;
    }

    return false;
}


bool Officer::AdvanceToTargetPosition()
{
    //
    // Work out where we want to be next

    if( (m_wayPoint - m_pos).Mag() > 5.0f )
    {    
        float speed = m_stats[StatSpeed];
        if( m_state == StateIdle ) speed *= 0.2f;

        float amountToTurn = SERVER_ADVANCE_PERIOD * 3.0f;
        Vector3 targetDir = (m_wayPoint- m_pos).Normalise();
        Vector3 actualDir = m_front * (1.0f - amountToTurn) + targetDir * amountToTurn;
        actualDir.Normalise();

        Vector3 oldPos = m_pos;
        Vector3 newPos = m_pos + actualDir * speed * SERVER_ADVANCE_PERIOD;


        //
        // Slow us down if we're going up hill
        // Speed up if going down hill
    
        float currentHeight = g_app->m_location->m_landscape.m_heightMap->GetValue( oldPos.x, oldPos.z );
        float nextHeight = g_app->m_location->m_landscape.m_heightMap->GetValue( newPos.x, newPos.z );
        float factor = 1.0f - (currentHeight - nextHeight) / -3.0f;
        if( factor < 0.1f ) factor = 0.1f;
        if( factor > 2.0f ) factor = 2.0f;
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
    }
    m_vel.Zero();
    return false;
}


bool Officer::SearchForRandomPosition()
{
    float distance = 30.0f;
    float angle = syncsfrand(2.0f * M_PI);

    m_wayPoint = m_pos + Vector3( sinf(angle) * distance,
                                   0.0f,
                                   cosf(angle) * distance );

    m_wayPoint = PushFromObstructions( m_wayPoint );    
    m_wayPoint.y = g_app->m_location->m_landscape.m_heightMap->GetValue( m_wayPoint.x, m_wayPoint.z );    

    return true;
}


void Officer::Absorb()
{
    int numFound;
    WorldObjectId *ids = g_app->m_location->m_entityGrid->GetFriends( m_pos.x, m_pos.z, OFFICER_ABSORBRANGE, &numFound, m_id.GetTeamId() );

    WorldObjectId nearestId;
    float nearestDistance = 99999.9f;

    for( int i = 0; i < numFound; ++i )
    {
        WorldObjectId id = ids[i];
        Entity *entity = g_app->m_location->GetEntity( id );
        if( entity && entity->m_type == Entity::TypeDarwinian && !entity->m_dead )
        {
            float distance = ( entity->m_pos - m_pos ).Mag();
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
        if( m_absorbTimer < 0.0f ) 
        {
            Entity *entity = g_app->m_location->GetEntity( nearestId );

            g_app->m_location->m_entityGrid->RemoveObject( nearestId, entity->m_pos.x, entity->m_pos.z, entity->m_radius );
            g_app->m_location->m_teams[nearestId.GetTeamId()].m_others.MarkNotUsed( nearestId.GetIndex() );
            ++m_shield;
            m_absorbTimer = 1.0f;
        }
    }
}


bool Officer::Advance( Unit *_unit )
{
    if( !m_onGround ) AdvanceInAir(_unit);   
    bool amIDead = Entity::Advance(_unit);
    if( m_inWater != -1.0f ) AdvanceInWater(_unit);

    if( m_onGround && !m_dead ) m_pos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( m_pos.x, m_pos.z );    

    
    //
    // Advance in whatever state we are in

    if( !amIDead && m_onGround && m_inWater == -1.0f )
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
        m_vel.y -= 20.0f;
        m_pos.y += m_vel.y * SERVER_ADVANCE_PERIOD;
    }
    

    //
    // If we are giving orders, render them

    if( m_orders == OrderGoto )
    {
        if( syncfrand() < 0.05f )
        {
            OfficerOrders *orders = new OfficerOrders();
            orders->m_pos = m_pos + Vector3(0,2,0);
            orders->m_wayPoint = m_orderPosition;
            int index = g_app->m_location->m_effects.PutData( orders );
            orders->m_id.Set( m_id.GetTeamId(), UNIT_EFFECTS, index, -1 );
            orders->m_id.GenerateUniqueId();
        }
    }

    //
    // If we are absorbing, look around for Darwinians

    if( m_absorb ) Absorb();


    //
    // Attack anything nearby with our "shield"
    
    if( m_shield > 0 )
    {
        WorldObjectId id = g_app->m_location->m_entityGrid->GetBestEnemy( m_pos.x, m_pos.z, 0.0f, OFFICER_ATTACKRANGE, m_id.GetTeamId() );
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
            if( exitFound ) m_wayPoint = exitPos + exitFront * 30.0f;
            if( m_orders == OrderGoto ) m_orders = OrderNone;
            m_wayPointTeleportId = -1;
        }
    }

    return amIDead || m_demoted;
}


void Officer::SetWaypoint( Vector3 const &_wayPoint )
{
		m_wayPoint = _wayPoint;
		m_state = StateToWaypoint; 
        
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
                float distance = ( building->m_pos - _wayPoint ).Mag();
                Teleport *teleport = (Teleport *) building;
                if( distance < 5.0f && teleport->Connected() )
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
}


void Officer::CancelOrderSounds()
{
    g_app->m_soundSystem->StopAllSounds( m_id, "Officer SetOrderNone" );
    g_app->m_soundSystem->StopAllSounds( m_id, "Officer SetOrderGoto" );
    g_app->m_soundSystem->StopAllSounds( m_id, "Officer SetOrderFollow" );
    g_app->m_soundSystem->StopAllSounds( m_id, "Officer SetOrderAbsorb" );
}


void Officer::SetOrders( Vector3 const &_orders )
{
    static float lastOrderSet = 0.0f;

    if( !g_app->m_location->IsWalkable( m_pos, _orders ) )
    {
        g_app->m_sepulveda->Say( "officer_notwalkable" );
    }
    else
    {
        float distanceToOrders = ( _orders - m_pos ).Mag();

        if( distanceToOrders > 20.0f )
        {
            m_orderPosition = _orders;        
            m_orders = OrderGoto;
            m_absorb = false;

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
                if( building->m_type == Building::TypeRadarDish ||
                    building->m_type == Building::TypeBridge )
                {
                    float distance = ( building->m_pos - _orders ).Mag();                    
                    if( distance < 5.0f )
                    {
                        Teleport *teleport = (Teleport *) building;
                        m_ordersBuildingId = building->m_id.GetUniqueId();
                        Vector3 entrancePos, entranceFront;
                        teleport->GetEntrance( entrancePos, entranceFront );
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

            float timeNow = GetHighResTime();
            if( timeNow > lastOrderSet + 1.0f )
            {
                lastOrderSet = timeNow;
                OfficerOrders orders;
                orders.m_pos = m_pos;
                orders.m_wayPoint = m_orderPosition;
                while( true )
                {   
                    if( orders.m_arrivedTimer < 0.0f )
                    {
                        g_app->m_particleSystem->CreateParticle( orders.m_pos, g_zeroVector, Particle::TypeMuzzleFlash, 50.0f );
                        g_app->m_particleSystem->CreateParticle( orders.m_pos, g_zeroVector, Particle::TypeMuzzleFlash, 40.0f );
                    }
                    if( orders.Advance() ) break;
                }

                CancelOrderSounds();
                g_app->m_soundSystem->TriggerEntityEvent( this, "SetOrderGoto" );
            }
        }
        else
        {
            float timeNow = GetHighResTime();
            int researchLevel = g_app->m_globalWorld->m_research->CurrentLevel( GlobalResearch::TypeOfficer );

            if( timeNow > lastOrderSet + 0.3f )
            {
                lastOrderSet = timeNow;

                switch( researchLevel )
                {
                    case 0 :
                    case 1 :
                    case 2 :
                        m_orders = OrderNone;
                        break;

                    case 3 :
                        if      ( m_orders == OrderNone )       m_orders = OrderFollow;
                        else if ( m_orders == OrderFollow )     m_orders = OrderNone;
                        else if ( m_orders == OrderGoto )       m_orders = OrderNone;
                        break;

                    case 4 :
                        if      ( m_orders == OrderNone )                   m_orders = OrderFollow;
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
    }
}

void Officer::SetNextMode()
{
    static float lastOrderSet = 0.0f;

    float timeNow = GetHighResTime();
    int researchLevel = g_app->m_globalWorld->m_research->CurrentLevel( GlobalResearch::TypeOfficer );

    if( timeNow > lastOrderSet + 0.3f )
    {
        lastOrderSet = timeNow;

        switch( researchLevel )
        {
            case 0 :
            case 1 :
            case 2 :
                m_orders = OrderNone;
                break;

            case 3 :
                if      ( m_orders == OrderNone )       m_orders = OrderFollow;
                else if ( m_orders == OrderFollow )     m_orders = OrderNone;
                else if ( m_orders == OrderGoto )       m_orders = OrderNone;
                break;

            case 4 :
                if      ( m_orders == OrderNone )                   m_orders = OrderFollow;
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

void Officer::SetPreviousMode()
{
    static float lastOrderSet = 0.0f;

    float timeNow = GetHighResTime();
    int researchLevel = g_app->m_globalWorld->m_research->CurrentLevel( GlobalResearch::TypeOfficer );

    if( timeNow > lastOrderSet + 0.3f )
    {
        lastOrderSet = timeNow;

        switch( researchLevel )
        {
            case 0 :
            case 1 :
            case 2 :
                m_orders = OrderNone;
                break;

            case 3 :
                if      ( m_orders == OrderNone )       m_orders = OrderFollow;
                else if ( m_orders == OrderFollow )     m_orders = OrderNone;
                else if ( m_orders == OrderGoto )       m_orders = OrderNone;
                break;

            case 4 :
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

char *Officer::GetOrderType( int _orderType )
{
    static char *orders[] = {   "None",
                                "Goto",
                                "Follow"   
                            };

    return orders[ _orderType ];
}

// ============================================================================

OfficerOrders::OfficerOrders()
:   WorldObject(),
    m_arrivedTimer(-1.0f)
{
    m_type = EffectOfficerOrders;
}


bool OfficerOrders::Advance()
{
    if( m_arrivedTimer >= 0.0f )
    {
        // We are already here, so just fade out
        m_vel.Zero();
        m_arrivedTimer += SERVER_ADVANCE_PERIOD;        
        if( m_arrivedTimer > 1.0f ) return true;        
    }
    else
    {
        float speed = m_vel.Mag();
        speed *= 1.1f;
        speed = max( speed, 30.0f );
        speed = min( speed, 150.0f );

        Vector3 toWaypoint = ( m_wayPoint - m_pos );
        toWaypoint.y = 0.0f;
        float distance = toWaypoint.Mag();

        toWaypoint.Normalise();
        m_vel = toWaypoint * speed;
        Vector3 oldPos = m_pos;
        m_pos += m_vel * SERVER_ADVANCE_PERIOD;

        float landHeight = g_app->m_location->m_landscape.m_heightMap->GetValue( m_pos.x, m_pos.z );
        m_pos.y = landHeight + 2.0f;

        m_vel = ( m_pos - oldPos ) / SERVER_ADVANCE_PERIOD;

        g_app->m_particleSystem->CreateParticle( oldPos, g_zeroVector, Particle::TypeMuzzleFlash, 30.0f );

        if( m_vel.Mag() * SERVER_ADVANCE_PERIOD > distance ) m_arrivedTimer = 0.0f;
        if( distance < 3.0f ) m_arrivedTimer = 0.0f;
    }
    
    return false;
}


void OfficerOrders::Render( float _time )
{
    float size = 15.0f;
    Vector3 predictedPos = m_pos + m_vel * _time;

    float alpha = 0.7f;
    if( m_arrivedTimer >= 0.0f )
    {
        float fraction = 1.0f - ( m_arrivedTimer + _time );
        fraction = max( fraction, 0.0f );
        fraction = min( fraction, 1.0f );
        //alpha = 0.7f * fraction;
        size *= fraction;
    }

    Vector3 camUp = g_app->m_camera->GetUp() * size;
    Vector3 camRight = g_app->m_camera->GetRight() * size;

    glColor4f( 1.0f, 0.3f, 1.0f, alpha );

    glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/starburst.bmp" ) );
    glDisable       ( GL_DEPTH_TEST );

    glBegin( GL_QUADS );
        glTexCoord2i( 0, 0 );       glVertex3fv( (predictedPos - camRight + camUp).GetData() );
        glTexCoord2i( 1, 0 );       glVertex3fv( (predictedPos + camRight + camUp).GetData() );
        glTexCoord2i( 1, 1 );       glVertex3fv( (predictedPos + camRight - camUp).GetData() );
        glTexCoord2i( 0, 1 );       glVertex3fv( (predictedPos - camRight - camUp).GetData() );
    glEnd();

    glEnable        ( GL_DEPTH_TEST );
    glDisable       ( GL_TEXTURE_2D );
}
