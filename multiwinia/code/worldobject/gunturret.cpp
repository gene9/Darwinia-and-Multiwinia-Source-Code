#include "lib/universal_include.h"

#include <math.h>

#include "lib/resource.h"
#include "lib/shape.h"
#include "lib/math_utils.h"
#include "lib/text_renderer.h"
#include "lib/debug_render.h"
#include "lib/math/random_number.h"
#include "lib/input/input.h"
#include "lib/input/input_types.h"
#include "lib/language_table.h"
#include "lib/filesys/text_file_writer.h"
#include "lib/filesys/text_stream_readers.h"

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
#include "multiwinia.h"
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


void ConvertColours ( ShapeFragment *_frag, RGBAColour _col )
{
    RGBAColour *newColours = new RGBAColour[_frag->m_numColours];

    for( int i = 0; i < _frag->m_numColours; ++i )
    {
        newColours[i].r = _col.r;
        newColours[i].g = _col.g;
        newColours[i].b = _col.b;

        // scale colours down - colours on 3d models appear brighter than 2d equivalents due to lighting
        newColours[i] *= 0.2;
    }
    _frag->RegisterColours( newColours, _frag->m_numColours );


    for( int i = 0; i < _frag->m_childFragments.Size(); ++i )
    {
        ConvertColours( _frag->m_childFragments[i], _col );
    }
}

GunTurret::GunTurret()
:   Building(),
    m_retargetTimer(0.0),
    m_fireTimer(0.0),
    m_nextBarrel(0),
    m_turret(NULL),
    m_ownershipTimer(0.0),
    m_health(60.0),
    m_targetCreated(false),
    m_aiTargetCreated(false),
    m_state(0),
    m_stateSwitchTimer(0.0),
    m_aiTarget(NULL),
    m_targetRadius(0.0f),
    m_targetForce(0.0f),
    m_targetAngle(0.0f),
    m_hadTargetTimer(0.0f),
    m_temperature(0.0f),
    m_overheatTimer(-1.0f),
    m_friendlyFireWarning(false),
    m_firedLastFrame(0.0f)
{
    SetShape( g_app->m_resource->GetShape( "battlecannonbase.shp" ) );
    m_type = TypeGunTurret;

    m_turret = g_app->m_resource->GetShape( "battlecannonturret.shp" );
    m_barrel = g_app->m_resource->GetShape( GetTurretModelName(GunTurretTypeStandard) );

    const char barrelMountName[] = "MarkerBarrel";
    m_barrelMount = m_turret->m_rootFragment->LookupMarker( barrelMountName );
    AppReleaseAssert( m_barrelMount, "GunTurret: Can't get Marker(%s) from shape(%s), probably a corrupted file\n", barrelMountName, m_turret->m_name );

    const char scoreMarkerName[] = "MarkerKills";
    m_scoreMarker = m_turret->m_rootFragment->LookupMarker( scoreMarkerName );
    AppReleaseAssert( m_scoreMarker, "GunTurret: Can't get Marker(%s) from shape(%s), probably a corrupted file\n", scoreMarkerName, m_turret->m_name );

    for( int i = 0; i < GUNTURRET_NUMBARRELS; ++i )
    {
        char name[64];
        sprintf( name, "MarkerBarrelEnd0%d", i+1 );
        m_barrelEnd[i] = m_barrel->m_rootFragment->LookupMarker( name );
        AppReleaseAssert( m_barrelEnd[i], "GunTurret: Can't get Marker(%s) from shape(%s), probably a corrupted file\n", name, m_barrel->m_name );
       
        sprintf( name, "MarkerShells0%d", i+1 );
        m_shellEject[i] = m_barrel->m_rootFragment->LookupMarker( name );
    }

    for( int i = 0; i < GUNTURRET_NUMSTATUSMARKERS; ++i )
    {
        char name[64];
        sprintf( name, "MarkerStatus0%d", i+1 );
        m_statusMarkers[i] = m_shape->m_rootFragment->LookupMarker( name );
    }

    for( int i = 0; i < NUM_TEAMS; ++i )
    {
        m_kills[i] = 0;
    }
}

void GunTurret::SetupTeamShapes()
{
    if( g_app->Multiplayer() )
    {
        // set up the turret shapes for each team now
        for( int i = 0; i < MULTIWINIA_NUM_TEAMCOLOURS+2; ++i )
        {
            for( int k = 0; k < 3; ++k )
            {
                char filename[256];
                sprintf( filename, "battlecannonturret.shp_%d_%d", i, k );
                Shape *shape = g_app->m_resource->GetShape(filename, false);

                RGBAColour colour = g_app->m_multiwinia->GetColour(i);
                if( k > 0 && i < MULTIWINIA_NUM_TEAMCOLOURS) colour = g_app->m_multiwinia->GetGroupColour(k-1, i);
                if( !shape )
                {					
                    shape = g_app->m_resource->GetShapeCopy( "battlecannonturret.shp", false, false );
                    ConvertColours( shape->m_rootFragment, colour);

                    shape->BuildDisplayList();
                    g_app->m_resource->AddShape( shape, filename );
                }

                for( int j = 0; j < NumGunTurretTypes; ++j )
                {
                    sprintf( filename, "%s_%d_%d", GetTurretModelName(j), i, k );
                    shape = g_app->m_resource->GetShape(filename, false);
                    if( !shape )
                    {
                        shape = g_app->m_resource->GetShapeCopy( GetTurretModelName(j), false, false );
                        //ConvertColours( shape->m_rootFragment, colour);
                        shape->BuildDisplayList();
                        g_app->m_resource->AddShape( shape, filename );
                    }        
                }
            }
        }
    }
}

void GunTurret::Initialise( Building *_template )
{
    //_template->m_up = g_app->m_location->m_landscape.m_normalMap->GetValue( _template->m_pos.x, _template->m_pos.z );
    _template->m_up = g_upVector;
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
    m_radius = 30.0;

    SearchForRandomPos();

    m_turret = g_app->m_resource->GetShape( "battlecannonturret.shp" );
    m_barrel = g_app->m_resource->GetShape( GetTurretModelName(m_state) );
}


void GunTurret::RegisterKill( int killedTeamId, int numKills )
{
    if( m_id.GetTeamId() < 0 || m_id.GetTeamId() >= NUM_TEAMS )
    {
        return;
    }

    if( g_app->m_location->IsFriend( m_id.GetTeamId(), killedTeamId ) )
    {
        m_kills[ m_id.GetTeamId() ] -= numKills;
    }
    else
    {
        m_kills[ m_id.GetTeamId() ] +=  numKills;
    }

    g_app->m_soundSystem->TriggerBuildingEvent( this, "RegisterKill" );
}


void GunTurret::SetDetail( int _detail )
{
    Building::SetDetail( _detail );

    m_centrePos = m_pos;
    m_radius = 30.0;
}


void GunTurret::Damage( double _damage )
{
    m_health += _damage;
    
    if( m_health <= 0.0 )
    {
        ExplodeBody();
        m_destroyed = true;
        if( m_aiTarget )
        {
            m_aiTarget->m_destroyed = true;
            m_aiTarget->m_neighbours.EmptyAndDelete();
            m_aiTarget = NULL;
        }
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

bool GunTurret::ClearLOS( Vector3 const &_pos )
{
    double stepSize = 30.0;
    double dist = (m_pos - _pos).Mag();
    int numSteps = dist / stepSize;
    Vector3 diff = (_pos - m_pos) / (double)numSteps;
    Vector3 pos = _pos;

    for( int i = 0; i < numSteps; ++i )
    {
        LList<int> *buildingIds = g_app->m_location->m_obstructionGrid->GetBuildings( pos.x, pos.z );
        
        for( int j = 0; j < buildingIds->Size(); ++j )
        {
            int buildingId = buildingIds->GetData(j);
            Building *building = g_app->m_location->GetBuilding( buildingId );
            if( building )
            {
                Vector3 dir = (_pos - m_pos).Normalise();
                if( building->DoesRayHit( m_pos, dir, dist ) ) return false;
            }
        }
        pos += diff;
    }

    return true;
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
            g_app->m_location->m_entityGrid->GetEnemies( s_neighbours, m_pos.x, m_pos.z,
                                                         GetMaxRange(), &numFound,
                                                         m_id.GetTeamId() );

            double bestDist = FLT_MAX;

            for( int i = 0; i < numFound; ++i )
            {
                Entity *e = g_app->m_location->GetEntity( s_neighbours[i] );
                if( e->m_type == Entity::TypeAI ) continue;
                if( e->m_type == Entity::TypeHarvester ) continue;
                if( e->m_dead ) continue;
                double dist = (e->m_pos - m_pos).Mag();
                if( dist >= GetMinRange() &&
                    dist < bestDist)
                {
                    if( ClearLOS( e->m_pos ) )
                    {
                        bestDist = dist;
                        m_targetId = s_neighbours[i];
                    }
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
        double angle = syncfrand( 2.0 * M_PI );
        double distance = 200.0f + syncfrand(200.0f);
        double height = syncfrand(100.0f);

        m_target = m_pos + Vector3( iv_cos(angle) * distance,
                                    height,
                                    iv_sin(angle) * distance );
    }
    else
    {
        Vector3 currentVector = m_target - m_pos;
        currentVector.y = 0;

        double angle = syncsfrand( 0.5 * M_PI );
        currentVector.RotateAroundY( angle );

        currentVector.y = syncsfrand(50);

        m_target = m_pos + currentVector;
    }
}


void GunTurret::RecalculateOwnership()
{
    int teamCount[NUM_TEAMS];
    memset( teamCount, 0, NUM_TEAMS*sizeof(int));

    int totalOccupiedPorts = 0;
    for( int i = 0; i < GetNumPorts(); ++i )
    {
        WorldObjectId id = GetPortOccupant(i);
        if( id.IsValid() )
        {
            teamCount[id.GetTeamId()] ++;
            totalOccupiedPorts++;
        }
    }

    int winningTeam = -1;
    bool multipleTeams = false;
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

        if( winningTeam != -1 &&
            totalOccupiedPorts != teamCount[winningTeam] )
        {
            //winningTeam = -1;
            multipleTeams = true;
        }
    }

    if( multipleTeams )
    {
        for( int i = 0; i < NUM_TEAMS; ++i )
        {
            if( teamCount[i] > 0 && !g_app->m_location->IsFriend( i, winningTeam ) )
            {
                winningTeam = -1;
                break;
            }
        }
    }



    if( winningTeam == -1 )
    {
        if( m_id.GetTeamId() != 255 )
        {
            m_turret = g_app->m_resource->GetShape( "battlecannonturret.shp" );
            m_barrel = g_app->m_resource->GetShape( GetTurretModelName(m_state) );
        }
        SetTeamId(255);
    }
    else if( winningTeam == 0 && !g_app->Multiplayer() )
    {
        // Green own the building, but set to yellow so the player
        // can control the building
        SetTeamId(2);
    }
    else 
    {
        if( m_state == GunTurretTypeSubversion &&
            !g_app->m_location->m_teams[winningTeam]->IsFuturewinianTeam() )
        {
            Damage(-1000);
            return;
        }
        if( winningTeam == g_app->m_location->GetFuturewinianTeamId() &&
            m_state != GunTurretTypeSubversion)
        {
            Damage(-1000);
            return;
        }
        // Red own the building, or this is multiplayer
		if(g_app->Multiplayer() && m_id.GetTeamId() != winningTeam )
        {
            char filename[256];

            Team *team = g_app->m_location->m_teams[winningTeam];
            int colourId = team->m_lobbyTeam->m_colourId;
            int groupId = team->m_lobbyTeam->m_coopGroupPosition;

            if( colourId == -1 )
            {
                // just in case - this should never happen!
                sprintf( filename, "battlecannonturret.shp" );
            }
            else
            {
                sprintf( filename, "battlecannonturret.shp_%d_%d", colourId, groupId );
            }
            m_turret = g_app->m_resource->GetShape( filename );

            if( colourId == -1 )
            {
                sprintf( filename, "%s", GetTurretModelName(m_state) );
            }
            else
            {
                sprintf( filename, "%s_%d_%d", GetTurretModelName(m_state), colourId, groupId );
            }
            m_barrel = g_app->m_resource->GetShape(filename );
        }
        SetTeamId(winningTeam);
    }
}


void GunTurret::PrimaryFire()
{
    if( m_fireTimer > 0.0 ) return;


	// Do this based on distance from camera

    bool fired = false;

    for( int i = 0; i < 2; ++i )
    {
        if( GetPortOccupant(m_nextBarrel).IsValid() )
        {
            Darwinian *d = (Darwinian *)g_app->m_location->GetEntitySafe( GetPortOccupant(m_nextBarrel), Entity::TypeDarwinian );

            Matrix34 turretMat( m_turretFront, g_upVector, m_pos );
            Vector3 barrelMountPos = m_barrelMount->GetWorldMatrix(turretMat).pos;
            Vector3 barrelMountFront = m_barrelMount->GetWorldMatrix(turretMat).f;
            Vector3 barrelRight = barrelMountFront ^ m_barrelUp;
            barrelMountFront = m_barrelUp ^ barrelRight;
            Matrix34 barrelMountMat( barrelMountFront, m_barrelUp, barrelMountPos );
            Matrix34 barrelMat = m_barrelEnd[m_nextBarrel]->GetWorldMatrix(barrelMountMat);
            
            Vector3 shellPos = barrelMat.pos;
            Vector3 shellVel = barrelMountFront.SetLength(500.0);
            Vector3 rocketTarget = shellPos + shellVel;
 
            Vector3 direction = barrelMountFront.Normalise();
            direction.Normalise();

            Vector3 mortarTarget = m_pos + m_targetPos;
            mortarTarget.x += syncsfrand( m_targetRadius );
            mortarTarget.z += syncsfrand( m_targetRadius );
            double force = m_targetForce * 1.0 + syncsfrand(0.1);

            Vector3 fireVel = barrelMountFront.SetLength(100.0);

            int type = m_state;
            if( type == GunTurretTypeStandard && d && d->m_subversionTaskId != -1 ) type = GunTurretTypeSubversion;
        
            switch( type )
            {
                case GunTurretTypeStandard:     g_app->m_location->FireTurretShell( shellPos - shellVel * SERVER_ADVANCE_PERIOD * 0.5f, shellVel, m_id.GetTeamId(), m_id.GetUniqueId() );   break;
                case GunTurretTypeRocket:       g_app->m_location->FireRocket( shellPos, rocketTarget, m_id.GetTeamId(), 1.0f, m_id.GetUniqueId() );  break;
                case GunTurretTypeMortar:       g_app->m_location->ThrowWeapon( shellPos, mortarTarget, EffectThrowableGrenade, m_id.GetTeamId(), force, 1000.0f, true );    break;
                case GunTurretTypeFlame:        g_app->m_location->LaunchFireball( shellPos, fireVel, 1.0f, m_id.GetUniqueId());    break;
                case GunTurretTypeSubversion:   g_app->m_location->FireLaser( shellPos, shellVel / 4.0f, m_id.GetTeamId(), true );
            }

            //
            // Eject cartridge

            if( m_shellEject[m_nextBarrel] )
            {
                Matrix34 shellMat = m_shellEject[m_nextBarrel]->GetWorldMatrix( barrelMountMat );

                Vector3 vel = shellMat.f;
                vel.y += 1.0f;
                vel += Vector3( SFRAND(0.5), SFRAND(0.5), SFRAND(0.5) );
                vel.SetLength( 20 + syncfrand(20) );
                Vector3 pos = shellMat.pos;
                if( m_nextBarrel == 0 || m_nextBarrel == 1 ) 
                {
                    pos -= shellMat.u * 2.5;
                }
                else
                {
                    pos += shellMat.u * 2.5;
                }

                if( type == GunTurret::GunTurretTypeStandard )
                {
                    TurretEmptyShell *shell = new TurretEmptyShell( pos, -shellMat.r, vel );
                    int weaponId = g_app->m_location->m_effects.PutData( shell );
                    shell->m_id.Set( m_id.GetTeamId(), UNIT_EFFECTS, weaponId, -1 );
                    shell->m_id.GenerateUniqueId();
                }
            }


            fired = true;
            m_fireTimer = GetReloadTime();
        }
        
        ++m_nextBarrel;
        if( m_nextBarrel >= GUNTURRET_NUMBARRELS ) 
        {
            m_nextBarrel = 0;
        }

        if( m_state == GunTurretTypeRocket ) break;

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

        m_overheatTimer = g_gameTimer.GetGameTime();
        m_temperature += 1.0;
        if( m_temperature > 40 ) m_temperature = 40;
        m_firedLastFrame = SERVER_ADVANCE_PERIOD;
    }
}

double GunTurret::GetReloadTime()
{
    switch( m_state )
    {
        case GunTurretTypeStandard: return 0.1;
        case GunTurretTypeRocket:   return 1.5;
        case GunTurretTypeMortar:   return 4.0;
        case GunTurretTypeFlame:    return 0.1;
        case GunTurretTypeSubversion:   return 0.5;
    }

    return 0.0;
}

double GunTurret::GetMinRange()
{
    switch( m_state )
    {
        case GunTurretTypeStandard:
            if( g_app->Multiplayer() )  return GUNTURRET_MINRANGE * 0.5;
            else                        return GUNTURRET_MINRANGE;

        case GunTurretTypeRocket:       return ROCKETTURRET_MINRANGE;
        case GunTurretTypeFlame:        return FLAMETURRET_MINRANGE;
        case GunTurretTypeSubversion:   return SUBVERSIONTURRET_MINRANGE;
    }

    return GUNTURRET_MINRANGE;
}

double GunTurret::GetMaxRange()
{
    switch( m_state )
    {
        case GunTurretTypeStandard:
            if( g_app->Multiplayer() )  return GUNTURRET_MAXRANGE * 0.75;
            else                        return GUNTURRET_MAXRANGE;

        case GunTurretTypeRocket:       return ROCKETTURRET_MAXRANGE;
        case GunTurretTypeFlame:        return FLAMETURRET_MAXRANGE;
        case GunTurretTypeSubversion:   return SUBVERSIONTURRET_MAXRANGE;
    }

    return GUNTURRET_MAXRANGE;
}

bool GunTurret::Advance()
{
    if( m_health <= 0.0 ) return true;

    //
    // Create an AI target on top of us, 
    // to get Darwinians to fight hard for control

    if( !m_aiTarget && !m_destroyed )
    {
        m_aiTarget = (AITarget *)Building::CreateBuilding( Building::TypeAITarget );

        Matrix34 mat( m_front, g_upVector, m_pos );

        AITarget targetTemplate;
        targetTemplate.m_pos       = m_pos;
        targetTemplate.m_pos.y     = g_app->m_location->m_landscape.m_heightMap->GetValue(m_pos.x, m_pos.z);

        m_aiTarget->Initialise( (Building *)&targetTemplate );
        m_aiTarget->m_turretTarget = true;
        m_aiTarget->m_id.SetUnitId( UNIT_BUILDINGS );
        m_aiTarget->m_id.SetUniqueId( g_app->m_globalWorld->GenerateBuildingId() );   

        g_app->m_location->m_buildings.PutData( m_aiTarget );

        g_app->m_location->RecalculateAITargets();
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
    if( m_ownershipTimer <= 0.0 )
    {
        RecalculateOwnership();
        m_ownershipTimer = GUNTURRET_OWNERSHIPTIMER;
    }


    Team *team = NULL;
    if( m_id.GetTeamId() != 255 )
    {
        team = g_app->m_location->m_teams[m_id.GetTeamId()];
    }
    bool underPlayerControl = false;
    if( team )
    {
        underPlayerControl = ( team && team->m_currentBuildingId == m_id.GetUniqueId() );
    }

      
    //
    // Look for a new target
    
    bool primaryFire = false;
    bool secondaryFire = false;

    //
    // If we had a target as recently as 1 second ago, and we're a normal gun turret,
    // continue firing because we'll probably hit something,
    // and will look totally bitchin' even if we dont.

    if( g_gameTimer.GetGameTime() < m_hadTargetTimer + 1.0f &&
        !underPlayerControl )
    {
        primaryFire = true;
    }

    
    //
    // If we are a flame turret, look out for friendly fire

    m_friendlyFireWarning = false;

    if( m_state == GunTurretTypeFlame && team )
    {
        float range = 20;
        Vector3 testPosition = m_pos + m_turretFront * 60;    
        int numFriends = g_app->m_location->m_entityGrid->GetNumFriends( testPosition.x, testPosition.z, range, m_id.GetTeamId() );
        if( numFriends > 10 ) m_friendlyFireWarning = true;
    }


    if( underPlayerControl )
    {
        m_hadTargetTimer = 0.0f;

        if( !g_app->Multiplayer() )
        {
            if( m_id.GetTeamId() != 2 )
            {
                // Player has lost control of the building
                team->SelectUnit( -1, -1, -1 );
                g_app->m_camera->RequestMode( Camera::ModeFreeMovement );
                return Building::Advance();
            }

           /* m_target = g_app->m_userInput->GetMousePos3d();
            primaryFire = g_inputManager->controlEvent( ControlUnitPrimaryFireTarget ) || g_inputManager->controlEvent( ControlUnitStartSecondaryFireDirected );
            secondaryFire = g_inputManager->controlEvent( ControlUnitSecondaryFireTarget );*/
        }
    }
    else
    {
        m_retargetTimer -= SERVER_ADVANCE_PERIOD;
        if( m_retargetTimer <= 0.0 )
        {
            bool targetFound = SearchForTargets();
            if( !targetFound && m_id.GetTeamId() != 255 ) SearchForRandomPos();
            m_retargetTimer = GUNTURRET_RETARGETTIMER;
            if( g_app->Multiplayer() ) m_retargetTimer = GUNTURRET_MULTIWINIA_RETARGETTIMER;
            if( m_state == GunTurretTypeMortar ) m_target = m_pos + m_targetPos;
        }
    
        if( m_targetId.IsValid() )
        {
            m_hadTargetTimer = g_gameTimer.GetGameTime();

            // Check our target is still alive
            Entity *target = g_app->m_location->GetEntity( m_targetId );
            if( !target )
            {
                m_targetId.SetInvalid();                    
            }
            else
            {
                m_target = target->m_pos;
                double theTime = GetNetworkTime();
                m_target += Vector3( iv_sin(theTime*3)*20, iv_abs(iv_sin(theTime*2)*10), iv_cos(theTime*3)*20 );

                Vector3 targetFront = ( m_target - m_pos );
                targetFront.HorizontalAndNormalise();
                double angle = iv_acos( targetFront * m_turretFront );
                
                primaryFire = ( angle < 0.3 );            
                if( m_friendlyFireWarning ) primaryFire = false;
            }
        }

        // standard turrets should never stop firing if there are enemies nearby
        if( g_app->Multiplayer() )
        {
            // something wrong the the entity grid? this causes turrets to fire nonstop forever once an enemy comes within range
            /*if( m_state == GunTurretTypeStandard &&
                g_app->m_location->m_entityGrid->AreEnemiesPresent( m_pos.x, m_pos.z, GetMaxRange(), m_id.GetTeamId() ) )
            {
                primaryFire = true;
            }*/
        }
    }

    
    double distance = ( m_target - m_pos ).Mag();
    if( distance < 75.0 ) 
    {
        m_target = m_pos + ( m_target - m_pos ).SetLength(75.0);
    }
    

    //
    // Face our target
    // Open fire

    if( m_id.GetTeamId() >= 0 && m_id.GetTeamId() < NUM_TEAMS )
    {
        Matrix34 turretPos( m_turretFront, m_up, m_pos );
        Vector3 barrelPos = m_barrelMount->GetWorldMatrix(turretPos).pos;
	    Vector3 toTarget = m_target - barrelPos;
	    toTarget.HorizontalAndNormalise();
        double v = toTarget * m_turretFront;

        // due to some mathematical fluke, this value will sometimes be outside the valid range for acos
        // when this happens, angle becomes invalid and the turret flips around 
        // clamp the values to be sure they are within the valid range
        clamp( v, -1.0, 1.0 );
        
        double angle = iv_acos( v );
        
        Vector3 rotation = m_turretFront ^ toTarget;
        rotation.SetLength(angle * 0.2);
        m_turretFront.RotateAround(rotation);    
        m_turretFront.Normalise();
        
        toTarget = (m_target - barrelPos).Normalise();
        Vector3 barrelRight = toTarget ^ g_upVector;
        Vector3 targetBarrelUp = toTarget ^ barrelRight;
        double factor = SERVER_ADVANCE_PERIOD * 2.0;    
        m_barrelUp = m_barrelUp * (1.0-factor) + targetBarrelUp * factor;

        if( m_fireTimer > 0.0 ) m_fireTimer -= SERVER_ADVANCE_PERIOD;
        if( primaryFire ) PrimaryFire();

    }

    //
    // Smoke if overheating

    if( m_overheatTimer > 0.0 )
    {
        double timeSinceLastFire = g_gameTimer.GetGameTime() - m_overheatTimer;
        if( timeSinceLastFire > 0.2 )
        {
            m_overheatTimer = g_gameTimer.GetGameTime();
            m_temperature -= 0.1 * 3;
            if( m_temperature < 0.0 )
            {
                m_temperature = 0.0;
                m_overheatTimer = -1.0;
            }

            if( m_temperature > 0.0 )
            {
                for( int i = 0; i < 4; ++i )
                {
                    Matrix34 turretMat( m_turretFront, g_upVector, m_pos );
                    Vector3 barrelMountPos = m_barrelMount->GetWorldMatrix(turretMat).pos;
                    Vector3 barrelMountFront = m_barrelMount->GetWorldMatrix(turretMat).f;
                    Vector3 barrelRight = barrelMountFront ^ m_barrelUp;
                    barrelMountFront = m_barrelUp ^ barrelRight;
                    Matrix34 barrelMountMat( barrelMountFront, m_barrelUp, barrelMountPos );
                    Matrix34 barrelMat = m_barrelEnd[i]->GetWorldMatrix(barrelMountMat);

                    Vector3 smokePos = barrelMat.pos + barrelMat.f;
                    if( m_state == GunTurretTypeFlame ) smokePos += barrelMat.f * 4;

                    int numParticles = syncfrand( int( 1.0 + m_temperature / 10.0 ) );
                    
                    for( int i = 0; i < numParticles; ++i )
                    {
                        Vector3 vel = g_upVector * 1.5 + barrelMat.f * 1;
                        vel += Vector3( SFRAND(0.3), SFRAND(0.3), SFRAND(0.3) );
                        vel.SetLength(5 + syncfrand(5));
                        double size = 20 + syncfrand(30);
                        g_app->m_particleSystem->CreateParticle( smokePos, vel, Particle::TypeMissileTrail, size );
                    }
                }
            }
        }
    }

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


void GunTurret::Render( double _predictionTime )
{
    if( g_app->m_editing )
    {
        m_turretFront = m_front;
    }


    //
    // Calculate our predicted orientation

    Matrix34 turretPos( m_turretFront, g_upVector, m_pos );
    Vector3 predictedTurretFront = m_turretFront;
    Vector3 predictedBarrelUp = m_barrelUp; 

    if( m_id.GetTeamId() >= 0 && m_id.GetTeamId() < NUM_TEAMS )
    {
        Vector3 barrelPos = m_barrelMount->GetWorldMatrix(turretPos).pos;
        Vector3 toTarget = m_target - barrelPos;
        toTarget.HorizontalAndNormalise();
        float angle = acosf( toTarget * m_turretFront );    

        Vector3 rotation = m_turretFront ^ toTarget;
        rotation.SetLength(angle * 2.0f * _predictionTime);
        
        predictedTurretFront.RotateAround(rotation);    
        predictedTurretFront.Normalise();

        toTarget = (m_target - barrelPos).Normalise();
        Vector3 barrelRight = toTarget ^ g_upVector;
        Vector3 targetBarrelUp = toTarget ^ barrelRight;
        float factor = _predictionTime * 2.0f;    
                
        predictedBarrelUp = predictedBarrelUp * (1.0f-factor) + targetBarrelUp * factor;
    }

    predictedBarrelUp *= -1;

    Matrix34 predictedTurretPos( predictedTurretFront, g_upVector, m_pos );
    m_turret->Render( _predictionTime, predictedTurretPos );

    Vector3 barrelPos = m_barrelMount->GetWorldMatrix(predictedTurretPos).pos;
    Vector3 barrelFront = m_barrelMount->GetWorldMatrix(predictedTurretPos).f;
    Vector3 barrelRight = barrelFront ^ predictedBarrelUp;
    barrelFront = predictedBarrelUp ^ barrelRight;
    barrelFront.Normalise();

    /*if( m_state == GunTurretTypeStandard ||
        m_state == GunTurretTypeRocket )
    {
        if( m_firedLastFrame > 0.0f ) barrelPos -= barrelFront * m_firedLastFrame * 10.0f;
        m_firedLastFrame -= g_advanceTime;
        if( m_firedLastFrame < 0.0f ) m_firedLastFrame = 0.0f;
    }*/

    Matrix34 barrelMat( barrelFront, predictedBarrelUp, barrelPos );    
    m_barrel->Render( _predictionTime, barrelMat );
    
    //RenderSphere( m_centrePos, m_radius * 0.75f );

    //
    // Render laser targets for easier aiming

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

        if( landHit )
        {
            m_actualTargetPos = landHitPos;
        }
        else
        {
            m_actualTargetPos = g_zeroVector;
        }
    }

    // 
    // Render targetting crosshair

/*
    Team *team = g_app->m_location->GetMyTeam();
    bool underPlayerControl = ( team && team->m_currentBuildingId == m_id.GetUniqueId() );

    if( underPlayerControl )
    {
        Vector3 targetPos = barrelPos + Vector3(0,10,0) + barrelFront.SetLength( 300.0 );
        double size = 20.0;
        Vector3 camUp = g_app->m_camera->GetUp();
        Vector3 camRight = g_app->m_camera->GetRight();
        targetPos += camUp * 5.0;

        glBindTexture( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "icons/mouse_missiletarget.bmp" ) );
        glEnable( GL_TEXTURE_2D );
        glDisable( GL_CULL_FACE );
        glBlendFunc( GL_SRC_ALPHA, GL_ONE );
        glEnable( GL_BLEND );
        glDepthMask( false );
        glDisable( GL_DEPTH_TEST );

        glBegin( GL_QUADS );
            glTexCoord2i(0,0);      glVertex3dv( (targetPos - camRight * size - camUp * size).GetData() );
            glTexCoord2i(1,0);      glVertex3dv( (targetPos + camRight * size - camUp * size).GetData() );
            glTexCoord2i(1,1);      glVertex3dv( (targetPos + camRight * size + camUp * size).GetData() );
            glTexCoord2i(0,1);      glVertex3dv( (targetPos - camRight * size + camUp * size).GetData() );
        glEnd();

        glEnable( GL_DEPTH_TEST );
        glDepthMask( true );
        glDisable( GL_BLEND );
        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        glEnable( GL_CULL_FACE );
        glDisable( GL_TEXTURE_2D );
    }*/

    //
    // Render kills

    if( m_id.GetTeamId() >= 0 && m_id.GetTeamId() < NUM_TEAMS )
    {
        int kills = m_kills[m_id.GetTeamId()];
        float size = 2.0f;
        if( kills > 999 ) size = 1.5f;

        char caption[256];
        if( m_friendlyFireWarning && fmodf( g_gameTime, 0.6f ) < 0.3f )     sprintf( caption, "FF!" );        
        else                                                                sprintf( caption, "%d", kills );        

        Matrix34 counterMat = m_scoreMarker->GetWorldMatrix(predictedTurretPos);
        counterMat.pos += counterMat.f * 0.6f;
        glColor4f( 1.0f, 1.0f, 1.0f, 0.0f );

        g_gameFont.SetRenderShadow(true);
        g_gameFont.DrawText3D( counterMat.pos, counterMat.f, counterMat.u, size, caption );
        
        counterMat.pos += counterMat.f * 0.01f;
        counterMat.pos += ( counterMat.f ^ counterMat.u ) * 0.2f;
        RGBAColour col =g_app->m_location->m_teams[m_id.GetTeamId()]->m_colour;        
        
        col.a = 180;
        glColor4ubv( col.GetData() );

        g_gameFont.SetRenderShadow(false);
        g_gameFont.DrawText3D( counterMat.pos, counterMat.f, counterMat.u, size, caption );
    }

}

void GunTurret::RenderAlphas( double _predictionTime )
{        
    if( !g_app->m_editing ) return;
#ifdef LOCATION_EDITOR
    if( g_app->m_locationEditor->m_mode == LocationEditor::ModeBuilding &&
        g_app->m_locationEditor->m_selectionId == m_id.GetUniqueId() )
    {
        if( m_state == GunTurretTypeMortar )
        {
            if( m_targetPos.Mag() > 0.0 || m_targetRadius > 0.0 )
            {
                Vector3 triggerPos = m_pos + m_targetPos;
                int numSteps = 20;
                glBegin( GL_LINE_LOOP );
                glLineWidth( 1.0 );
                glColor4f( 1.0, 0.0, 0.0, 1.0 );
                for( int i = 0; i < numSteps; ++i )
                {
                    double angle = 2.0 * M_PI * (double)i / (double) numSteps;
                    Vector3 thisPos = triggerPos + Vector3( iv_sin(angle)*m_targetRadius, 0.0,
                                                            iv_cos(angle)*m_targetRadius );
                    thisPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( thisPos.x, thisPos.z );
                    thisPos.y += 10.0;
                    glVertex3dv( thisPos.GetData() );
                }
                glEnd();
            }

            if( m_targetForce > 0.0 )
            {            
                Vector3 target = m_pos + m_targetPos;                

                Vector3 pos = m_pos;
                pos.y += 5.0;
                Vector3 front = (target-m_pos).Normalise();
                front.y = 1.0;
                front.Normalise();
                Grenade grenade( pos, front, m_targetForce );
                grenade.m_life = 1000.0;
                grenade.m_explodeOnImpact = true;
                glColor4f( 1.0, 1.0, 1.0, 1.0 );
                glLineWidth( 2.0 );
                glBegin( GL_LINES );
                while( true )
                {
                    glVertex3dv( grenade.m_pos.GetData() );
                    bool dead = grenade.Advance();
                    glVertex3dv( grenade.m_pos.GetData() );
                
                    if( dead ) break;
                }
                glEnd();
            }
        }
    }
#endif
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

		if( !m_statusMarkers[i] ){
            char name[64];
            sprintf( name, "MarkerStatus0%d", i+1 );
            AppReleaseAssert( m_statusMarkers[i], "GunTurret: Can't get Marker(%s) from shape(%s), probably a corrupted file\n", name, m_shape->m_name );
		}

        Matrix34 worldMat = m_statusMarkers[i]->GetWorldMatrix(rootMat);
        
        //
        // Render the status light

        double size = 6.0;
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
            RGBAColour teamColour = g_app->m_location->m_teams[occupantId.GetTeamId()]->m_colour;
            glColor4ubv( teamColour.GetData() );
        }
        
        glBegin( GL_QUADS );
            glTexCoord2i( 0, 0 );           glVertex3dv( (statusPos - camR - camU).GetData() );
            glTexCoord2i( 1, 0 );           glVertex3dv( (statusPos + camR - camU).GetData() );
            glTexCoord2i( 1, 1 );           glVertex3dv( (statusPos + camR + camU).GetData() );
            glTexCoord2i( 0, 1 );           glVertex3dv( (statusPos - camR + camU).GetData() );
        glEnd();
    }

    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glDisable       ( GL_BLEND );
    glDepthMask     ( true );
    glDisable       ( GL_TEXTURE_2D );
    glEnable        ( GL_CULL_FACE );
}

bool GunTurret::DoesRayHit(Vector3 const &_rayStart, Vector3 const &_rayDir, 
                          double _rayLen, Vector3 *_pos, Vector3 *norm )
{
    if( g_app->m_editing )
    {
        return RaySphereIntersection(_rayStart, _rayDir, m_pos, m_radius, _rayLen);
    }
    else
    {        
        //
        // Do we hit the main turret?

        Matrix34 turretMat( m_turretFront, g_upVector, m_pos );                
        RayPackage ray(_rayStart, _rayDir, _rayLen);       
        if( m_turret->RayHit(&ray, turretMat, true) ) return true;

        //
        // Do we hit the barrels?

        Vector3 predictedBarrelUp = m_barrelUp; 
        Vector3 barrelPos = m_barrelMount->GetWorldMatrix(turretMat).pos;
        Vector3 barrelFront = m_barrelMount->GetWorldMatrix(turretMat).f;
        Vector3 barrelRight = barrelFront ^ predictedBarrelUp;
        barrelFront = predictedBarrelUp ^ barrelRight;
        barrelFront.Normalise();
        Matrix34 barrelMat( barrelFront, predictedBarrelUp, barrelPos );    
        if( m_barrel->RayHit(&ray, barrelMat, true) ) return true;


        //
        // The area under the turret where the darwinians stand

        if( RaySphereIntersection( _rayStart, _rayDir, m_pos + g_upVector * 10, 12 ) )
        {
            return true;
        }


        //
        // Thats it

        return false;
    }
}


void GunTurret::ListSoundEvents( LList<char *> *_list )
{
    Building::ListSoundEvents( _list );

    _list->PutData( "TargetSighted" );
    _list->PutData( "RegisteredKill" );

    _list->PutData( "FireShell" );
    _list->PutData( "FireFlame" );
    _list->PutData( "FireRocket" );
    _list->PutData( "FireGrenade" );
}

void GunTurret::SetTarget( Vector3 _target )
{
    m_target = _target;
}

char *GunTurret::GetTurretTypeName( int _type )
{
    switch( _type )
    {
        case GunTurretTypeStandard: return "buildingname_gunturret";
        case GunTurretTypeRocket:   return "buildingname_rocketturret";
        case GunTurretTypeMortar:   return "buildingname_grenadeturret";
        case GunTurretTypeFlame:    return "buildingname_flameturret";
        case GunTurretTypeSubversion:   return "buildingname_subversionturret";
    }

    return "buildingname_gunturret";
}


char *GunTurret::GetTurretModelName( int _type )
{
    if( g_app->Multiplayer() )
    {
        switch( _type )
        {
            case GunTurretTypeFlame:    return "cannon_flame.shp";          break;
            case GunTurretTypeRocket:   return "cannon_rocket.shp";         break;
            case GunTurretTypeSubversion:   return "battlecannonbarrel.shp";    break;
            default:                    return "cannon_bullets.shp";        break;
        }
    }
    else
    {
        return "battlecannonbarrel.shp";
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
            m_targetPos.x = iv_atof( _in->GetNextToken() );
            m_targetPos.z = iv_atof( _in->GetNextToken() );
            m_targetRadius = iv_atof( _in->GetNextToken() );
            m_targetForce = iv_atof( _in->GetNextToken() );
        }
    }
}


void GunTurret::Write( TextWriter *_out )
{
    Building::Write( _out );
    _out->printf( "%-4d", m_state );
    _out->printf( "%-2.2f ", m_targetPos.x );
    _out->printf( "%-2.2f ", m_targetPos.z );
    _out->printf( "%-2.2f ", m_targetRadius );
    _out->printf( "%-2.2f ", m_targetForce );

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

    if( g_app->Multiplayer() )
    {
        if( turret->m_state == GunTurret::GunTurretTypeStandard )
        {
            m_id.SetTeamId( turret->m_id.GetTeamId() );
        }
    }

    return false;
}


void GunTurretTarget::Render( double _time )
{
    //RenderSphere( m_pos, 10.0 );
}
