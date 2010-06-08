#include "lib/universal_include.h"

#include "lib/preferences.h"
#include "lib/math/random_number.h"
#include "lib/resource.h"
#include "lib/shape.h"
#include "lib/bitmap.h"
#include "lib/filesys/binary_stream_readers.h"
#include "lib/language_table.h"

#include "app.h"
#include "camera.h"
#include "entity_grid.h"
#include "explosion.h"
#include "global_world.h"
#include "level_file.h"
#include "location.h"
#include "main.h"
#include "obstruction_grid.h"
#include "resource.h"
#include "team.h"
#include "markersystem.h"
#include "multiwinia.h"
#include "multiwiniahelp.h"
#include "particle_system.h"

#include "sound/soundsystem.h"

#include "worldobject/ai.h"
#include "worldobject/anthill.h"
#include "worldobject/crate.h"
#include "worldobject/darwinian.h"
#include "worldobject/meteor.h"
#include "worldobject/nuke.h"
#include "worldobject/spam.h"
#include "worldobject/tree.h"
#include "worldobject/triffid.h"
#include "worldobject/gunturret.h"
#include "worldobject/randomiser.h"
#include "worldobject/spawnpoint.h"
#include "worldobject/portal.h"

int Crate::s_crates = 0;
bool Crate::s_spaceShipUsed = false;

#define CRATE_CAPTURE_TIME 2.5

Crate::Crate()
:   Building(),
    m_recountTimer(1.0),
    m_life(60.0),
    m_bubbleTimer(100.0),
    m_captureTimer(0.0),
    m_aiTarget(NULL),
    m_rewardId(-1),
    m_percentCapturedSmooth(0.0)
{
    m_type = Building::TypeCrate;
    SetShape( g_app->m_resource->GetShape("researchitem.shp") );

    s_crates++;
    SyncRandLog( "Static crate count = %d", s_crates );
}

Crate::~Crate()
{
    s_crates--;

    SyncRandLog( "Static crate count = %d", s_crates );
}

void Crate::Initialise( Building *_template )
{
    Building::Initialise( _template );

    m_pos.y = _template->m_pos.y;
    m_onGround = false;

    g_app->m_soundSystem->TriggerBuildingEvent( this, "Falling" );
}

void Crate::SetDetail( int _detail )
{
    Vector3 pos = m_pos;
    Building::SetDetail( _detail );
    m_pos = pos;

    Matrix34 mat( m_front, m_up, m_pos );
    m_centrePos = m_shape->CalculateCentre( mat );        
    m_radius = m_shape->CalculateRadius( mat, m_centrePos );
}

bool Crate::Advance()
{
    if( m_onGround )
    {
        AdvanceOnGround();
    }
    else
    {
        AdvanceFloating();
    }

    return Building::Advance();
}

void Crate::AdvanceFloating()
{
	double groundHeight = g_app->m_location->m_landscape.m_heightMap->GetValue( m_pos.x, m_pos.z );
	groundHeight += 20;

    if( m_pos.y <= groundHeight  )
    {
        // we've landed
        m_vel.Zero();
        m_onGround = true;

        g_app->m_soundSystem->StopAllSounds( m_id, "Crate Falling" );
        g_app->m_soundSystem->TriggerBuildingEvent( this, "Land" );
        
        return;
    }

    double timeIndex = GetNetworkTime() + m_id.GetUniqueId() * 20;

    Vector3 oldPos = m_pos;

    double speed = 40.0;

    double swayDistance = 300.0;
    swayDistance *= ( m_pos.y / CRATE_START_HEIGHT );

    Vector3 targetPos = m_pos;
    targetPos.x += iv_cos( timeIndex ) * swayDistance;
    targetPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( targetPos.x, targetPos.z );
    targetPos = PushFromObstructions ( targetPos );

    Vector3 targetDir = (targetPos - m_pos).Normalise();

    Vector3 newPos = m_pos + targetDir * speed;
    Vector3 targetVel = (newPos - m_pos).SetLength( speed );

    newPos = PushFromObstructions( newPos );

    Vector3 moved = newPos - oldPos;
    if( moved.Mag() > speed * SERVER_ADVANCE_PERIOD ) moved.SetLength( speed * SERVER_ADVANCE_PERIOD );
    newPos = m_pos + moved;

    m_pos = newPos;
    m_vel = (m_pos - oldPos) / SERVER_ADVANCE_PERIOD;

    Matrix34 mat( m_front, m_up, m_pos );
    m_centrePos = m_shape->CalculateCentre( mat );  
}

static void colourToInt( unsigned _r, unsigned _g, unsigned _b, unsigned _a, unsigned *_colour )
{
    _r <<= 24;
    _g <<= 16;
    _b <<= 8;
    *_colour |= _r;
	*_colour |= _g;
	*_colour |= _b;
	*_colour |= _a;
}

static int getNextTeam( int _teamId, int *_teams )
{
    int current = _teamId;
    while( true )
    {
        current++;
        if( current >= NUM_TEAMS ) current = 0;
        if( current == _teamId ) return g_app->m_location->GetMonsterTeamId();  // we've looped back around, only 1 team present
        if( _teams[current] == 1 ) return current;
    }
}


bool Crate::IsBasicCrate( int _type )
{
    return( _type == CrateRewardSquad ||
            _type == CrateRewardEngineer ||
            _type == CrateRewardArmour ||
            _type == CrateRewardAirstrike ||
            _type == CrateRewardDarwinians ||
            _type == CrateRewardGunTurret ||
            _type == CrateRewardForest );
}

bool Crate::IsAdvancedCrate ( int _type )
{
    return( !IsBasicCrate(_type) );
}


bool Crate::IsValid( int _type )
{
#ifndef INCLUDE_CRATES_ADVANCED
    if( IsAdvancedCrate(_type) )
    {
        return false;
    }
#endif

    if( g_app->m_multiwinia->m_basicCratesOnly &&
        IsAdvancedCrate( _type ) )
    {
        return false;
    }

    if( _type == Crate::CrateRewardHarvester ||
        _type == Crate::CrateRewardEngineer ||
        _type == Crate::CrateRewardSpawnMania )
    {
        // these 3 rewards are useless on levels with no spawn points, so make sure there are spawn points before generating them
        int totalSpawnPoints = 0;
        for( int i = 0; i < NUM_TEAMS + 1; ++i )
        {
            totalSpawnPoints += SpawnPoint::s_numSpawnPoints[i];
        }
        if( totalSpawnPoints == 0 ) return false;
    }

    bool armourReinforcements = (g_app->m_multiwinia->m_gameType == Multiwinia::GameTypeRocketRiot ||
        g_app->m_location->m_levelFile->m_levelOptions[LevelFile::LevelOptionTrunkPortArmour] > 0 );

    if( armourReinforcements &&
        _type == CrateRewardArmour )    
        return false;

/*    if( !armourReinforcements && _type == CrateRewardExtinguisher )
        return false;*/

    if( g_app->m_multiwinia->m_gameType == Multiwinia::GameTypeAssault )
    {
        if( _type == CrateRewardRocketTurret ||
            _type == CrateRewardSpaceShip )
        {
            return false;
        }
    }

    if( s_spaceShipUsed && _type == CrateRewardSpaceShip ) return false;

    if( _type == CrateRewardCrateMania && s_crates >= 4 ) return false;

    if( _type == CrateRewardAntNest )
    {
        int numAntsNests = 0;
        for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
        {
            if( g_app->m_location->m_buildings.ValidIndex( i ) )
            {
                Building *b = g_app->m_location->m_buildings[i];
                if( b->m_type == Building::TypeAntHill )
                {
                    numAntsNests++;
                }
            }
        }

        for( int i = 0; i < NUM_TEAMS; ++i )
        {
            if( g_app->m_location->m_teams[i] == NULL ||
				g_app->m_location->m_teams[i]->m_teamType == TeamTypeUnused ) continue;

            numAntsNests += g_app->m_location->m_teams[i]->m_taskManager->CountNumTasks( GlobalResearch::TypeAntsNest ) * 2;
        }

        if( numAntsNests >= 4 ) return false;
    }

    return !g_app->m_location->m_levelFile->m_invalidCrates[_type];
}

void Crate::CaptureCrate()
{
    int rewardId = syncrand() % NumCrateRewards;

    while(true)
    {
        if( rewardId != CrateRewardSpam &&
            rewardId != CrateRewardRandomiser &&
            rewardId != CrateRewardPlague &&
            rewardId != CrateRewardSpaceShip &&
            rewardId != CrateRewardTriffids &&
#ifdef INCLUDE_THEBEAST
            rewardId != CrateRewardTheBeast &&
#endif
            IsValid( rewardId ) )
        {
            break;
        }
        rewardId = syncrand() % NumCrateRewards;
    }
    
    
    //
    // Code for picking specific crate types
    // TAG REMOVE ME
    //{
    //    LList<int> crateTypes;
    //    crateTypes.PutData( CrateRewardAntNest );
    //    crateTypes.PutData( CrateRewardNuke );
    //    crateTypes.PutData( CrateRewardGunTurret );
    //    crateTypes.PutData( CrateRewardNapalmStrike );

    //    int chosenIndex = syncrand() % crateTypes.Size();
    //    rewardId = crateTypes[ chosenIndex ];
    //}

	if( m_rewardId != -1 ) rewardId = m_rewardId;

    Team *team = g_app->m_location->m_teams[m_id.GetTeamId()];

    int monsterTeam = g_app->m_location->GetMonsterTeamId();

    bool messageShown = false;
    
    switch( rewardId )
    {
        case CrateRewardSquad:              team->m_taskManager->RunTask( GlobalResearch::TypeSquad );          break;        
        case CrateRewardHarvester:          team->m_taskManager->RunTask( GlobalResearch::TypeHarvester );      break;        
        case CrateRewardEngineer:           team->m_taskManager->RunTask( GlobalResearch::TypeEngineer );       break;
        case CrateRewardArmour:             team->m_taskManager->RunTask( GlobalResearch::TypeArmour );         break;        
        case CrateRewardAirstrike:          team->m_taskManager->RunTask( GlobalResearch::TypeAirStrike );      break;
        case CrateRewardNuke:               team->m_taskManager->RunTask( GlobalResearch::TypeNuke );           break;
        case CrateRewardSubversion:         team->m_taskManager->RunTask( GlobalResearch::TypeSubversion );     break;
        case CrateRewardBooster:            team->m_taskManager->RunTask( GlobalResearch::TypeBooster );        break;
        case CrateRewardHotFeet:            team->m_taskManager->RunTask( GlobalResearch::TypeHotFeet );        break;
        case CrateRewardSpam:               team->m_taskManager->RunTask( GlobalResearch::TypeInfection );      break;
        case CrateRewardDarkForest:         team->m_taskManager->RunTask( GlobalResearch::TypeDarkForest );     break;
        case CrateRewardAntNest:            team->m_taskManager->RunTask( GlobalResearch::TypeAntsNest );       break;
        //case CrateRewardPlague:             team->m_taskManager->RunTask( GlobalResearch::TypePlague );         break;
        case CrateRewardEggs:               team->m_taskManager->RunTask( GlobalResearch::TypeEggSpawn );       break;
        case CrateRewardMeteor:             team->m_taskManager->RunTask( GlobalResearch::TypeMeteorShower );   break;
//        case CrateRewardExtinguisher:       team->m_taskManager->RunTask( GlobalResearch::TypeExtinguisher );   break;
        case CrateRewardRage:               team->m_taskManager->RunTask( GlobalResearch::TypeRage );           break;
//        case CrateRewardTank:               team->m_taskManager->RunTask( GlobalResearch::TypeTank );           break;

        case CrateRewardNapalmStrike:       
        {
            if( team->m_taskManager->RunTask( GlobalResearch::TypeAirStrike ) )
            {
                team->m_taskManager->GetTask( team->m_taskManager->m_mostRecentTaskId )->m_option = 1;
            }
        }
        break;

        case CrateRewardGunTurret:          
        {
            if( team->m_taskManager->RunTask( GlobalResearch::TypeGunTurret ) )
            {
                team->m_taskManager->GetTask( team->m_taskManager->m_mostRecentTaskId )->m_option = GunTurret::GunTurretTypeStandard;
            }
        }
        break;

        case CrateRewardRocketTurret:          
        {
            if( team->m_taskManager->RunTask( GlobalResearch::TypeGunTurret ) )
            {
                team->m_taskManager->GetTask( team->m_taskManager->m_mostRecentTaskId )->m_option = GunTurret::GunTurretTypeRocket;
            }
        }
        break;

        case CrateRewardFlameTurret:          
        {
            if( team->m_taskManager->RunTask( GlobalResearch::TypeGunTurret ) )
            {
                team->m_taskManager->GetTask( team->m_taskManager->m_mostRecentTaskId )->m_option = GunTurret::GunTurretTypeFlame;
            }
        }
        break;

            // These powerups all go off on the spot, immediately

        case CrateRewardDarwinians:         RunDarwinians( m_pos, m_id.GetTeamId() );       messageShown = true;      break;        
        case CrateRewardRandomiser:         RunRandomiser( m_pos, m_id.GetTeamId() );       messageShown = true;      break;        
        case CrateRewardSpawnMania:         RunSpawnMania();                                messageShown = true;      break; 
        case CrateRewardBitzkreig:          RunBitzkreig();                                 messageShown = true;      break;
        case CrateRewardCrateMania:         RunCrateMania();                                messageShown = true;      break;
        case CrateRewardForest:             RunMagicalForest( m_pos, monsterTeam );         messageShown = true;      break;
//        case CrateRewardFormationIncrease:  RunFormtationIncrease( m_id.GetTeamId() );      messageShown = true;      break;
        case CrateRewardRocketDarwinians:   RunRocketDarwinians( m_id.GetTeamId() );        messageShown = true;      break;
        case CrateRewardSpaceShip:          RunSpaceShip();                                 messageShown = true;      break;
        case CrateRewardTriffids:           RunTriffids( m_pos, monsterTeam );              messageShown = true;      break;
        case CrateRewardSlowDownTime:       RunSlowDownTime();                              messageShown = true;      break;
        case CrateRewardSpeedUpTime:        RunSpeedUpTime();                               messageShown = true;      break;
#ifdef INCLUDE_THEBEAST
        case CrateRewardTheBeast:           RunTheBeast( m_pos );                           messageShown = true;        break;
#endif

        default:    AppReleaseAssert(false, "Undefined Crate Reward in CaptureCrate(): id = %d", rewardId );  break;
    };

    
    //
    // Show a message

    if( !messageShown )
    {
		UnicodeString msg;

        if( m_id.GetTeamId() != g_app->m_globalWorld->m_myTeamId )
        {
			msg = LANGUAGEPHRASE("crate_enemycapture");
			msg.ReplaceStringFlag( L'T', UnicodeString(team->GetTeamName()) );
            g_app->m_location->SetCurrentMessage( msg, m_id.GetTeamId() );   
        }
        else
        {
            // Removed by Chris
            // Since the crate popup gives you the same information

            //msg = LANGUAGEPHRASE("crate_capture");
			//UnicodeString replace;
			//GetNameTranslated(rewardId, replace);
			//msg.ReplaceStringFlag( L'P', replace);
        }        
    }
}

void Crate::CaptureCrate(int _crateId, int _teamId)
{
    Team *team = g_app->m_location->m_teams[_teamId];
    switch( _crateId )
    {
        case CrateRewardSquad:              team->m_taskManager->RunTask( GlobalResearch::TypeSquad );          break;        
        case CrateRewardHarvester:          team->m_taskManager->RunTask( GlobalResearch::TypeHarvester );      break;        
        case CrateRewardEngineer:           team->m_taskManager->RunTask( GlobalResearch::TypeEngineer );       break;
        case CrateRewardArmour:             team->m_taskManager->RunTask( GlobalResearch::TypeArmour );         break;        
        case CrateRewardAirstrike:          team->m_taskManager->RunTask( GlobalResearch::TypeAirStrike );      break;
        case CrateRewardNuke:               team->m_taskManager->RunTask( GlobalResearch::TypeNuke );           break;
        case CrateRewardSubversion:         team->m_taskManager->RunTask( GlobalResearch::TypeSubversion );     break;
        case CrateRewardBooster:            team->m_taskManager->RunTask( GlobalResearch::TypeBooster );        break;
        case CrateRewardHotFeet:            team->m_taskManager->RunTask( GlobalResearch::TypeHotFeet );        break;
        //case CrateRewardSpam:               team->m_taskManager->RunTask( GlobalResearch::TypeInfection );      break;
        case CrateRewardForest:             team->m_taskManager->RunTask( GlobalResearch::TypeMagicalForest );  break;
        case CrateRewardDarkForest:         team->m_taskManager->RunTask( GlobalResearch::TypeDarkForest );     break;
        case CrateRewardAntNest:            team->m_taskManager->RunTask( GlobalResearch::TypeAntsNest );       break;
        //case CrateRewardPlague:             team->m_taskManager->RunTask( GlobalResearch::TypePlague );         break;
        case CrateRewardEggs:               team->m_taskManager->RunTask( GlobalResearch::TypeEggSpawn );       break;
        case CrateRewardMeteor:             team->m_taskManager->RunTask( GlobalResearch::TypeMeteorShower );   break;
        case CrateRewardRage:               team->m_taskManager->RunTask( GlobalResearch::TypeRage );           break;
//        case CrateRewardFormationIncrease:  RunFormtationIncrease( _teamId );                                   break;
        case CrateRewardRocketDarwinians:   RunRocketDarwinians( _teamId );                                     break;
//        case CrateRewardExtinguisher:       team->m_taskManager->RunTask( GlobalResearch::TypeExtinguisher );   break;
//        case CrateRewardTank:               team->m_taskManager->RunTask( GlobalResearch::TypeTank );           break;
//        case CrateRewardTheBeast:           RunTheBeast( m_pos );                                               break;

        case CrateRewardGunTurret:          
        {
            if( team->m_taskManager->RunTask( GlobalResearch::TypeGunTurret ) )
            {
                team->m_taskManager->GetTask( team->m_taskManager->m_mostRecentTaskId )->m_option = GunTurret::GunTurretTypeStandard;
            }
        }
        break;

        case CrateRewardRocketTurret:          
        {
            if( team->m_taskManager->RunTask( GlobalResearch::TypeGunTurret ) )
            {
                team->m_taskManager->GetTask( team->m_taskManager->m_mostRecentTaskId )->m_option = GunTurret::GunTurretTypeRocket;
            }
        }
        break;

        case CrateRewardFlameTurret:          
        {
            if( team->m_taskManager->RunTask( GlobalResearch::TypeGunTurret ) )
            {
                team->m_taskManager->GetTask( team->m_taskManager->m_mostRecentTaskId )->m_option = GunTurret::GunTurretTypeFlame;
            }
        }
        break;

        case CrateRewardNapalmStrike:       
        {
            if( team->m_taskManager->RunTask( GlobalResearch::TypeAirStrike ) )
            {
                team->m_taskManager->GetTask( team->m_taskManager->m_mostRecentTaskId )->m_option = 1;
            }
        }
        break;

        default:    AppReleaseAssert(false, "Undefined Crate Reward in RunCaptureCrate(int): id = %d", _crateId );  break;
    }
}

void Crate::RunUncapturedCrate( int _teamId )
{
#ifndef INCLUDE_CRATES_ADVANCED
    // If we arent allowed to use advanced crates
    // then there isnt much we can do here, since all 'bad' crate types are banned.
    // So bail immediately.
    return;
#endif
    if( g_app->m_multiwinia->m_basicCratesOnly ) return;


    int rewardId = 0;

    if( m_rewardId != -1 )
    {
        rewardId = m_rewardId;
    }
    else
    {
        LList<int> acceptableCrates;
        acceptableCrates.PutData( CrateRewardSpam );
        acceptableCrates.PutData( CrateRewardRandomiser );
        acceptableCrates.PutData( CrateRewardPlague );
        acceptableCrates.PutData( CrateRewardDarkForest );
        acceptableCrates.PutData( CrateRewardTriffids );
#ifdef INCLUDE_THEBEAST
        acceptableCrates.PutData( CrateRewardTheBeast );
#endif
        if( !s_spaceShipUsed && 
            g_app->m_multiwinia->m_gameType != Multiwinia::GameTypeAssault) 
        {
            acceptableCrates.PutData( CrateRewardSpaceShip );
        }

        acceptableCrates.PutData( CrateRewardAntNest );

        int chosenIndex = syncrand() % acceptableCrates.Size();
        rewardId = acceptableCrates[chosenIndex];
    }
    
    Team *team = g_app->m_location->m_teams[_teamId];

    int monsterTeam = g_app->m_location->GetMonsterTeamId();
    //if( monsterTeam == _teamId )
    {
        g_app->m_location->InitMonsterTeam( _teamId );
    }
    

    switch( rewardId )
    {
        case CrateRewardAirstrike:      RunAirStrike( m_pos, monsterTeam );     break;
        //case CrateRewardNuke:           RunNuke( m_pos, _teamId );           break;        
        case CrateRewardDarwinians:     RunDarwinians( m_pos, _teamId );        break;
        case CrateRewardSpam:           RunInfection( m_pos, monsterTeam );      break;  
        case CrateRewardForest:         RunMagicalForest( m_pos, monsterTeam );  break;
        case CrateRewardDarkForest:     RunDarkForest( m_pos, monsterTeam );     break;
        case CrateRewardRandomiser:     RunRandomiser( m_pos, monsterTeam );     break;
        case CrateRewardAntNest:        RunAntNest( m_pos, monsterTeam );        break;
        case CrateRewardEggs:           RunEggs( m_pos, monsterTeam );           break;
        case CrateRewardMeteor:         RunMeteorShower( m_pos, monsterTeam );   break;
        case CrateRewardSpaceShip:      RunSpaceShip();                         break;
        case CrateRewardTriffids:       RunTriffids( m_pos, monsterTeam );       break;
        case CrateRewardPlague:         RunPlague( m_pos, monsterTeam );        break;
        case CrateRewardNapalmStrike:   RunAirStrike( m_pos, monsterTeam, true );   break;
        case CrateRewardSlowDownTime:   RunSlowDownTime();                          break;
        case CrateRewardSpeedUpTime:    RunSpeedUpTime();                       break;
#ifdef INCLUDE_THEBEAST
        case CrateRewardTheBeast:       RunTheBeast( m_pos );                   break;
#endif

        default:    AppReleaseAssert(false, "Undefined Crate Reward in RunUncapturedCrate(): id = %d", rewardId );  break;
    }
}


void Crate::StopCaptureSounds()
{
    g_app->m_soundSystem->StopAllSounds( m_id, "Crate Capturing_" );
}


void Crate::AdvanceOnGround()
{
    if( !m_aiTarget )
    {
        m_aiTarget = (AITarget *)Building::CreateBuilding( Building::TypeAITarget );

        AITarget targetTemplate;
        targetTemplate.m_pos       = m_pos;
        targetTemplate.m_pos.y     = g_app->m_location->m_landscape.m_heightMap->GetValue(m_pos.x, m_pos.z);

        m_aiTarget->Initialise( (Building *)&targetTemplate );
        m_aiTarget->m_id.SetUnitId( UNIT_BUILDINGS );
        m_aiTarget->m_id.SetUniqueId( g_app->m_globalWorld->GenerateBuildingId() );   
        m_aiTarget->m_priorityModifier = 0.5;
        m_aiTarget->m_crateTarget = true;

        g_app->m_location->m_buildings.PutData( m_aiTarget );

        g_app->m_location->RecalculateAITargets();
    }

    int numNearby = 0;
    int currentMajority = RecalculateOwnership(numNearby);

    if( m_id.GetTeamId() == currentMajority &&
        m_id.GetTeamId() != 255) // current owner still in control, advance normally
    {
        bool includes[NUM_TEAMS];
        memset( includes, false, sizeof(bool) * NUM_TEAMS );
        includes[m_id.GetTeamId()] = true;

        int prevIntCaptured = int ( 5.0 * m_captureTimer / CRATE_CAPTURE_TIME );
        double amountToAdd = SERVER_ADVANCE_PERIOD * (numNearby / 100.0);
        if( amountToAdd > 0.05 ) amountToAdd = 0.05;
        m_captureTimer += amountToAdd;
        m_captureTimer = min( m_captureTimer, CRATE_CAPTURE_TIME );
        int postIntCaptured = int ( 5.0 * m_captureTimer / CRATE_CAPTURE_TIME );
        
        /*m_life += SERVER_ADVANCE_PERIOD;
        m_life = min( 60.0, m_life );*/
        m_life = 60.0;

        if( postIntCaptured != prevIntCaptured )
        {
            StopCaptureSounds();
            char newSound[256];
            sprintf( newSound, "Capturing_%dpc", postIntCaptured * 20 );
            g_app->m_soundSystem->TriggerBuildingEvent( this, newSound );
        }

        if( m_captureTimer >= CRATE_CAPTURE_TIME )
        {
            // team has captured the crate
            bool noAdvanced = g_app->m_multiwinia->m_basicCratesOnly;
#ifndef INCLUDE_CRATES_ADVANCED
            noAdvanced = true;
#endif
            if( m_rewardId != -1 ||
                syncfrand(1.0) < 0.9 ||
                noAdvanced)
            {
                CaptureCrate();
            }
            else
            {
                RunUncapturedCrate( m_id.GetTeamId() );
            }

            m_destroyed = true;

            StopCaptureSounds();
            g_app->m_soundSystem->StopAllSounds( m_id, "Crate BeginCapture" );
            g_app->m_soundSystem->TriggerBuildingEvent( this, "Captured" );

            if( m_aiTarget )
            {
                m_aiTarget->m_destroyed = true;
                m_aiTarget->m_neighbours.EmptyAndDelete();
                m_aiTarget = NULL;
                g_app->m_location->RecalculateAITargets();
            }

            if( m_id.GetTeamId() == g_app->m_globalWorld->m_myTeamId &&
                g_app->m_multiwiniaHelp )
            {
                g_app->m_multiwiniaHelp->NotifyCapturedCrate();
            }
        }
    }
    else
    {
        int prevIntCaptured = int ( 5.0 * m_captureTimer / CRATE_CAPTURE_TIME );
        m_captureTimer -= SERVER_ADVANCE_PERIOD;
        m_captureTimer = max( 0.0, m_captureTimer );
        int postIntCaptured = int ( 5.0 * m_captureTimer / CRATE_CAPTURE_TIME );

        if( postIntCaptured != prevIntCaptured )
        {
            StopCaptureSounds();
            char newSound[256];
            sprintf( newSound, "Capturing_%pc", postIntCaptured );
            g_app->m_soundSystem->TriggerBuildingEvent( this, newSound );
        }

        if( m_captureTimer <= 0.0 &&
            m_id.GetTeamId() != currentMajority )
        {
            SetTeamId( currentMajority );

            if( currentMajority == 255 )
            {
                StopCaptureSounds();
                g_app->m_soundSystem->StopAllSounds( m_id, "Crate BeginCapture" );
                g_app->m_soundSystem->TriggerBuildingEvent( this, "LostCapture" );
            }
            else
            {
                g_app->m_soundSystem->TriggerBuildingEvent( this, "BeginCapture" );
                g_app->m_soundSystem->TriggerBuildingEvent( this, "Capturing_0pc" );
            }
        }

        bool tutorial = (g_app->m_multiwiniaTutorial && 
                         (g_app->m_multiwiniaTutorialType == App::MultiwiniaTutorial1 ||
                          !g_app->m_multiwiniaHelp->m_tutorial2AICanSpawn ) );
        if( m_id.GetTeamId() == 255 && !tutorial )
        {
            // if no one is trying to capture us, start to decay
            m_life -= SERVER_ADVANCE_PERIOD;
            if( m_life <= 0.0 )
            {
                if( m_captureTimer <= 0.0 )
                {
                    //RunUncapturedCrate( g_app->m_location->GetMonsterTeamId() );
                    m_destroyed = true;
                    g_app->m_soundSystem->TriggerBuildingEvent( this, "Expired" );

                    //g_app->m_location->SetCurrentMessage( "Crate Expired!" );
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
            }
        }
    }

    if( m_destroyed )
    {
        // add an explosion effect
        Matrix34 mat( m_front, g_upVector, m_pos );
        g_explosionManager.AddExplosion( m_shape, mat, 1.0f );
    }
}


int Crate::RecalculateOwnership( int &_num )
{
    //if( GetNetworkTime() > m_recountTimer )
    {
        int teamCount[NUM_TEAMS];
        memset( teamCount, 0, sizeof(int)*NUM_TEAMS);
        int totalCount = 0;
        int highestCount = 0;
        int newTeamId = 255;

        for( int t = 0; t < NUM_TEAMS; ++t )
        {
            if( g_app->m_location->m_teams[t]->m_teamType != TeamTypeUnused )
            {
                int numFound;
                bool includes[NUM_TEAMS];
                memset( includes, false, sizeof(bool) * NUM_TEAMS );
                includes[t] = true;
                g_app->m_location->m_entityGrid->GetNeighbours( s_neighbours, m_pos.x, m_pos.z, CRATE_SCAN_RANGE, &numFound, includes );
                int entitiesCounted = 0;
        
                for( int i = 0; i < numFound; ++i )
                {
                    WorldObjectId id = s_neighbours[i];
                    Entity *entity = g_app->m_location->GetEntity( id );
                    if( entity && entity->m_type == Entity::TypeDarwinian && 
                        !entity->m_dead &&
                        ((Darwinian *)entity)->m_state != Darwinian::StateInsideArmour )
                    {
                        ++entitiesCounted;
                    }
                }

                teamCount[t] = entitiesCounted;
                totalCount += entitiesCounted;

                if( teamCount[t] > highestCount )
                {
                    newTeamId = t;
                    highestCount = teamCount[t];
                    _num = highestCount;
                }
            }
        }

        /*if( teamCount[newTeamId] > 30 )
        {
            SetTeamId( newTeamId );
        }
        else
        {
            SetTeamId( 255 );
        }*/
        m_recountTimer = GetNetworkTime() + 1.0;

        return newTeamId;
    }
}


double Crate::GetPercentCaptured()
{
    if( m_id.GetTeamId() == 255 ) return 0.0;

    return (m_captureTimer / CRATE_CAPTURE_TIME);
}


void Crate::Render( double _predictionTime )
{
    Vector3 rotateAround = g_upVector;
    rotateAround.RotateAroundX( g_gameTime * 1.0 );    
    rotateAround.RotateAroundZ( g_gameTime * 0.7 );
    rotateAround.Normalise();
    
    m_front.RotateAround( rotateAround * g_advanceTime );
    m_up.RotateAround( rotateAround * g_advanceTime );

    Vector3 predictedPos = m_pos + m_vel * _predictionTime;
	Matrix34 mat(m_front, m_up, predictedPos);

	m_shape->Render(0.0, mat);
}

void Crate::RenderAlphas( double _predictionTime )
{
    Building::RenderAlphas( _predictionTime );
    if( m_onGround )
    {
        RenderAlphasOnGround( _predictionTime );
    }
    else
    {
        RenderAlphasFloating( _predictionTime );
    }

}

void Crate::RenderAlphasOnGround( double _predictionTime )
{
    RenderBubble( _predictionTime );
	RenderGlow( _predictionTime );
}

void Crate::RenderBubble( double _predictionTime )
{
    glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/bubble.bmp" ) );
	glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
 	glEnable		( GL_BLEND );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );
    glDepthMask     ( false );    
    glDisable       ( GL_CULL_FACE );

    float size = m_radius * 2.0f;
    Vector3 pos = m_pos + m_vel * _predictionTime;// - size/2.0f;
    
    if( m_onGround )
    {
        if( m_bubbleTimer > 0.0 )
        {
            Vector3 pos = m_pos + m_vel * _predictionTime;
            Vector3 up = g_app->m_camera->GetUp();
            Vector3 right = g_app->m_camera->GetRight();
           
            glColor4ub   ( 255, 255, 255, 150 );

            float predictedHealth = m_bubbleTimer;
            if( m_onGround ) predictedHealth -= 40.0f * _predictionTime;
            else             predictedHealth -= 20.0f * _predictionTime;
            float landHeight = g_app->m_location->m_landscape.m_heightMap->GetValue( pos.x, pos.z );

            predictedHealth = max( 0.0f, predictedHealth );

            m_bubbleTimer -= 40.0 * SERVER_ADVANCE_PERIOD;

            size *= 0.5f;
            
            glColor4ub  ( 255, 255, 255, predictedHealth * 2.0f );
            glBegin(GL_QUADS);

            for( int i = 0; i < 3; ++i )
            {
                Vector3 fragmentPos = pos;
                if( i == 0 ) fragmentPos.x -= 10.0f - predictedHealth / 10.0f;
                if( i == 1 ) fragmentPos.z += 10.0f - predictedHealth / 10.0f;
                if( i == 2 ) fragmentPos.x += 10.0f - predictedHealth / 10.0f;            
                fragmentPos.y += ( fragmentPos.y - landHeight ) * i * 0.5f;            
                
                float tleft = 0.0f;
                float tright = 1.0f;
                float ttop = 0.0f;
                float tbottom = 1.0f;

                if( i == 0 )
                {
                    tright -= (tright-tleft)/2;
                    tbottom -= (tbottom-ttop)/2;
                }
                if( i == 1 )
                {
                    tleft += (tright-tleft)/2;
                    tbottom -= (tbottom-ttop)/2;
                }
                if( i == 2 )
                {
                    ttop += (tbottom-ttop)/2;
                    tleft += (tright-tleft)/2;
                }

                glTexCoord2f(tleft, tbottom);     glVertex3dv( (fragmentPos + right * size - up * size).GetData() );
                glTexCoord2f(tright, tbottom);    glVertex3dv( (fragmentPos - right * size - up * size).GetData() );
                glTexCoord2f(tright, ttop);       glVertex3dv( (fragmentPos - right * size + up * size).GetData() );
                glTexCoord2f(tleft, ttop);        glVertex3dv( (fragmentPos + right * size + up * size).GetData() );
            }

            glEnd();
        }
    }
    else
    {
        glColor4ub   ( 255, 255, 255, 150 );    

        Vector3 up = g_app->m_camera->GetUp();
        Vector3 right = g_app->m_camera->GetRight();

        glBegin( GL_QUADS );
            glTexCoord2i( 0, 0 );       glVertex3dv( (pos - right * size - up * size).GetData() );
            glTexCoord2i( 0, 1 );       glVertex3dv( (pos - right * size + up * size).GetData() );
            glTexCoord2i( 1, 1 );       glVertex3dv( (pos + right * size + up * size).GetData() );
            glTexCoord2i( 1, 0 );       glVertex3dv( (pos + right * size - up * size).GetData() );    
        glEnd();
    }
    
    glEnable        ( GL_CULL_FACE );
	glDisable       ( GL_BLEND );
    glDisable       ( GL_TEXTURE_2D );
    glDepthMask     ( true );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
}


void Crate::RenderGlow( double _predictionTime )
{
    Vector3 camUp = g_app->m_camera->GetUp();
    Vector3 camRight = g_app->m_camera->GetRight();
        
	//
	// Red glow while falling to earth

    glDepthMask     ( false );
    glEnable        ( GL_BLEND );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );
    glEnable        ( GL_TEXTURE_2D );
    glDisable       ( GL_DEPTH_TEST );
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/starburst.bmp" ) );
    glBegin         ( GL_QUADS );

    int buildingDetail = g_prefsManager->GetInt( "RenderBuildingDetail", 1 );
    int numStars = 10;
    float timeIndex = g_gameTime + m_id.GetUniqueId() * 10.0f;
    float alpha = 0.6f * ( m_pos.y / CRATE_START_HEIGHT );

    if( buildingDetail == 2 ) numStars = 5;
    if( buildingDetail == 3 ) numStars = 2;
    
	if( !m_onGround )
	{
		for( int i = 0; i < numStars; ++i )
		{
			Vector3 pos = m_pos + m_vel * _predictionTime;
			pos.x += sinf(timeIndex+i) * i * 1.7f;
			pos.y += cosf(timeIndex+i) * cosf(i*20) * 4;
			pos.z += cosf(timeIndex+i) * i * 1.7f;

			float size = i * 15.0f;
	        
			glColor4f( 1.0f, 0.4f, 0.2f, alpha );

			glTexCoord2i(0,0);      glVertex3dv( (pos - camRight * size + camUp * size).GetData() );
			glTexCoord2i(1,0);      glVertex3dv( (pos + camRight * size + camUp * size).GetData() );
			glTexCoord2i(1,1);      glVertex3dv( (pos + camRight * size - camUp * size).GetData() );
			glTexCoord2i(0,1);      glVertex3dv( (pos - camRight * size - camUp * size).GetData() );
		}
	}

	//
	// Fixed glow on all the time
	// Colour = team colour currently capturing

    glEnd();
    glDepthMask     ( false );
    glEnable        ( GL_BLEND );
	glEnable		( GL_DEPTH_TEST );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );
    glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/cloudyglow.bmp" ) );
    
    int maxBlobs = 20;
    if( buildingDetail == 2 ) maxBlobs = 10;
    if( buildingDetail == 3 ) maxBlobs = 0;

    alpha = 1.0f;

	RGBAColour colour(255,155,100,55);
	if( m_id.GetTeamId() != 255 )
	{
		RGBAColour teamColour = g_app->m_location->m_teams[m_id.GetTeamId()]->m_colour;
		teamColour.a = 155;
		colour = teamColour;
	}

	glColor4ubv( colour.GetData() );
    glBegin( GL_QUADS );

    for( int i = 0; i < maxBlobs; ++i )
    {        
        Vector3 pos = m_centrePos + m_vel * _predictionTime;
        pos.x += sinf(timeIndex+i) * i * 0.3f;
        pos.y += cosf(timeIndex+i) * sinf(i*10) * 5;
        pos.z += cosf(timeIndex+i) * i * 0.3f;

        float size = 5.0f + sinf(timeIndex+i*10) * 7.0f;
        size = max( size, 2.0f );
                
        glTexCoord2i(0,0);      glVertex3dv( (pos - camRight * size + camUp * size).GetData() );
        glTexCoord2i(1,0);      glVertex3dv( (pos + camRight * size + camUp * size).GetData() );
        glTexCoord2i(1,1);      glVertex3dv( (pos + camRight * size - camUp * size).GetData() );
        glTexCoord2i(0,1);      glVertex3dv( (pos - camRight * size - camUp * size).GetData() );
    }

    glEnd();
    glDisable( GL_TEXTURE_2D );
}


void Crate::RenderAlphasFloating( double _predictionTime )
{
    RenderBubble( _predictionTime );
    RenderGroundMarker( _predictionTime );
	RenderGlow( _predictionTime );
}


void Crate::RenderGroundMarker( double _predictionTime )
{
	glEnable        (GL_TEXTURE_2D);
	glEnable        (GL_BLEND);
    glDisable       (GL_CULL_FACE );
    glDepthMask     (false);
    glDisable       (GL_DEPTH_TEST);

    Vector3 pos = m_pos + m_vel * _predictionTime;
    pos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( pos.x, pos.z );

    float scale = m_radius * 2.0f;
    /*float camDist = ( g_app->m_camera->GetPos() - pos ).Mag();
    scale *= sqrtf(camDist) / 40.0f;*/

    Vector3 front(1,0,0);
    front.RotateAroundY( g_gameTimer.GetAccurateTime() * -0.3f ); 
    Vector3 up = g_app->m_location->m_landscape.m_normalMap->GetValue( pos.x, pos.z );
    Vector3 rightAngle = (front ^ up).Normalise();   
    front = rightAngle ^ up;

    pos -= rightAngle *0.5f * scale;
    pos += front * 0.5f * scale;
    pos += up * 3.0f;

    RGBAColour colour(255,255,255,180);

    char filename[256];
    char shadow[256];

    sprintf( filename, "icons/target.bmp" );
    sprintf(shadow, "shadow_%s", filename);

    if( !g_app->m_resource->DoesTextureExist( shadow ) )
    {
        BinaryReader *binReader = g_app->m_resource->GetBinaryReader( filename );
        BitmapRGBA bmp( binReader, "bmp" );
	    SAFE_DELETE(binReader);

        bmp.ApplyBlurFilter( 10.0f );
	    g_app->m_resource->AddBitmap(shadow, bmp);
    }

    glBindTexture   (GL_TEXTURE_2D, g_app->m_resource->GetTexture(shadow));
    glBlendFunc     (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR);
    glColor4f       (0.5f, 0.5f, 0.5f, 0.0f);

    glBegin( GL_QUADS );
        glTexCoord2i(0,1);      glVertex3dv( (pos).GetData() );
        glTexCoord2i(1,1);      glVertex3dv( (pos + rightAngle * scale).GetData() );
        glTexCoord2i(1,0);      glVertex3dv( (pos - front * scale + rightAngle * scale).GetData() );
        glTexCoord2i(0,0);      glVertex3dv( (pos - front * scale).GetData() );
    glEnd();

    glColor4ubv     (colour.GetData());
	glBindTexture   (GL_TEXTURE_2D, g_app->m_resource->GetTexture( filename ));
	glBlendFunc     (GL_SRC_ALPHA, GL_ONE);							

    glBegin( GL_QUADS );
        glTexCoord2i(0,1);      glVertex3dv( (pos).GetData() );
        glTexCoord2i(1,1);      glVertex3dv( (pos + rightAngle * scale).GetData() );
        glTexCoord2i(1,0);      glVertex3dv( (pos - front * scale + rightAngle * scale).GetData() );
        glTexCoord2i(0,0);      glVertex3dv( (pos - front * scale).GetData() );
    glEnd();

    glDepthMask     (true);
    glEnable        (GL_DEPTH_TEST );    
    glDisable       (GL_BLEND);
	glBlendFunc     (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);							
	glDisable       (GL_TEXTURE_2D);
    glEnable        (GL_CULL_FACE);
}

bool Crate::IsInView()
{
    if( Building::IsInView() ) return true;

    if( g_app->m_camera->PosInViewFrustum( m_pos+Vector3(0,g_app->m_camera->GetPos().y,0) ))
    {
        return true;
    }

    Vector3 pos = m_pos;
    pos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( pos.x, pos.z );
    if( g_app->m_camera->PosInViewFrustum( pos ) ) return true;

    return false;
}

Vector3 Crate::PushFromObstructions( Vector3 _pos )
{
    Vector3 result = _pos;    
    if( m_onGround )
    {
        result.y = g_app->m_location->m_landscape.m_heightMap->GetValue( result.x, result.z );
    }

    //Matrix34 transform( m_front, g_upVector, result );    

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
            //if( m_shape && building->DoesShapeHit( m_shape, transform ) ) hit = true;
            if( building->DoesSphereHit( result, m_radius ) ) hit = true;

            if( hit )
            {
                Vector3 bPos = building->m_pos;
                Vector3 ourPos = result;
                bPos.y = 0.0;
                ourPos.y = 0.0;
                Vector3 pushForce = (bPos - ourPos).SetLength(2.0);
                while( building->DoesSphereHit( result, m_radius ) )
                {
                    result -= pushForce; 
                    //result.y = g_app->m_location->m_landscape.m_heightMap->GetValue( result.x, result.z );
                }
            }
        }
    }

    if( g_app->m_location->m_landscape.m_heightMap->GetValue( result.x, result.z ) <= 10.0 )
    {
        result = _pos;
    }

    return result;
}


bool Crate::DoesSphereHit(Vector3 const &_pos, double _radius)
{
    return false;
}


bool Crate::DoesShapeHit(Shape *_shape, Matrix34 _transform)
{
    return false;
}


bool Crate::DoesRayHit(Vector3 const &_rayStart, Vector3 const &_rayDir, 
                              double _rayLen, Vector3 *_pos, Vector3 *norm )
{
    return false;
}


char *Crate::GetName(int _type)
{
    // NOTE BY CHRIS
    // There cant be any spaces in these names
    
    switch( _type )
    {
        case Crate::CrateRewardSquad:               return "Squad";
        case Crate::CrateRewardHarvester:           return "Harvester";
        case Crate::CrateRewardEngineer:            return "Engineer";
        case Crate::CrateRewardArmour:              return "Armour";
        case Crate::CrateRewardAirstrike:           return "Airstrike";
        case Crate::CrateRewardNuke:                return "Nuke";
        case Crate::CrateRewardDarwinians:          return "Darwinians";
        case Crate::CrateRewardBooster:             return "PersonalShield";
        case Crate::CrateRewardSubversion:          return "Subversion";
        case Crate::CrateRewardHotFeet:             return "HotFeet";
        case Crate::CrateRewardGunTurret:           return "GunTurret";
        case Crate::CrateRewardSpam:                return "Infection";
        case Crate::CrateRewardForest:              return "MagicalForest";
        case Crate::CrateRewardRandomiser:          return "Randomiser";
        case Crate::CrateRewardAntNest:             return "AntNest";
        case Crate::CrateRewardPlague:              return "Plague";
        case Crate::CrateRewardEggs:                return "Eggs";
        case Crate::CrateRewardMeteor:              return "Meteor";
        case Crate::CrateRewardDarkForest:          return "DarkForest";
        case Crate::CrateRewardCrateMania:          return "CrateMania";
        case Crate::CrateRewardSpawnMania:          return "SpawnMania";
        case Crate::CrateRewardBitzkreig:           return "Bitzkreig";
//        case Crate::CrateRewardFormationIncrease:   return "FormationBooster";
        case Crate::CrateRewardRocketDarwinians:    return "Rocketwinians";
//        case Crate::CrateRewardExtinguisher:        return "FireExtinguisher";
        case Crate::CrateRewardRage:                return "Rage";
////        case Crate::CrateRewardTank:                return "Tank";
        case Crate::CrateRewardRocketTurret:        return "RocketTurret";
        case Crate::CrateRewardFlameTurret:         return "FlameTurret";
        case Crate::CrateRewardSpaceShip:           return "SpaceShip";
        case Crate::CrateRewardTriffids:            return "Triffids";
        case Crate::CrateRewardNapalmStrike:        return "NapalmStrike";
        case Crate::CrateRewardSlowDownTime:        return "SlowDown";
        case Crate::CrateRewardSpeedUpTime:         return "SpeedUp";
#ifdef INCLUDE_THEBEAST
        case Crate::CrateRewardTheBeast:            return "TheBeast";
#endif
    }

    return "Crate";
}

void Crate::GetNameTranslated(int _type, UnicodeString &_dest)
{
    char name[512];
    sprintf( name, "cratename_%s", GetName( _type ) );
    if( g_app->m_langTable->DoesPhraseExist( name ) )
    {
        _dest = LANGUAGEPHRASE(name);
    }
    else
    {
        _dest = UnicodeString( GetName( _type ) );
    }
}

void Crate::TriggerSoundEvent(int _crateType, Vector3 const &pos, int _teamId )
{
    char *eventName = (char*)GetName(_crateType);

    g_app->m_soundSystem->TriggerOtherEvent( pos, eventName, SoundSourceBlueprint::TypeCratePowerup, _teamId );
}


void Crate::GenerateTrees( Vector3 _pos )
{    
    int numTrees = 1 + syncrand() % 2;

    // check to see if there are any trees on the level already to use as a template
    Tree *existing = NULL;
    for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
    {
        if( g_app->m_location->m_buildings.ValidIndex(i) )
        {
            if( g_app->m_location->m_buildings[i]->m_type == Building::TypeTree )
            {
                Tree *tree = (Tree *)g_app->m_location->m_buildings[i];

                if( !tree->m_evil )
                {
                    existing = tree;
                    break;
                }
            }

        }
    }

    Tree treeTemplate;
    if( existing )
    {
        treeTemplate.Clone( existing );        
    }
    else
    {
        // if theres no existing tree, create a random one
        // this will probably look awful
        treeTemplate.m_budsize = 1.2 - syncfrand(0.4);
        treeTemplate.m_pushOut = 1.2 - syncfrand(0.4);
        treeTemplate.m_pushUp = 1.2 - syncfrand(0.4);

		// Passing syncrand() to multiple arguments of the same function call gives unsynchronized
		// results (the order of evaluation of arguments is undefined in C++). 
		// It's not especially important, but we define an ordering here:

		long rand1 = syncrand();
		long rand2 = syncrand();
		long rand3 = syncrand();
		long rand4 = syncrand();
		long rand5 = syncrand();
		long rand6 = syncrand();
		long rand7 = syncrand();
		long rand8 = syncrand();

		unsigned col = 0;

        colourToInt(100 + rand1 % 155, 100 + rand2 % 155, 100 + rand3 % 155, 100 + rand4 % 155, &col );
        treeTemplate.m_branchColour = col;
        colourToInt(100 + rand5 % 155, 100 + rand6 % 155, 100 + rand7 % 155, 100 + rand8 % 155, &col );
        treeTemplate.m_leafColour = col;
        treeTemplate.m_height = 80.0;
        treeTemplate.m_iterations = 6;
    }

    for( int i = 0; i < numTrees; ++i )
    {
        Building *newBuilding = Building::CreateBuilding( Building::TypeTree );
        int id = g_app->m_location->m_buildings.PutData(newBuilding);
        treeTemplate.m_id.Set( 255, UNIT_BUILDINGS, id, g_app->m_globalWorld->GenerateBuildingId() );
        treeTemplate.m_pos = _pos;
        treeTemplate.m_pos.x += syncsfrand(100.0);
        treeTemplate.m_pos.z += syncsfrand(100.0);
        treeTemplate.m_pos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( treeTemplate.m_pos.x, treeTemplate.m_pos.z );
        
        newBuilding->Initialise( &treeTemplate);

        Tree *newTree = (Tree *) newBuilding;
        newTree->m_seed = (int) syncfrand(99999);
        if( newTree->m_height > 100 ) newTree->m_height = 100;
        newTree->m_height = newTree->m_height * (1.2 - syncfrand(0.4));
        newTree->SetFireAmount(-1.0); // this sets the tree to its min height, without actually being on fire and burning nearby darwinians
        newTree->m_spawnsRemaining = 2 + syncfrand(3);
        newTree->m_destroyable = true;
        newTree->m_spiritDropRate = 15 + syncfrand(15);
        newBuilding->SetDetail( 1 );
    }

    g_app->m_location->m_obstructionGrid->CalculateAll();
}


void Crate::GenerateEvilTrees( Vector3 _pos )
{
#ifdef INCLUDE_CRATES_ADVANCED
    int numTrees = 2 + syncrand() % 3;

    // check to see if there are any trees on the level already to use as a template
    Building *b = NULL;
    for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
    {
        if( g_app->m_location->m_buildings.ValidIndex(i) )
        {
            if( g_app->m_location->m_buildings[i]->m_type == Building::TypeTree )
            {
                b = g_app->m_location->m_buildings[i];
                break;
            }

        }
    }

    Tree treeTemplate;
    if( !b )
    {
        // if theres no existing tree, create a random one
        // this will probably look awful
        treeTemplate.m_budsize = 1.2 - syncfrand(0.4);
        treeTemplate.m_pushOut = 1.2 - syncfrand(0.4);
        treeTemplate.m_pushUp = 1.2 - syncfrand(0.4);

        treeTemplate.m_height = 80.0;
        treeTemplate.m_iterations = 5;
    }

    unsigned int treeColour = 0;
    unsigned int leafColour = 0;
    colourToInt( 0, 0, 0, 255, &treeColour );
    colourToInt( 0, 0, 0, 200, &leafColour );


    for( int i = 0; i < numTrees; ++i )
    {
        Building *newBuilding = Building::CreateBuilding( Building::TypeTree );
        int id = g_app->m_location->m_buildings.PutData(newBuilding);
        Vector3 pos = _pos;
        pos.x += syncsfrand(100.0);
        pos.z += syncsfrand(100.0);
        pos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( pos.x, pos.z );
        //        pos = PushFromObstructions( pos );
        if( b )
        {
            newBuilding->Initialise( b );
        }
        else
        {
            newBuilding->Initialise( (Building *)&treeTemplate);
        }    

        newBuilding->m_id.Set( 255, UNIT_BUILDINGS, -1, g_app->m_globalWorld->GenerateBuildingId() );

        Tree *newTree = (Tree *) newBuilding;
        newTree->m_pos = pos;
        newTree->m_seed = (int) syncfrand(99999);
        if( newTree->m_height > 100 ) newTree->m_height = 100;
        newTree->m_height = newTree->m_height * (1.2 - syncfrand(0.4));
        newTree->SetFireAmount(-1.0); // this sets the tree to its min height, without actually being on fire and burning nearby darwinians
        newTree->m_spawnsRemaining = 2 + syncfrand(3);
        newTree->m_destroyable = true;
        newTree->m_evil = true;
        newTree->m_branchColour = treeColour;
        newTree->m_leafColour = leafColour;
        newTree->m_iterations = 5;
        newBuilding->SetDetail( 1 );
    }

    g_app->m_location->m_obstructionGrid->CalculateAll();
#endif
}


WorldObjectId Crate::RunAirStrike( Vector3 const &_pos, int _teamId, bool _special )
{
    //AppDebugOut("Creating AirStrike markers\n");
    int numMarkers = 3 + syncrand() % 5;
    int weaponId = -1;
    if( g_app->m_multiwiniaTutorial ) numMarkers = 10;

    for( int i = 0; i < numMarkers; ++i )
    {
        Vector3 targetPos = _pos;
        targetPos += Vector3( SFRAND(100.0), 0.0, SFRAND(100.0) );
        targetPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( targetPos.x, targetPos.z );
        weaponId = g_app->m_location->ThrowWeapon( _pos, targetPos, WorldObject::EffectThrowableAirstrikeMarker, _teamId );
        AirStrikeMarker *marker = (AirStrikeMarker *)g_app->m_location->m_effects[weaponId];
        marker->m_naplamBombers = _special;        
        //AppDebugOut("Marker %d Created at x:%2.2f y:%2.2f z:%2.2f\n", i+1, targetPos.x, targetPos.y, targetPos.z );
    }

    Team *team = g_app->m_location->m_teams[ _teamId ];

    UnicodeString msg;
    if( _special )
    {
        if( _teamId == g_app->m_location->GetMonsterTeamId() )
        {
            msg = LANGUAGEPHRASE("crate_message_auto_napalm");
        }
        else
        {
            msg = LANGUAGEPHRASE("crate_message_run_napalm") ;
            msg.ReplaceStringFlag( 'T', team->GetTeamName() );
            //sprintf( msg, "%s %s", team->GetTeamName(), LANGUAGEPHRASE("crate_message_run_napalm"));
        }
    }
    else
    {
        if( _teamId == g_app->m_location->GetMonsterTeamId() )
        {
            msg = LANGUAGEPHRASE("crate_message_auto_airstrike");
        }
        else
        {
            msg = LANGUAGEPHRASE("crate_message_run_airstrike");
            msg.ReplaceStringFlag( 'T', team->GetTeamName() );
            //sprintf( msg, "%s %s", team->GetTeamName(), LANGUAGEPHRASE("crate_message_run_airstrike"));
        }
    }
    g_app->m_location->SetCurrentMessage( msg, _teamId );
    g_app->m_markerSystem->RegisterMarker_Fixed( _teamId, _pos, "icons/icon_airstrike.bmp" );

    TriggerSoundEvent( CrateRewardAirstrike, _pos, _teamId );

    return g_app->m_location->m_effects[weaponId]->m_id;
}

void Crate::RunNuke( Vector3 const &_pos, int _teamId )
{
#ifdef INCLUDE_CRATES_ADVANCED
    
    int worldX = g_app->m_location->m_landscape.GetWorldSizeX() - 100.0;
    int worldZ = g_app->m_location->m_landscape.GetWorldSizeZ() - 100.0;
    
    for( int i = 0; i < 10; ++i )
    {
        Vector3 pos( FRAND(worldX), 0.0, FRAND(worldZ) );
        while( g_app->m_location->m_landscape.m_heightMap->GetValue( pos.x, pos.z ) > 0.0 )
        {
            pos.Set( FRAND(worldX), 0.0, FRAND(worldZ) );
        }

        WorldObjectId id = g_app->m_location->SpawnEntities( pos, _teamId, -1, Entity::TypeNuke, 1, Vector3(), 0.0 );

        Nuke *nuke = (Nuke *)g_app->m_location->GetEntitySafe( id, Entity::TypeNuke );
        if( nuke )
        {
            Vector3 targetPos = _pos;
            if( i == 0 ) 
            {
                nuke->m_renderMarker = true;
            }
            else
            {
                targetPos.x += syncsfrand( 300.0 );
                targetPos.z += syncsfrand( 300.0 );
            }
            targetPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( targetPos.x, targetPos.z ) - 5.0;
            nuke->m_targetPos = targetPos;
        }
    }


    Team *team = g_app->m_location->m_teams[ _teamId ];

    UnicodeString msg;
    if( _teamId == g_app->m_location->GetMonsterTeamId() )
    {
        msg = LANGUAGEPHRASE("crate_message_auto_nuke");
    }
    else
    {
        msg = LANGUAGEPHRASE("crate_message_run_nuke");
        msg.ReplaceStringFlag( 'T', team->GetTeamName() );
        //sprintf( msg, "%s %s", team->GetTeamName(), LANGUAGEPHRASE("crate_message_run_nuke") );
    }

    g_app->m_location->SetCurrentMessage( msg, _teamId  );
    g_app->m_markerSystem->RegisterMarker_Fixed( _teamId, _pos, "icons/icon_nuke.bmp" );

    TriggerSoundEvent( CrateRewardNuke, _pos, _teamId );
#endif
}

void Crate::RunSubversion(const Vector3 &_pos, int _teamId, int _taskId )
{
#ifdef INCLUDE_CRATES_ADVANCED
    int numFound;
    bool include[NUM_TEAMS];
    memset( include, false, sizeof( include ) );
    include[_teamId] = true;
    g_app->m_location->m_entityGrid->GetNeighbours( s_neighbours, _pos.x, _pos.z, POWERUP_EFFECT_RANGE, &numFound, include);

    for( int i = 0; i < numFound; ++i )
    {
        WorldObjectId id = s_neighbours[i];
        Entity *entity = g_app->m_location->GetEntity( id );
        if( entity && entity->m_type == Entity::TypeDarwinian &&
            entity->m_id.GetTeamId() == _teamId )
        {
            ((Darwinian *)entity)->GiveMindControl( _taskId );
        }
    }

    Team *team = g_app->m_location->m_teams[ _teamId ];

    UnicodeString msg(LANGUAGEPHRASE("crate_message_run_subversion"));
    msg.ReplaceStringFlag( 'T', team->GetTeamName() );
    //sprintf( msg, "%s %s", team->GetTeamName(), LANGUAGEPHRASE("crate_message_run_subversion") );
    g_app->m_location->SetCurrentMessage( msg, _teamId  );
    g_app->m_markerSystem->RegisterMarker_Fixed( _teamId, _pos, "icons/icon_subversion.bmp" );

    TriggerSoundEvent( CrateRewardSubversion, _pos, _teamId );
#endif
}

void Crate::RunBooster( Vector3 const &_pos, int _teamId, int _taskId  )
{
    int numFound;
    bool include[NUM_TEAMS];
    memset( include, false, sizeof(include) );
    include[_teamId] = true;
    g_app->m_location->m_entityGrid->GetNeighbours( s_neighbours, _pos.x, _pos.z, POWERUP_EFFECT_RANGE, &numFound, include);

    for( int i = 0; i < numFound; ++i )
    {
        WorldObjectId id = s_neighbours[i];
        Entity *entity = g_app->m_location->GetEntity( id );
        if( entity && entity->m_type == Entity::TypeDarwinian &&
            entity->m_id.GetTeamId() == _teamId )
        {
            ((Darwinian *)entity)->SetBooster(_taskId);
        }
    }

    Team *team = g_app->m_location->m_teams[ _teamId ];

    UnicodeString msg(LANGUAGEPHRASE("crate_message_run_personalshield"));
    msg.ReplaceStringFlag( 'T', team->GetTeamName() );
    //sprintf( msg, "%s %s", team->GetTeamName(), LANGUAGEPHRASE("crate_message_run_personalshield") );
    g_app->m_location->SetCurrentMessage( msg, _teamId  );
    g_app->m_markerSystem->RegisterMarker_Fixed( _teamId, _pos, "icons/icon_personalshield.bmp" );

    TriggerSoundEvent( CrateRewardBooster, _pos, _teamId );
}

void Crate::RunHotFeet( Vector3 const &_pos, int _teamId, int _taskId )
{
#ifdef INCLUDE_CRATES_ADVANCED
    int numFound;
    bool include[NUM_TEAMS];
    memset( include, false, sizeof(include) );
    include[_teamId] = true;
    g_app->m_location->m_entityGrid->GetNeighbours( s_neighbours, _pos.x, _pos.z, POWERUP_EFFECT_RANGE, &numFound, include);

    for( int i = 0; i < numFound; ++i )
    {
        WorldObjectId id = s_neighbours[i];
        Entity *entity = g_app->m_location->GetEntity( id );
        if( entity && entity->m_type == Entity::TypeDarwinian &&
            entity->m_id.GetTeamId() == _teamId )
        {
            ((Darwinian *)entity)->GiveHotFeet( _taskId );
        }
    }

    Team *team = g_app->m_location->m_teams[ _teamId ];

    UnicodeString msg(LANGUAGEPHRASE("crate_message_run_hotfeet"));
    msg.ReplaceStringFlag( 'T', team->GetTeamName() );
    //sprintf( msg, "%s %s", team->GetTeamName(), LANGUAGEPHRASE("crate_message_run_hotfeet") );
    g_app->m_location->SetCurrentMessage( msg, _teamId  );
    g_app->m_markerSystem->RegisterMarker_Fixed( _teamId, _pos, "icons/icon_hotfeet.bmp" );

    TriggerSoundEvent( CrateRewardHotFeet, _pos, _teamId );
#endif
}

WorldObjectId Crate::RunGunTurret( Vector3 const &_pos, int _teamId, int _turretType )
{
    GunTurret turretTemplate;
    turretTemplate.m_pos = _pos;
    turretTemplate.m_state = _turretType;
    
    GunTurret *turret = (GunTurret *) Building::CreateBuilding( Building::TypeGunTurret );
    g_app->m_location->m_buildings.PutData( turret );
    turret->Initialise((Building *)&turretTemplate);
    int id = g_app->m_globalWorld->GenerateBuildingId();
    turret->m_id.SetUnitId( UNIT_BUILDINGS );
    turret->m_id.SetUniqueId( id );
    g_app->m_location->m_obstructionGrid->CalculateAll();

    turret->ExplodeBody();

    //if( _teamId != g_app->m_globalWorld->m_myTeamId )
    {
        g_app->m_markerSystem->RegisterMarker_Fixed( _teamId, _pos, "icons/icon_gunturret.bmp" );
    }

    Team *team = g_app->m_location->m_teams[ _teamId ];

    UnicodeString msg(LANGUAGEPHRASE("crate_message_run_gunturret"));
    msg.ReplaceStringFlag( 'T', UnicodeString(team->GetTeamName()) );
    msg.ReplaceStringFlag( 'G', LANGUAGEPHRASE(GunTurret::GetTurretTypeName( _turretType )) );
    g_app->m_location->SetCurrentMessage( msg, _teamId  );

    TriggerSoundEvent( CrateRewardGunTurret, _pos, _teamId );

    return turret->m_id;
}

void Crate::RunInfection( Vector3 const &_pos, int _teamId  )
{
#ifdef INCLUDE_CRATES_ADVANCED
    for( int i = 0; i < 100; ++i )
    {
        Vector3 vel = g_upVector;
        vel += Vector3( SFRAND(1.0), FRAND(2.0), SFRAND(1.0) );
        vel.SetLength( 100.0 );

        SpamInfection *infection = new SpamInfection();
        infection->m_pos = _pos;
        infection->m_vel = vel;
        //infection->m_parentId = m_id.GetUniqueId();
        int index = g_app->m_location->m_effects.PutData( infection );
        infection->m_id.Set( g_app->m_location->GetMonsterTeamId(), UNIT_EFFECTS, index, -1 );
        infection->m_id.GenerateUniqueId();
    }

    g_app->m_markerSystem->RegisterMarker_Fixed( _teamId, _pos, "icons/icon_infection.bmp" );
    g_app->m_location->SetCurrentMessage( LANGUAGEPHRASE("crate_message_auto_infection"), _teamId  );

    TriggerSoundEvent( CrateRewardSpam, _pos, _teamId );
#endif
}

void Crate::RunMagicalForest(const Vector3 &_pos, int _teamId)
{
    GenerateTrees( _pos );

	UnicodeString msg;
    GetNameTranslated( Crate::CrateRewardForest, msg );
	UnicodeString temp(LANGUAGEPHRASE("multiwinia_exclamation"));
	temp.ReplaceStringFlag(L'T', msg);
	msg = temp;

    g_app->m_location->SetCurrentMessage( msg, _teamId  );

    g_app->m_markerSystem->RegisterMarker_Fixed( _teamId, _pos, "icons/icon_magicalforest.bmp" );

    TriggerSoundEvent( CrateRewardForest, _pos, _teamId );
}

void Crate::RunDarkForest( Vector3 const &_pos, int _teamId )
{
#ifdef INCLUDE_CRATES_ADVANCED
    GenerateEvilTrees( _pos );

    //if( _teamId != g_app->m_globalWorld->m_myTeamId )
    {
        g_app->m_markerSystem->RegisterMarker_Fixed( _teamId, _pos, "icons/icon_darkforest.bmp" );
    }

    Team *team = g_app->m_location->m_teams[ _teamId ];

    UnicodeString msg;
    if( _teamId == g_app->m_location->GetMonsterTeamId() )
    {
		GetNameTranslated( Crate::CrateRewardDarkForest, msg );
		UnicodeString temp(LANGUAGEPHRASE("multiwinia_exclamation"));
		temp.ReplaceStringFlag(L'T', msg);
		msg = temp;
    }
    else
    {
		msg = LANGUAGEPHRASE("crate_message_run_darkforest");
		msg.ReplaceStringFlag( 'T', UnicodeString(team->GetTeamName()));
        //sprintf( msg, "%s %s", team->GetTeamName(), LANGUAGEPHRASE("crate_message_run_darkforest") );
    }
    g_app->m_location->SetCurrentMessage( msg, _teamId  );

    TriggerSoundEvent( CrateRewardDarkForest, _pos, _teamId );
#endif
}

void Crate::RunAntNest( Vector3 const &_pos, int _teamId )
{
    AntHill antHillTemplate;                    
    antHillTemplate.m_id.SetTeamId( g_app->m_location->GetMonsterTeamId() );

    for( int i = 0; i < 2; ++i )
    {
        antHillTemplate.m_pos = _pos;
        antHillTemplate.m_pos.x += syncsfrand(200.0);
        antHillTemplate.m_pos.z += syncsfrand(200.0);
        antHillTemplate.m_pos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( antHillTemplate.m_pos.x, antHillTemplate.m_pos.z );

        antHillTemplate.m_front.x = syncsfrand(1.0);
        antHillTemplate.m_front.z = syncsfrand(1.0);
        antHillTemplate.m_front.Normalise();

        antHillTemplate.m_numAntsInside = 75 - syncrand() % 25;

        Building *newBuilding = Building::CreateBuilding( Building::TypeAntHill );
        int id = g_app->m_location->m_buildings.PutData(newBuilding); 
        newBuilding->Initialise( (Building*)&antHillTemplate);
        newBuilding->SetDetail( 1 );
        newBuilding->m_id.Set( g_app->m_location->GetMonsterTeamId(), UNIT_BUILDINGS, -1, g_app->m_globalWorld->GenerateBuildingId() );

        Matrix34 mat( newBuilding->m_front, g_upVector, newBuilding->m_pos );
        g_explosionManager.AddExplosion( newBuilding->m_shape, mat, 1.0 );
    }

    // spawn a single ant to wake up the monster team, incase it hasnt been created yet
    g_app->m_location->SpawnEntities( _pos, g_app->m_location->GetMonsterTeamId(), -1, Entity::TypeArmyAnt, 1, Vector3(), 0.0 );

    g_app->m_location->m_obstructionGrid->CalculateAll();

    //if( _teamId != g_app->m_globalWorld->m_myTeamId )
    {
        g_app->m_markerSystem->RegisterMarker_Fixed( _teamId, _pos, "icons/icon_antnest.bmp" );
    }

    Team *team = g_app->m_location->m_teams[ _teamId ];

    UnicodeString msg;
    if( _teamId == g_app->m_location->GetMonsterTeamId() )
    {
		GetNameTranslated( Crate::CrateRewardAntNest, msg );
		UnicodeString temp(LANGUAGEPHRASE("multiwinia_exclamation"));
		temp.ReplaceStringFlag(L'T', msg);
		msg = temp;
    }
    else
    {
		msg = LANGUAGEPHRASE("crate_message_run_antnest");
        msg.ReplaceStringFlag( 'T', UnicodeString(team->GetTeamName()) );
        //sprintf( msg, "%s %s", team->GetTeamName(), LANGUAGEPHRASE("crate_message_run_antnest") );
    }
    g_app->m_location->SetCurrentMessage( msg, _teamId  );

    TriggerSoundEvent( CrateRewardAntNest, _pos, _teamId );

}

void Crate::RunPlague( Vector3 const &_pos, int _teamId )
{
#ifdef INCLUDE_CRATES_ADVANCED
    int numFound;
    g_app->m_location->m_entityGrid->GetNeighbours( s_neighbours, _pos.x, _pos.z, POWERUP_EFFECT_RANGE, &numFound);

    for( int i = 0; i < numFound; ++i )
    {
        WorldObjectId id = s_neighbours[i];
        Entity *entity = g_app->m_location->GetEntity( id );
        if( entity && entity->m_type == Entity::TypeDarwinian )
        {
            ((Darwinian *)entity)->Infect();
        }
    }

    g_app->m_markerSystem->RegisterMarker_Fixed( _teamId, _pos, "icons/icon_plague.bmp" );
    g_app->m_location->SetCurrentMessage( LANGUAGEPHRASE("crate_message_auto_plague"), _teamId  );

    TriggerSoundEvent( CrateRewardPlague, _pos, _teamId );
#endif
}

void Crate::RunEggs( Vector3 const &_pos, int _teamId )
{
#ifdef INCLUDE_CRATES_ADVANCED
    int numEggs = 2 + syncrand() % 3;

    for( int i = 0; i < numEggs; ++i )
    {
        Vector3 pos = _pos;
        pos.y += 20.0;
        Vector3 vel(SFRAND(1), 4.0, SFRAND(1) );
        vel.SetLength( 50.0 + syncfrand(50.0) );
        
        WorldObjectId eggId = g_app->m_location->SpawnEntities( pos, _teamId, -1, Entity::TypeTriffidEgg, 1, vel, 0.0 );
        TriffidEgg *triffidEgg = (TriffidEgg *) g_app->m_location->GetEntitySafe( eggId, Entity::TypeTriffidEgg );
        if( triffidEgg ) 
        {
            int randomChance = -1;
            
            while( true )
            {
                randomChance = syncrand() % Triffid::NumSpawnTypes;          

                if( randomChance != Triffid::SpawnSpirits &&
                    randomChance != Triffid::SpawnEggs &&
                    randomChance != Triffid::SpawnSoulDestroyer &&
                    randomChance != Triffid::SpawnAnts &&
                    randomChance != Triffid::SpawnDarwinians )
                {
                    break;
                }
            }
            
            
            triffidEgg->m_spawnType = randomChance;
            triffidEgg->m_size = 1.0;
            triffidEgg->SetTimer(4.0 + syncfrand(4.0));
            triffidEgg->m_spawnPoint = pos;
            triffidEgg->m_roamRange = 300.0;
        }
    }

    //if( _teamId != g_app->m_globalWorld->m_myTeamId )
    {
        g_app->m_markerSystem->RegisterMarker_Fixed( _teamId, _pos, "icons/icon_egg.bmp" );
    }

    Team *team = g_app->m_location->m_teams[ _teamId ];

    UnicodeString msg;
    if( _teamId == g_app->m_location->GetMonsterTeamId() )
    {
        msg = LANGUAGEPHRASE("crate_message_auto_eggs");
    }
    else
    {
        msg = LANGUAGEPHRASE("crate_message_run_eggs");
        msg.ReplaceStringFlag( 'T', team->GetTeamName() );
        //sprintf( msg, "%s %s", team->GetTeamName(), LANGUAGEPHRASE("crate_message_run_eggs") );
    }
    g_app->m_location->SetCurrentMessage( msg, _teamId  );

    TriggerSoundEvent( CrateRewardEggs, _pos, _teamId );
#endif
}

void Crate::RunMeteorShower( Vector3 const &_pos, int _teamId )
{
#ifdef INCLUDE_CRATES_ADVANCED
    int numMeteors = 5 + syncrand() % 5;
    double area = POWERUP_EFFECT_RANGE * 6.0;

    double worldSizeX = g_app->m_location->m_landscape.GetWorldSizeX();
    double worldSizeZ = g_app->m_location->m_landscape.GetWorldSizeZ();

    for( int i = 0; i < numMeteors; ++i )
    {
        Vector3 targetPos = _pos;
        targetPos.x += syncsfrand( area );
        targetPos.z += syncsfrand( area );
        /*while( g_app->m_location->m_landscape.m_heightMap->GetValue( targetPos.x, targetPos.z ) <= 0.0 )
        {
            targetPos = _pos;
            targetPos.x += syncsfrand( area );
            targetPos.z += syncsfrand( area );
        }*/
        targetPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( targetPos.x, targetPos.z ) - 10.0;

        Vector3 pos( FRAND(worldSizeX), 1500.0, FRAND(worldSizeZ) );
        pos.y += syncfrand( 500.0 );
        WorldObjectId id = g_app->m_location->SpawnEntities( pos, _teamId, -1, Entity::TypeMeteor, 1, g_zeroVector, 0.0 );
        Meteor *m = (Meteor *)g_app->m_location->GetEntitySafe( id, Entity::TypeMeteor );
        if( m )
        {
            m->m_targetPos = targetPos;
        }
    }

    //if( _teamId != g_app->m_globalWorld->m_myTeamId )
    {
        g_app->m_markerSystem->RegisterMarker_Fixed( _teamId, _pos, "icons/icon_meteorshower.bmp" );
    }

    Team *team = g_app->m_location->m_teams[ _teamId ];

    UnicodeString msg = LANGUAGEPHRASE("crate_message_run_meteor");
    msg.ReplaceStringFlag( 'T', UnicodeString(team->GetTeamName()) );
    g_app->m_location->SetCurrentMessage( msg, _teamId  );

    TriggerSoundEvent( CrateRewardMeteor, _pos, _teamId );
#endif
}

void Crate::RunDarwinians( Vector3 const &_pos, int _teamId )
{
    int num = 50 + syncrand() % 50;
    for( int i = 0; i < num; ++i )
    {
        Vector3 vel = g_upVector + Vector3(SFRAND(1), 0.0, SFRAND(1) );
        vel.SetLength( 10.0 + syncfrand(20.0) );
        WorldObjectId id = g_app->m_location->SpawnEntities( _pos, _teamId, -1, Entity::TypeDarwinian, 1, vel, 0.0, 0.0 );
        Entity *entity = g_app->m_location->GetEntity( id );
        entity->m_front.y = 0.0;
        entity->m_front.Normalise();
        entity->m_onGround = false;
    }

    Team *team = g_app->m_location->m_teams[_teamId];
    UnicodeString msg;
    if( _teamId == g_app->m_location->GetMonsterTeamId() )
    {
        msg = UnicodeString("Evil Darwinians!");
    }
    else
    {
        msg = LANGUAGEPHRASE("crate_message_darwinians") ;
        msg.ReplaceStringFlag( 'T', team->GetTeamName() );
        //sprintf( msg, "%s %s", g_app->m_location->m_teams[_teamId]->GetTeamName(), LANGUAGEPHRASE("crate_message_darwinians") );
    }
    g_app->m_location->SetCurrentMessage( msg, _teamId  );

    g_app->m_markerSystem->RegisterMarker_Fixed( _teamId, _pos, "icons/icon_darwinian.bmp" );

    TriggerSoundEvent( CrateRewardDarwinians, _pos, _teamId );
}

void Crate::RunRandomiser( Vector3 const &_pos, int _teamId )
{
#ifdef INCLUDE_CRATES_ADVANCED
    Randomiser *r = new Randomiser();
    g_app->m_location->m_effects.PutData( r );
    r->m_pos = _pos;

	UnicodeString currentMessage;
	GetNameTranslated( Crate::CrateRewardRandomiser, currentMessage );
    g_app->m_location->SetCurrentMessage( currentMessage, _teamId  );
 
    g_app->m_markerSystem->RegisterMarker_Fixed( _teamId, _pos, "icons/icon_randomizer.bmp" );            

    TriggerSoundEvent( CrateRewardRandomiser, _pos, _teamId );
#endif
}

void Crate::RunExtinguisher( Vector3 const &_pos, int _teamId )
{
    int numFound;
    g_app->m_location->m_entityGrid->GetEnemies( s_neighbours, _pos.x, _pos.z, POWERUP_EFFECT_RANGE * 2.0, &numFound, 255 );

    for( int i = 0; i < numFound; ++i )
    {
        WorldObjectId id = s_neighbours[i];
        Darwinian *d = (Darwinian *)g_app->m_location->GetEntitySafe( id, Entity::TypeDarwinian );
        if( d )
        {
            if( d->m_state == Darwinian::StateOnFire )
            {
                d->m_state = Darwinian::StateIdle;
            }
        }
    }

    for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
    {
        if( g_app->m_location->m_buildings.ValidIndex(i) )
        {
            Tree *tree = (Tree *)g_app->m_location->m_buildings[i];
            if( tree->m_type == Building::TypeTree )
            {
                if( (tree->m_pos - _pos).Mag() <= POWERUP_EFFECT_RANGE * 2.0 )
                {
                    if( tree->IsOnFire() )
                    {
                        tree->SetFireAmount( -tree->GetFireAmount() );
                    }
                }
            }
        }
    }

    for( int i = 0; i < 200; ++i )
    {
        RGBAColour col(70, 180, 200, 200);
        if( i > 100 ) col.Set( 255,255,255,200);
        Vector3 pos = _pos;
        pos.x += sfrand( POWERUP_EFFECT_RANGE * 4.0 );
        pos.z += sfrand( POWERUP_EFFECT_RANGE * 4.0 );
        while( (pos - _pos).Mag() > POWERUP_EFFECT_RANGE * 2.0 )
        {
            pos = _pos;
            pos.x += sfrand( POWERUP_EFFECT_RANGE * 4.0 );
            pos.z += sfrand( POWERUP_EFFECT_RANGE * 4.0 );
        }
        pos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( pos.x, pos.z );
        double size = 50.0 + frand(100.0);
        g_app->m_particleSystem->CreateParticle( pos, g_upVector * 10.0, Particle::TypeFire, size, col );
    }
}

void Crate::RunSpawnMania()
{
#ifdef INCLUDE_CRATES_ADVANCED
    g_app->m_location->m_spawnManiaTimer = 5.0;

	UnicodeString msg;
	GetNameTranslated( Crate::CrateRewardSpawnMania, msg );
	UnicodeString temp(LANGUAGEPHRASE("multiwinia_exclamation"));
	temp.ReplaceStringFlag(L'T', msg);
	msg = temp;

    g_app->m_location->SetCurrentMessage( msg );
    
    TriggerSoundEvent( CrateRewardSpawnMania, g_zeroVector, 255 );    
#endif
}

void Crate::RunBitzkreig()
{
#ifdef INCLUDE_CRATES_ADVANCED
    g_app->m_location->m_bitzkreigTimer = 15.0;
    UnicodeString temp;
	GetNameTranslated( Crate::CrateRewardBitzkreig, temp );
	UnicodeString msg = UnicodeString(temp.Length()+2, temp.m_unicodestring, temp.Length());
	msg.m_unicodestring[temp.Length()] = L'!';
	msg.m_unicodestring[temp.Length()+1] = L'\0';

    g_app->m_location->SetCurrentMessage( msg );

    TriggerSoundEvent( CrateRewardBitzkreig, g_zeroVector, 255 );
#endif
}


void Crate::RunCrateMania()
{
#ifdef INCLUDE_CRATES_ADVANCED
    int numToCreate = 5 + syncfrand(5);

    for( int i = 0; i < numToCreate; ++i )
    {
        g_app->m_location->CreateRandomCrate();
    }

    UnicodeString temp;
	GetNameTranslated( Crate::CrateRewardCrateMania, temp );
	UnicodeString msg = UnicodeString(temp.Length()+2, temp.m_unicodestring, temp.Length());
	msg.m_unicodestring[temp.Length()] = L'!';
	msg.m_unicodestring[temp.Length()+1] = L'\0';

    g_app->m_location->SetCurrentMessage( msg );
    TriggerSoundEvent( CrateRewardCrateMania, g_zeroVector, 255 );
#endif

}

void Crate::RunFormtationIncrease(int _teamId)
{
    /*Team *team = g_app->m_location->m_teams[_teamId];
    int maxSize = team->m_evolutions[Team::EvolutionFormationSize];

    team->m_evolutions[Team::EvolutionFormationSize] = 200;
    //else if( maxSize == 120 ) team->m_evolutions[Team::EvolutionFormationSize] = 225;

    UnicodeString msg = LANGUAGEPHRASE("crate_message_formationbooster");
    msg.ReplaceStringFlag( 'T', team->GetTeamName() );
    //sprintf( msg, "%s %s", team->GetTeamName(), LANGUAGEPHRASE("crate_message_formationbooster") );
    g_app->m_location->SetCurrentMessage( msg, _teamId  );

    TriggerSoundEvent( CrateRewardFormationIncrease, g_zeroVector, 255 );*/
}

void Crate::RunRocketDarwinians( int _teamId )
{
    Team *team = g_app->m_location->m_teams[_teamId];
    
    team->m_evolutions[Team::EvolutionRockets] = 1;

    UnicodeString msg = LANGUAGEPHRASE("crate_message_rocketwinians") ;
    msg.ReplaceStringFlag( 'T', team->GetTeamName());
    //sprintf( msg, "%s %s", team->GetTeamName(), LANGUAGEPHRASE("crate_message_rocketwinians") );
    g_app->m_location->SetCurrentMessage( msg, _teamId  );

    TriggerSoundEvent( CrateRewardRocketDarwinians, g_zeroVector, 255 );
}

void Crate::RunRage( Vector3 const &_pos, int _teamId, int _taskId )
{
    int numFound;
    bool include[NUM_TEAMS];
    memset( include, false, sizeof(include) );
    include[_teamId] = true;
    g_app->m_location->m_entityGrid->GetNeighbours( s_neighbours, _pos.x, _pos.z, POWERUP_EFFECT_RANGE, &numFound, include);

    for( int i = 0; i < numFound; ++i )
    {
        WorldObjectId id = s_neighbours[i];
        Entity *entity = g_app->m_location->GetEntity( id );
        if( entity && entity->m_type == Entity::TypeDarwinian &&
            entity->m_id.GetTeamId() == _teamId )
        {
            ((Darwinian *)entity)->GiveRage( _taskId );
        }
    }

    Team *team = g_app->m_location->m_teams[ _teamId ];

    UnicodeString msg = LANGUAGEPHRASE("crate_message_rage");
    msg.ReplaceStringFlag( 'T', team->GetTeamName() );
    //sprintf( msg, "%s %s", team->GetTeamName(), LANGUAGEPHRASE("crate_message_rage") );

    g_app->m_location->SetCurrentMessage( msg, _teamId  );
    g_app->m_markerSystem->RegisterMarker_Fixed( _teamId, _pos, "icons/icon_rage.bmp" );

    TriggerSoundEvent( CrateRewardRage, g_zeroVector, 255 );
}

void Crate::RunSpaceShip()
{
#ifdef INCLUDE_CRATES_ADVANCED
    Vector3 pos;
    int worldSizeX = g_app->m_location->m_landscape.GetWorldSizeX();
    int worldSizeZ = g_app->m_location->m_landscape.GetWorldSizeZ();

    while( g_app->m_location->m_landscape.m_heightMap->GetValue( pos.x, pos.z ) < 10.0 )
    {
        pos.Set( FRAND(worldSizeX), 5000.0, FRAND(worldSizeZ) );
    }

    g_app->m_location->SpawnEntities( pos, g_app->m_location->GetFuturewinianTeamId(), -1, Entity::TypeSpaceShip, 1, g_zeroVector, 0.0 );

    g_app->m_location->SetCurrentMessage( LANGUAGEPHRASE("crate_message_spaceship") );

    TriggerSoundEvent( CrateRewardSpaceShip, g_zeroVector, 255 );

    s_spaceShipUsed = true;
#endif
}

void Crate::RunTriffids(Vector3 const &_pos, int _teamId )
{
#ifdef INCLUDE_CRATES_ADVANCED
    int numTriffids = 1 + syncrand() % 3;
    for( int i = 0; i < numTriffids; ++i )
    {
        Triffid triffidTemplate;
        triffidTemplate.m_pos = _pos;
        triffidTemplate.m_pos.x += syncsfrand(50.0);
        triffidTemplate.m_pos.z += syncsfrand(50.0);
        triffidTemplate.m_pos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( triffidTemplate.m_pos.x, triffidTemplate.m_pos.z );

        triffidTemplate.m_pitch = 0.6 + syncfrand(0.6);
        triffidTemplate.m_force = 60.0 + syncfrand(60.0);
        triffidTemplate.m_variance = syncfrand(0.5);
        triffidTemplate.m_reloadTime = 40.0 + syncfrand(60.0);

        triffidTemplate.m_front = (triffidTemplate.m_pos - _pos).Normalise();

        for( int s = 0; s < Triffid::NumSpawnTypes; ++s )
        {
            triffidTemplate.m_spawn[s] = true;
            if( s == Triffid::SpawnSpirits ||
                s == Triffid::SpawnEggs )
            {
                triffidTemplate.m_spawn[s] = false;
            }
        }

        Triffid *triffid = (Triffid *)Building::CreateBuilding( Building::TypeTriffid );
        triffid->Initialise( (Building *)&triffidTemplate );
        int id = g_app->m_location->m_buildings.PutData(triffid);
        triffid->m_id.Set( g_app->m_location->GetMonsterTeamId(), UNIT_BUILDINGS, -1, g_app->m_globalWorld->GenerateBuildingId() );

        triffid->SetDetail( 1 );
    }

	UnicodeString msg;
	GetNameTranslated( Crate::CrateRewardTriffids, msg );
	UnicodeString temp(LANGUAGEPHRASE("multiwinia_exclamation"));
	temp.ReplaceStringFlag(L'T', msg);
	msg = temp;

    g_app->m_location->SetCurrentMessage( msg );
    g_app->m_markerSystem->RegisterMarker_Fixed( _teamId, _pos, "icons/icon_triffid.bmp" );

    TriggerSoundEvent( CrateRewardTriffids, _pos, 255 );
#endif
}


void Crate::RunSlowDownTime()
{
#ifdef INCLUDE_CRATES_ADVANCED
    if( g_app->m_multiwinia->GameOver() )
        return;

    double newSpeed = 0.2 + syncfrand(0.2);
    g_gameTimer.RequestSpeedChange( newSpeed, 0.05, 15.0 );

    g_app->m_location->SetCurrentMessage( LANGUAGEPHRASE("crate_message_slowdown") );
#endif
}


void Crate::RunSpeedUpTime()
{
#ifdef INCLUDE_CRATES_ADVANCED
    if( g_app->m_multiwinia->GameOver() )
        return;

    double newSpeed = 1.3 + syncfrand(0.7);
    g_gameTimer.RequestSpeedChange( newSpeed, 0.05, 15.0 );

    g_app->m_location->SetCurrentMessage( LANGUAGEPHRASE("crate_message_speedup") );
#endif
}

void Crate::RunTheBeast( const Vector3 &_pos )
{
    Portal portalTemplate;
    portalTemplate.m_pos = _pos;

    Portal *portal = (Portal *)Building::CreateBuilding( Building::TypePortal );
    portal->Initialise( (Building *)&portalTemplate );
    int id = g_app->m_location->m_buildings.PutData(portal);
    portal->m_id.Set( g_app->m_location->GetMonsterTeamId(), UNIT_BUILDINGS, -1, g_app->m_globalWorld->GenerateBuildingId() );
    portal->SetDetail(1);

    g_app->m_location->SetCurrentMessage( LANGUAGEPHRASE("crate_message_thebeast") );
}


void Crate::ListSoundEvents( LList<char *> *_list )
{
    Building::ListSoundEvents( _list );

    _list->PutData( "Falling" );
    _list->PutData( "Land" );
    _list->PutData( "BeginCapture" );

    for( int i = 0; i < 5; ++i )
    {
        char name[256];
        sprintf( name, "Capturing_%dpc", i * 20 );
        _list->PutData( strdup(name) );
    }

    _list->PutData( "Captured" );
    _list->PutData( "LostCapture" );
    _list->PutData( "Expired" );
}


char *HashDouble( double value, char *buffer );


char *Crate::LogState( char *_message )
{
    static char s_result[1024];

    static char buf1[32], buf2[32], buf3[32], buf4[32], buf5[32];

    sprintf( s_result, "%sCRATE Id[%d] pos[%s,%s,%s], vel[%s] radius[%s]",
        (_message)?_message:"",
        m_id.GetUniqueId(),
        HashDouble( m_pos.x, buf1 ),
        HashDouble( m_pos.y, buf2 ),
        HashDouble( m_pos.z, buf3 ),
        HashDouble( m_vel.x + m_vel.y + m_vel.z, buf4 ),
        HashDouble( m_radius, buf5 ) );

    return s_result;
}
