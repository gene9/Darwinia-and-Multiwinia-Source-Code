#include "lib/universal_include.h"

#include <math.h>

#include "lib/resource.h"
#include "lib/shape.h"
#include "lib/math_utils.h"
#include "lib/text_renderer.h"
#include "lib/debug_render.h"

#include "lib/input/input.h"
#include "lib/input/input_types.h"

#include "globals.h"
#include "app.h"
#include "location.h"
#include "team.h"
#include "unit.h"
#include "entity_grid.h"
#include "camera.h"
#include "user_input.h"
#include "main.h"
#include "global_world.h"
#include "level_file.h"
#include "explosion.h"

#include "worldobject/gunturret.h"
#include "worldobject/darwinian.h"
#include "worldobject/ai.h"

#include "sound/soundsystem.h"


GunTurret::GunTurret()
:   Building(),
    m_retargetTimer(0.0f),
    m_fireTimer(0.0f),
    m_nextBarrel(0),
    m_turret(NULL),
    m_ownershipTimer(0.0f),
    m_health(60.0f),
    m_targetCreated(false),
    m_aiTargetCreated(false)
{
    SetShape( g_app->m_resource->GetShape( "battlecannonbase.shp" ) );
    m_type = TypeGunTurret;
    
    m_turret = g_app->m_resource->GetShape( "battlecannonturret.shp" );
    m_barrel = g_app->m_resource->GetShape( "battlecannonbarrel.shp" );
    
    m_barrelMount = m_turret->m_rootFragment->LookupMarker( "MarkerBarrel" );
    
    for( int i = 0; i < GUNTURRET_NUMBARRELS; ++i )
    {
        char name[64];
        sprintf( name, "MarkerBarrelEnd0%d", i+1 );
        m_barrelEnd[i] = m_barrel->m_rootFragment->LookupMarker( name );
    }

    for( int i = 0; i < GUNTURRET_NUMSTATUSMARKERS; ++i )
    {
        char name[64];
        sprintf( name, "MarkerStatus0%d", i+1 );
        m_statusMarkers[i] = m_shape->m_rootFragment->LookupMarker( name );
    }
}


void GunTurret::Initialise( Building *_template )
{
    _template->m_up = g_app->m_location->m_landscape.m_normalMap->GetValue( _template->m_pos.x, _template->m_pos.z );
    Vector3 right( 1, 0, 0 );
    _template->m_front = right ^ _template->m_up;

    Building::Initialise( _template );    

    m_turretFront = m_front;
    m_barrelUp = g_upVector;
    m_centrePos = m_pos;
    m_radius = 30.0f;

    SearchForRandomPos();
}


void GunTurret::SetDetail( int _detail )
{
    Building::SetDetail( _detail );

    m_centrePos = m_pos;
    m_radius = 30.0f;
}


void GunTurret::Damage( float _damage )
{
    m_health += _damage;
    
    if( m_health <= 0.0f )
    {
        ExplodeBody();
    }
}


void GunTurret::ExplodeBody()
{
    Matrix34 turretPos( m_turretFront, m_up, m_pos );
    Vector3 barrelPos = m_barrelMount->GetWorldMatrix(turretPos).pos;
    Vector3 barrelFront = m_barrelMount->GetWorldMatrix(turretPos).f;
    Matrix34 barrelMat( barrelFront, m_up, barrelPos );

    g_explosionManager.AddExplosion( m_turret, turretPos );
    g_explosionManager.AddExplosion( m_barrel, barrelMat );
}


bool GunTurret::SearchForTargets()
{
    WorldObjectId previousTarget = m_targetId;
    m_targetId.SetInvalid();

    if( m_id.GetTeamId() != 255 )
    {
        m_targetId = g_app->m_location->m_entityGrid->GetBestEnemy( m_pos.x, m_pos.z, 
                                                                    GUNTURRET_MINRANGE, 
                                                                    GUNTURRET_MAXRANGE, 
                                                                    m_id.GetTeamId() );   
    }

    Entity *entity = g_app->m_location->GetEntity( m_targetId );
    
    if( entity && m_targetId != previousTarget )
    {
        g_app->m_soundSystem->TriggerBuildingEvent( this, "TargetSighted" );
    }

    return( m_targetId.IsValid() );
}


void GunTurret::SearchForRandomPos()
{
    float angle = syncfrand( 2.0f * M_PI );
    float distance = 200.0f + syncfrand(200.0f);
    float height = syncfrand(100.0f);

    m_target = m_pos + Vector3( cosf(angle) * distance,
                                height,
                                sinf(angle) * distance );
}


void GunTurret::RecalculateOwnership()
{
    int teamCount[NUM_TEAMS];
    memset( teamCount, 0, NUM_TEAMS*sizeof(int));

    for( int i = 0; i < GetNumPorts(); ++i )
    {
        WorldObjectId id = GetPortOccupant(i);
        if( id.IsValid() )
        {
            teamCount[id.GetTeamId()] ++;
        }
    }

    int winningTeam = -1;
    for( int i = 0; i < NUM_TEAMS; ++i )
    {
        if( teamCount[i] > 0 && 
            winningTeam == -1 )
        {
            winningTeam = i;
        }
        else if( winningTeam != -1 && 
                 teamCount[i] > 0 && 
                 teamCount[i] > teamCount[winningTeam] )
        {
            winningTeam = i;
        }
    }

    if( winningTeam == -1 )
    {
        SetTeamId(255);
    }
    else if( winningTeam == 0 )
    {
        // Green own the building, but set to yellow so the player
        // can control the building
        SetTeamId(2);
    }
    else 
    {
        // Red own the building
        SetTeamId(winningTeam);
    }
}


void GunTurret::PrimaryFire()
{
    bool fired = false;

    for( int i = 0; i < 2; ++i )
    {
        if( GetPortOccupant(m_nextBarrel).IsValid() )
        {
            Matrix34 turretMat( m_turretFront, g_upVector, m_pos );
            Vector3 barrelMountPos = m_barrelMount->GetWorldMatrix(turretMat).pos;
            Vector3 barrelMountFront = m_barrelMount->GetWorldMatrix(turretMat).f;
            Vector3 barrelRight = barrelMountFront ^ m_barrelUp;
            barrelMountFront = m_barrelUp ^ barrelRight;
            Matrix34 barrelMountMat( barrelMountFront, m_barrelUp, barrelMountPos );
            Matrix34 barrelMat = m_barrelEnd[m_nextBarrel]->GetWorldMatrix(barrelMountMat);
            
            Vector3 shellPos = barrelMat.pos;
            Vector3 shellVel = barrelMountFront.SetLength(500.0f);
        
            
            g_app->m_location->FireTurretShell( shellPos, shellVel );        
            fired = true;
        }
        
        ++m_nextBarrel;
        if( m_nextBarrel >= GUNTURRET_NUMBARRELS ) 
        {
            m_nextBarrel = 0;
        }

    }

    if( fired )
    {
        g_app->m_soundSystem->TriggerBuildingEvent( this, "FireShell" );
    }
}


bool GunTurret::Advance()
{
    if( m_health <= 0.0f ) return true;

    //
    // Create an AI target on top of us, 
    // to get Darwinians to fight hard for control

    if( !m_aiTargetCreated )
    {
        Building *aiTarget = Building::CreateBuilding( TypeAITarget );
        aiTarget->m_pos = m_pos;
        aiTarget->m_front = m_front;        
        g_app->m_location->m_buildings.PutData( aiTarget );
        int uniqueId = g_app->m_globalWorld->GenerateBuildingId();
        aiTarget->m_id.SetUniqueId( uniqueId );
        m_aiTargetCreated = true;
    }

    //
    // Create a GunTurret target that darwinians can run from

    if( !m_targetCreated )
    {
        GunTurretTarget *target = new GunTurretTarget( m_id.GetUniqueId() );
        int index = g_app->m_location->m_effects.PutData( target );
        target->m_id.Set( m_id.GetTeamId(), UNIT_EFFECTS, index, -1 );        
        target->m_id.GenerateUniqueId();
        m_targetCreated = true;
    }
    

    // 
    // Time to recalculate ownership?

    m_ownershipTimer -= SERVER_ADVANCE_PERIOD;
    if( m_ownershipTimer <= 0.0f )
    {
        RecalculateOwnership();
        m_ownershipTimer = GUNTURRET_OWNERSHIPTIMER;
    }


    Team *team = g_app->m_location->GetMyTeam();
    bool underPlayerControl = ( team && team->m_currentBuildingId == m_id.GetUniqueId() );

    
    //
    // Look for a new target
    
    bool primaryFire = false;
    bool secondaryFire = false;

    if( underPlayerControl )
    {
        if( m_id.GetTeamId() != 2 )
        {
            // Player has lost control of the building
            team->SelectUnit( -1, -1, -1 );
            g_app->m_camera->RequestMode( Camera::ModeFreeMovement );
            return Building::Advance();
        }
        m_target = g_app->m_userInput->GetMousePos3d();

#ifdef JAMES_FIX
        primaryFire = g_controlBindings->ControlMouseEvent( ControlBindings::ControlUnitPrimaryFire, team->m_currentMouseStatus, team->m_mouseDeltas );
        secondaryFire = g_controlBindings->ControlMouseEvent( ControlBindings::ControlUnitSecondaryFire, team->m_currentMouseStatus, team->m_mouseDeltas );
#endif // JAMES_FIX
        primaryFire = g_inputManager->controlEvent( ControlUnitPrimaryFireTarget ) || g_inputManager->controlEvent( ControlUnitStartSecondaryFireDirected );
        secondaryFire = g_inputManager->controlEvent( ControlUnitSecondaryFireTarget );
    }
    else
    {
        m_retargetTimer -= SERVER_ADVANCE_PERIOD;
        if( m_retargetTimer <= 0.0f )
        {
            bool targetFound = SearchForTargets();
            if( !targetFound && m_id.GetTeamId() != 255 ) SearchForRandomPos();
            m_retargetTimer = GUNTURRET_RETARGETTIMER;
        }
    
        if( m_targetId.IsValid() )
        {
            // Check our target is still alive
            Entity *target = g_app->m_location->GetEntity( m_targetId );
            if( !target )
            {
                m_targetId.SetInvalid();
                return Building::Advance();
            }
            m_target = target->m_pos;
            m_target += Vector3( sinf(g_gameTime*3)*20, fabs(sinf(g_gameTime*2)*10), cosf(g_gameTime*3)*20 );

            Vector3 targetFront = ( m_target - m_pos );
            targetFront.HorizontalAndNormalise();
            float angle = acosf( targetFront * m_turretFront );
            
            primaryFire = ( angle < 0.25f );            
        }            
    }

    
    float distance = ( m_target - m_pos ).Mag();
    if( distance < 75.0f ) m_target = m_pos + ( m_target - m_pos ).SetLength(75.0f);
    

    //
    // Face our target

    Matrix34 turretPos( m_turretFront, m_up, m_pos );
    Vector3 barrelPos = m_barrelMount->GetWorldMatrix(turretPos).pos;
	Vector3 toTarget = m_target - barrelPos;
	toTarget.HorizontalAndNormalise();
	float angle = acosf( toTarget * m_turretFront );    
    
    Vector3 rotation = m_turretFront ^ toTarget;
	rotation.SetLength(angle * 0.2f);
	m_turretFront.RotateAround(rotation);    
    m_turretFront.Normalise();
    
    toTarget = (m_target - barrelPos).Normalise();
    Vector3 barrelRight = toTarget ^ g_upVector;
    Vector3 targetBarrelUp = toTarget ^ barrelRight;
    float factor = SERVER_ADVANCE_PERIOD * 2.0f;    
    m_barrelUp = m_barrelUp * (1.0f-factor) + targetBarrelUp * factor;
    
    //
    // Open fire

    if( primaryFire ) PrimaryFire();
    
    return Building::Advance();
}


Vector3 GunTurret::GetTarget()
{
    return m_target;
}


bool GunTurret::IsInView()
{
    Team *team = g_app->m_location->GetMyTeam();
    bool underPlayerControl = ( team && team->m_currentBuildingId == m_id.GetUniqueId() );

    if( underPlayerControl ) 
    {
        return true;
    }
    else
    {
        return Building::IsInView();
    }
}


void GunTurret::Render( float _predictionTime )
{
    if( g_app->m_editing )
    {
        m_turretFront = m_front;
    }

    Matrix34 turretPos( m_turretFront, g_upVector, m_pos );
    m_turret->Render( _predictionTime, turretPos );

    Vector3 barrelPos = m_barrelMount->GetWorldMatrix(turretPos).pos;
    Vector3 barrelFront = m_barrelMount->GetWorldMatrix(turretPos).f;
    Vector3 barrelRight = barrelFront ^ m_barrelUp;
    barrelFront = m_barrelUp ^ barrelRight;
    barrelFront.Normalise();
    Matrix34 barrelMat( barrelFront, m_barrelUp, barrelPos );    
    m_barrel->Render( _predictionTime, barrelMat );
    
    //RenderArrow( barrelPos, barrelPos + barrelFront * 1000.0f, 1.0f );
    
    // 
    // Render targetting crosshair

/*
    Team *team = g_app->m_location->GetMyTeam();
    bool underPlayerControl = ( team && team->m_currentBuildingId == m_id.GetUniqueId() );

    if( underPlayerControl )
    {
        Vector3 targetPos = barrelPos + Vector3(0,10,0) + barrelFront.SetLength( 300.0f );
        float size = 20.0f;
        Vector3 camUp = g_app->m_camera->GetUp();
        Vector3 camRight = g_app->m_camera->GetRight();
        targetPos += camUp * 5.0f;

        glBindTexture( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "icons/mouse_missiletarget.bmp" ) );
        glEnable( GL_TEXTURE_2D );
        glDisable( GL_CULL_FACE );
        glBlendFunc( GL_SRC_ALPHA, GL_ONE );
        glEnable( GL_BLEND );
        glDepthMask( false );
        glDisable( GL_DEPTH_TEST );

        glBegin( GL_QUADS );
            glTexCoord2i(0,0);      glVertex3fv( (targetPos - camRight * size - camUp * size).GetData() );
            glTexCoord2i(1,0);      glVertex3fv( (targetPos + camRight * size - camUp * size).GetData() );
            glTexCoord2i(1,1);      glVertex3fv( (targetPos + camRight * size + camUp * size).GetData() );
            glTexCoord2i(0,1);      glVertex3fv( (targetPos - camRight * size + camUp * size).GetData() );
        glEnd();

        glEnable( GL_DEPTH_TEST );
        glDepthMask( true );
        glDisable( GL_BLEND );
        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        glEnable( GL_CULL_FACE );
        glDisable( GL_TEXTURE_2D );
    }*/


}


void GunTurret::RenderPorts()
{
    glDisable       ( GL_CULL_FACE );
    glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/starburst.bmp" ) );
    glDepthMask     ( false );
    glEnable        ( GL_BLEND );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );

    for( int i = 0; i < GetNumPorts(); ++i )
    {        
        Matrix34 rootMat(m_front, m_up, m_pos);
        Matrix34 worldMat = m_statusMarkers[i]->GetWorldMatrix(rootMat);
        
        //
        // Render the status light

        float size = 6.0f;
        Vector3 camR = g_app->m_camera->GetRight() * size;
        Vector3 camU = g_app->m_camera->GetUp() * size;

        Vector3 statusPos = worldMat.pos;

        WorldObjectId occupantId = GetPortOccupant(i);
        if( !occupantId.IsValid() ) 
        {
            glColor4ub( 150, 150, 150, 255 );
        }
        else
        {
            RGBAColour teamColour = g_app->m_location->m_teams[occupantId.GetTeamId()].m_colour;
            glColor4ubv( teamColour.GetData() );
        }
        
        glBegin( GL_QUADS );
            glTexCoord2i( 0, 0 );           glVertex3fv( (statusPos - camR - camU).GetData() );
            glTexCoord2i( 1, 0 );           glVertex3fv( (statusPos + camR - camU).GetData() );
            glTexCoord2i( 1, 1 );           glVertex3fv( (statusPos + camR + camU).GetData() );
            glTexCoord2i( 0, 1 );           glVertex3fv( (statusPos - camR + camU).GetData() );
        glEnd();
    }

    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glDisable       ( GL_BLEND );
    glDepthMask     ( true );
    glDisable       ( GL_TEXTURE_2D );
    glEnable        ( GL_CULL_FACE );
}

bool GunTurret::DoesRayHit(Vector3 const &_rayStart, Vector3 const &_rayDir, 
                          float _rayLen, Vector3 *_pos, Vector3 *norm )
{
    if( g_app->m_editing )
    {
        return RaySphereIntersection(_rayStart, _rayDir, m_pos, m_radius, _rayLen);
    }
    else
    {
        return Building::DoesRayHit( _rayStart, _rayDir, _rayLen, _pos, norm );
    }
}


void GunTurret::ListSoundEvents( LList<char *> *_list )
{
    Building::ListSoundEvents( _list );

    _list->PutData( "TargetSighted" );
    _list->PutData( "FireShell" );
}


GunTurretTarget::GunTurretTarget( int _buildingId )
:   WorldObject(),
    m_buildingId(_buildingId)
{
    m_type = EffectGunTurretTarget;
}


bool GunTurretTarget::Advance()
{
    GunTurret *turret = (GunTurret *) g_app->m_location->GetBuilding( m_buildingId );
    if( !turret || turret->m_type != Building::TypeGunTurret )
    {
        return true;
    }

    if( turret->GetNumPortsOccupied() > 0 )
    {
        m_pos = turret->GetTarget();    
    }
    else
    {
        m_pos.Zero();
    }

    return false;
}


void GunTurretTarget::Render( float _time )
{
    //RenderSphere( m_pos, 10.0f );
}

