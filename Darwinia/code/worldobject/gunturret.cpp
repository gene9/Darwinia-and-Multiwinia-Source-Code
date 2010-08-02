#include "lib/universal_include.h"

#include <math.h>

#include "lib/resource.h"
#include "lib/shape.h"
#include "lib/math_utils.h"
#include "lib/text_renderer.h"
#include "lib/debug_render.h"

#include "lib/input/input.h"
#include "lib/input/input_types.h"
#include "lib/language_table.h"
#include "lib/file_writer.h"
#include "lib/text_stream_readers.h"

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
#include "location_editor.h"
#include "particle_system.h"
#include "obstruction_grid.h"
#include "renderer.h"
#include "gamecursor.h"

#include "worldobject/gunturret.h"
#include "worldobject/darwinian.h"
#include "worldobject/ai.h"
#include "worldobject/entity.h"

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
	m_state = GUNTURRET_DEFAULT_TYPE;

    m_turret = g_app->m_resource->GetShape( "battlecannonturret.shp" );
    m_barrel = g_app->m_resource->GetShape( "battlecannonbarrel.shp" );

    m_barrelMount = m_turret->m_rootFragment->LookupMarker( "MarkerBarrel" );
    //DarwiniaReleaseAssert( m_barrelMount, "GunTurret: Can't get Marker(%s) from shape(%s), probably a corrupted file\n", barrelMountName, m_turret->m_name );

    for( int i = 0; i < GUNTURRET_NUMBARRELS; ++i )
    {
        char name[64];
        sprintf( name, "MarkerBarrelEnd0%d", i+1 );
        m_barrelEnd[i] = m_barrel->m_rootFragment->LookupMarker( name );
        DarwiniaReleaseAssert( m_barrelEnd[i], "GunTurret: Can't get Marker(%s) from shape(%s), probably a corrupted file\n", name, m_barrel->m_name );
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

	m_state = ((GunTurret *)_template)->m_state;
    m_targetPos = ((GunTurret *)_template)->m_targetPos;
    m_targetRadius = ((GunTurret *)_template)->m_targetRadius;
    m_targetForce = ((GunTurret *)_template)->m_targetForce;

	Building::Initialise( _template );

    m_turretFront = m_front;
    m_barrelUp = g_upVector;
    m_centrePos = m_pos;
    m_radius = 30.0f;

    SearchForRandomPos();

    m_turret = g_app->m_resource->GetShape( "battlecannonturret.shp", m_id.GetTeamId(), false );
    m_barrel = g_app->m_resource->GetShapeOrShape( GetTurretModelName(m_state), "battlecannonbarrel.shp", m_id.GetTeamId(), false );
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
        if( m_state == GunTurretTypeMortar )
        {
            m_targetId = g_app->m_location->m_entityGrid->GetBestEnemy( m_pos.x + m_targetPos.x, m_pos.z + m_targetPos.z, 0.0, m_targetRadius, m_id.GetTeamId() );
        }
        else
        {
           /* m_targetId = g_app->m_location->m_entityGrid->GetBestEnemy( m_pos.x, m_pos.z, 
                                                                        GetMinRange(), 
                                                                        GetMaxRange(), 
                                                                        m_id.GetTeamId() );   */
            int numFound;
            WorldObjectId *s_neighbours = g_app->m_location->m_entityGrid->GetEnemies( m_pos.x, m_pos.z,
                                                         GetMaxRange(), &numFound,
                                                         m_id.GetTeamId() );

            double bestDist = FLT_MAX;

            for( int i = 0; i < numFound; ++i )
            {
                Entity *e = g_app->m_location->GetEntity( s_neighbours[i] );
                if( e->m_type == Entity::TypeAI ) continue;
                if( e->m_type == Entity::TypeSnowBox ) continue;
                //if( e->m_type == Entity::TypeHarvester ) continue;
                if( e->m_dead ) continue;
                double dist = (e->m_pos - m_pos).Mag();
                if( dist >= GetMinRange() &&
                    dist < bestDist)
                {
                    bestDist = dist;
                    m_targetId = s_neighbours[i];
                }
            }
        }
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
    if( m_target == g_zeroVector )
    {
        float angle = syncfrand( 2.0 * M_PI );
        float distance = 200.0f + syncfrand(200.0f);
        float height = syncfrand(100.0f);

        m_target = m_pos + Vector3( cos(angle) * distance,
                                    height,
                                    sin(angle) * distance );
    }
    else
    {
        Vector3 currentVector = m_target - m_pos;
        currentVector.y = 0;

        float angle = syncsfrand( 0.5 * M_PI );
        currentVector.RotateAroundY( angle );

        currentVector.y = syncsfrand(50);

        m_target = m_pos + currentVector;
    }
}


void GunTurret::RecalculateOwnership()
{
    int teamCount[NUM_TEAMS];
	bool teamPresent[NUM_TEAMS];
	bool lowestFriendlyTeam[NUM_TEAMS];
	
	for ( int i = 0; i < NUM_TEAMS; i++ )
	{
		teamPresent[i] = false;
		lowestFriendlyTeam[i] = i;
		teamCount[i] = 0;
	}

	for( int i = 0; i < GetNumPorts(); ++i )
    {
        WorldObjectId id = GetPortOccupant(i);
        if( id.IsValid() )
        {
			teamPresent[id.GetTeamId()] = true;
        }
    }
    int totalOccupiedPorts = 0;
    for( int i = 0; i < GetNumPorts(); ++i )
    {
        WorldObjectId id = GetPortOccupant(i);
        if( id.IsValid() )
        {
			for ( int j = 0; j < NUM_TEAMS; j++ )
			{
				if ( j < lowestFriendlyTeam[id.GetTeamId()] && teamPresent[j] && g_app->m_location->IsFriend(j,id.GetTeamId()) ) {
					lowestFriendlyTeam[id.GetTeamId()] = j;
				}
			}
            teamCount[lowestFriendlyTeam[id.GetTeamId()]] ++;
			totalOccupiedPorts++;
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
		if ( m_id.GetTeamId() != 255 )
		{
			SetTeamId(255);
			// Decolour
			m_turret = g_app->m_resource->GetShape( "battlecannonturret.shp", -1, false);
			m_barrel = g_app->m_resource->GetShapeOrShape( GetTurretModelName(m_state), "battlecannonbarrel.shp", -1, false );
		}
    }
    else if ( !g_app->m_location->IsFriend(m_id.GetTeamId(), winningTeam) )
    {
        SetTeamId(winningTeam);
		// Now recolour
		m_turret = g_app->m_resource->GetShape( "battlecannonturret.shp", winningTeam, false);
		m_barrel = g_app->m_resource->GetShapeOrShape( GetTurretModelName(m_state), "battlecannonbarrel.shp", winningTeam, false );
    }
}


void GunTurret::PrimaryFire()
{
    if( m_fireTimer > 0.0 ) return;

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
            Vector3 rocketTarget = shellPos + shellVel;

            Vector3 mortarTarget = m_pos + m_targetPos;
            mortarTarget.x += syncsfrand( m_targetRadius );
            mortarTarget.z += syncsfrand( m_targetRadius );
            double force = m_targetForce * 1.0 + syncsfrand(0.1);

			Vector3 fireVel = barrelMountFront.SetLength(100.0);

            //g_app->m_location->FireTurretShell( shellPos, shellVel );
            int type = m_state;
            //if( type == GunTurretTypeStandard && d && d->m_subversionTaskId != -1 ) type = GunTurretTypeSubversion;
        
            switch( type )
            {
                case GunTurretTypeStandard:     g_app->m_location->FireTurretShell( shellPos - shellVel * SERVER_ADVANCE_PERIOD * 0.5f, shellVel);   break;
                case GunTurretTypeRocket:       g_app->m_location->FireRocket( shellPos, rocketTarget, m_id.GetTeamId() );  break;
                case GunTurretTypeMortar:       g_app->m_location->ThrowWeapon( shellPos, mortarTarget, EffectThrowableGrenade, m_id.GetTeamId() );    break;
                //case GunTurretTypeFlame:        g_app->m_location->LaunchFireball( shellPos, fireVel, 1.0f, m_id.GetUniqueId());    break;
                case GunTurretTypeSubversion:   g_app->m_location->FireSubversion( shellPos, shellVel / 4.0f, m_id.GetTeamId() ); break;
                case GunTurretTypeLaser:		g_app->m_location->FireLaser( shellPos, shellVel / 4.0f, m_id.GetTeamId() ); break;
            }
			fired = true;
			m_fireTimer = GetReloadTime();
        }

        ++m_nextBarrel;
        if( m_nextBarrel >= GUNTURRET_NUMBARRELS )
        {
            m_nextBarrel = 0;
        }

    }

    if( fired )
    {
        switch( m_state )
        {
            case GunTurretTypeStandard:     g_app->m_soundSystem->TriggerBuildingEvent( this, "FireShell" );    break;
            case GunTurretTypeFlame:        g_app->m_soundSystem->TriggerBuildingEvent( this, "FireFlame" );    break;
            case GunTurretTypeMortar:       g_app->m_soundSystem->TriggerBuildingEvent( this, "FireGrenade" );    break;
            case GunTurretTypeRocket:       g_app->m_soundSystem->TriggerBuildingEvent( this, "FireRocket" );    break;                
        }
    }
}

double GunTurret::GetReloadTime()
{
    switch( m_state )
    {
        case GunTurretTypeStandard:		return 0.1;
        case GunTurretTypeRocket:		return 1.5;
        case GunTurretTypeMortar:		return 4.0;
        case GunTurretTypeFlame:		return 0.1;
        case GunTurretTypeSubversion:   return 0.25;
        case GunTurretTypeLaser:		return 0.125;
    }

    return 0.0;
}

double GunTurret::GetMinRange()
{
    switch( m_state )
    {
        case GunTurretTypeStandard:		return GUNTURRET_MINRANGE;
        case GunTurretTypeRocket:       return ROCKETTURRET_MINRANGE;
        case GunTurretTypeFlame:        return FLAMETURRET_MINRANGE;
        case GunTurretTypeSubversion:   return SUBVERSIONTURRET_MINRANGE;
        case GunTurretTypeLaser:		return LASERTURRET_MINRANGE;
    }

    return GUNTURRET_MINRANGE;
}

double GunTurret::GetMaxRange()
{
    switch( m_state )
    {
        case GunTurretTypeStandard:		return GUNTURRET_MAXRANGE;
        case GunTurretTypeRocket:       return ROCKETTURRET_MAXRANGE;
        case GunTurretTypeFlame:        return FLAMETURRET_MAXRANGE;
        case GunTurretTypeSubversion:   return SUBVERSIONTURRET_MAXRANGE;
        case GunTurretTypeLaser:		return LASERTURRET_MAXRANGE;
    }

    return GUNTURRET_MAXRANGE;
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
        if( !g_app->m_location->IsFriend(m_id.GetTeamId(),2) )
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

    if( m_fireTimer > 0.0 ) m_fireTimer -= SERVER_ADVANCE_PERIOD;
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

    if( m_id.GetTeamId() >= 0 && m_id.GetTeamId() < NUM_TEAMS )
    {
        Vector3 barrelPos = m_barrelMount->GetWorldMatrix(turretPos).pos;
        Vector3 toTarget = m_target - barrelPos;
        toTarget.HorizontalAndNormalise();
        float angle = acosf( toTarget * m_turretFront );    

        Vector3 rotation = m_turretFront ^ toTarget;
        rotation.SetLength(angle * 2.0f * _predictionTime);
        
        //predictedTurretFront.RotateAround(rotation);    
        //predictedTurretFront.Normalise();

        toTarget = (m_target - barrelPos).Normalise();
        Vector3 barrelRight = toTarget ^ g_upVector;
        Vector3 targetBarrelUp = toTarget ^ barrelRight;
        float factor = _predictionTime * 2.0f;    
                
        //predictedBarrelUp = predictedBarrelUp * (1.0f-factor) + targetBarrelUp * factor;
    }
	
    Vector3 barrelPos = m_barrelMount->GetWorldMatrix(turretPos).pos;
    Vector3 barrelFront = m_barrelMount->GetWorldMatrix(turretPos).f;
    Vector3 barrelRight = barrelFront ^ m_barrelUp;
    barrelFront = m_barrelUp ^ barrelRight;
    barrelFront.Normalise();
    Matrix34 barrelMat( barrelFront, m_barrelUp, barrelPos );
    m_barrel->Render( _predictionTime, barrelMat );

    //RenderArrow( barrelPos, barrelPos + barrelFront * 1000.0f, 1.0f );

	    Team *team = g_app->m_location->GetMyTeam();
    bool underPlayerControl = ( team && team->m_currentBuildingId == m_id.GetUniqueId() );

    if( underPlayerControl )
    {      
        float distToTarget = ( m_target - barrelPos ).Mag();
        if( distToTarget > 5000 ) distToTarget = 50000;

        Vector3 landHitPos;        
        Vector3 rayStart = barrelPos + barrelFront * 100000;
        Vector3 rayDir = -barrelFront;
        bool landHit = RaySphereIntersection( rayStart, rayDir, barrelPos, distToTarget, 1000000000, &landHitPos );

    }

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
    _list->PutData( "FireFlame" );
    _list->PutData( "FireRocket" );
    _list->PutData( "FireGrenade" );

}

void GunTurret::SetTarget( Vector3 _target )
{
    m_target = _target;
}

GunTurretTarget::GunTurretTarget( int _buildingId )
:   WorldObject(),
    m_buildingId(_buildingId)
{
    m_type = EffectGunTurretTarget;
}


char *GunTurret::GetTurretTypeName( int _type )
{
    switch( _type )
    {
        case GunTurretTypeStandard:		return "buildingname_gunturret";
        case GunTurretTypeRocket:		return "buildingname_rocketturret";
        case GunTurretTypeMortar:		return "buildingname_grenadeturret";
        case GunTurretTypeFlame:		return "buildingname_flameturret";
        case GunTurretTypeSubversion:   return "buildingname_subversionturret";
        case GunTurretTypeLaser:		return "buildingname_laserturret";
    }

    return "buildingname_gunturret";
}


char *GunTurret::GetTurretModelName( int _type )
{
    switch( _type )
    {
        case GunTurretTypeFlame:		return "cannon_flame.shp";          break;
        case GunTurretTypeRocket:		return "cannon_rocket.shp";         break;
        case GunTurretTypeSubversion:   return "battlecannonbarrel.shp";    break;
        case GunTurretTypeLaser:		return "battlecannonbarrel.shp";    break;
        default:						return "cannon_bullets.shp";        break;
    }
}

void GunTurret::Read( TextReader *_in, bool _dynamic )
{
    Building::Read( _in, _dynamic );
    if( _in->TokenAvailable() )
    {
        m_state = atoi( _in->GetNextToken() );
        if( _in->TokenAvailable() )
        {
            m_targetPos.x = atof( _in->GetNextToken() );
            m_targetPos.z = atof( _in->GetNextToken() );
            m_targetRadius = atof( _in->GetNextToken() );
            m_targetForce = atof( _in->GetNextToken() );
        }
    }
}


void GunTurret::Write( FileWriter *_out )
{
    Building::Write( _out );
    _out->printf( "%-4d", m_state );
    _out->printf( "%-2.2f ", m_targetPos.x );
    _out->printf( "%-2.2f ", m_targetPos.z );
    _out->printf( "%-2.2f ", m_targetRadius );
    _out->printf( "%-2.2f ", m_targetForce );

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

