#include "lib/universal_include.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "lib/debug_utils.h"
#include "lib/debug_render.h"
#include "lib/math_utils.h"
#include "lib/profiler.h"
#include "lib/resource.h"
#include "lib/shape.h"
#include "lib/texture_uv.h"
#include "lib/filesys/text_stream_readers.h"
#include "lib/language_table.h"
#include "lib/math/random_number.h"

#include "app.h"
#include "camera.h"
#include "main.h"
#include "particle_system.h"
#include "user_input.h"
#include "renderer.h"
#include "sound/soundsystem.h"
#include "team.h"
#include "unit.h"
#include "location.h"
#include "entity_grid.h"
#include "obstruction_grid.h"
#include "routing_system.h"
#include "level_file.h"

#include "worldobject/entity.h"
#include "worldobject/radardish.h"
#include "worldobject/bridge.h"
#include "worldobject/insertion_squad.h"
#include "worldobject/engineer.h"
#include "worldobject/virii.h"
#include "worldobject/insertion_squad.h"
#include "worldobject/lasertrooper.h"
#include "worldobject/egg.h"
#include "worldobject/sporegenerator.h"
#include "worldobject/lander.h"
#include "worldobject/tripod.h"
#include "worldobject/centipede.h"
#include "worldobject/airstrike.h"
#include "worldobject/spider.h"
#include "worldobject/darwinian.h"
#include "worldobject/officer.h"
#include "worldobject/armyant.h"
#include "worldobject/armour.h"
#include "worldobject/souldestroyer.h"
#include "worldobject/triffid.h"
#include "worldobject/ai.h"
#include "worldobject/laserfence.h"
#include "worldobject/shaman.h"
#include "worldobject/harvester.h"
#include "worldobject/nuke.h"
#include "worldobject/tentacle.h"
#include "worldobject/meteor.h"
#include "worldobject/tank.h"
#include "worldobject/spaceship.h"
#include "worldobject/dropship.h"



// ****************************************************************************
//  Class EntityBlueprint
// ****************************************************************************

char	*EntityBlueprint::m_names[Entity::NumEntityTypes];
double	 EntityBlueprint::m_stats[Entity::NumEntityTypes][Entity::NumStats];
int     Entity::s_entityPopulation = 0;


void EntityBlueprint::Initialise()
{
	TextReader *theFile = g_app->m_resource->GetTextReader("stats.txt");
	AppReleaseAssert(theFile && theFile->IsOpen(), "Couldn't open stats.txt");

    int entityIndex = 0;

    while( theFile->ReadLine() )
    {
		if (!theFile->TokenAvailable()) continue;		
        AppReleaseAssert(entityIndex < Entity::NumEntityTypes, "Too many entity blueprints defined");
		
        m_names[entityIndex] = strdup(theFile->GetNextToken());
		m_stats[entityIndex][0] = iv_atof(theFile->GetNextToken());
        m_stats[entityIndex][1] = iv_atof(theFile->GetNextToken());
        m_stats[entityIndex][2] = iv_atof(theFile->GetNextToken());

        ++entityIndex;
    }

	delete theFile;
}

char *EntityBlueprint::GetName( unsigned char _type )
{
    AppDebugAssert( _type < Entity::NumEntityTypes );
    return m_names[_type];
}

double EntityBlueprint::GetStat( unsigned char _type, int _stat )
{
    AppDebugAssert( _type < Entity::NumEntityTypes );
    AppDebugAssert( _stat < Entity::NumStats );

	if (_stat == Entity::StatSpeed) 
	{
		if (_type != Entity::TypeSpaceInvader)
			return m_stats[_type][_stat] * (1.0 + (double) g_app->m_difficultyLevel / 10.0);
	}

    return m_stats[_type][_stat];
}


std::vector<WorldObjectId> Entity::s_neighbours;


// ****************************************************************************
//  Class Entity
// ****************************************************************************

Entity::Entity()
:   WorldObject(),
    m_formationIndex(-1),
    m_dead(false),
    m_reloading(0.0),
    m_shape(NULL),
    m_justFired(false),
    m_radius(0.0),
    m_buildingId(-1),
    m_roamRange(0.0),
    m_inWater(-1.0),
    m_renderDamaged(false),
	m_routeId(-1),
	m_routeWayPointId(-1),
	m_routeTriggerDistance(10.0),
    m_aiTimer(0.0),
    m_destroyed(false)
{
    memset( m_stats, 0, NumStats * sizeof(m_stats[0]) );

    s_entityPopulation++;
}


Entity::~Entity()
{
    s_entityPopulation--;

#ifdef TRACK_SYNC_RAND
	void UntrackEntity( Entity *_entity );
	UntrackEntity( this );
#endif
}


void Entity::SetType( unsigned char _type )
{
    m_type = _type;

    for( int i = 0; i < NumStats; ++i )
    {
        m_stats[i] = EntityBlueprint::GetStat( m_type, i );		
    }
	

    m_reloading = syncfrand( m_stats[StatRate] );
}


bool Entity::ChangeHealth( int amount, int _damageType )
{
    if( !m_dead )
    {
        if( amount < 0 )
        {
            g_app->m_soundSystem->TriggerEntityEvent( this, "LoseHealth" );
        }

        if( m_stats[StatHealth] + amount <= 0 )
        {
            m_stats[StatHealth] = 100;
            m_dead = true;
            g_app->m_soundSystem->TriggerEntityEvent( this, "Die" );
            g_app->m_location->SpawnSpirit( m_pos, m_vel * 0.5, m_id.GetTeamId(), m_id );
        }
        else if( m_stats[StatHealth] + amount > 255 )
        {
            m_stats[StatHealth] = 255;
        }
        else
        {
            m_stats[StatHealth] += amount;    
        }
    }

    return true;
}

void Entity::RunAI( AI *_ai )
{
}

void Entity::Attack( Vector3 const &pos )
{
    if( m_reloading == 0.0 )
    {
		Vector3 toPos = pos;
		toPos.x += syncsfrand(15.0);
		toPos.z += syncsfrand(15.0);
		
		Vector3 fromPos = m_pos;
		fromPos.y += 3.0f;
		Vector3 velocity = (toPos - fromPos).SetLength(150.0f);
		g_app->m_location->FireLaser( fromPos, velocity, m_id.GetTeamId() );

        m_reloading = m_stats[StatRate];
        m_justFired = true;

        g_app->m_soundSystem->TriggerEntityEvent( this, "Attack" );
    }
}


bool Entity::AdvanceDead( Unit *_unit )
{
    int newHealth = m_stats[StatHealth];
    if( m_onGround ) newHealth -= 40 * SERVER_ADVANCE_PERIOD;
    else             newHealth -= 20 * SERVER_ADVANCE_PERIOD;
    
    if( newHealth <= 0 )
    {
        m_stats[StatHealth] = 0;        
        return true;
    }
    else
    {
        m_stats[StatHealth] = newHealth;
    }        

    return false;
}


int Entity::EnterTeleports( int _requiredId )
{
    LList<int> *buildings = g_app->m_location->m_obstructionGrid->GetBuildings( m_pos.x, m_pos.z );
    
    for( int i = 0; i < buildings->Size(); ++i )
    {
        int buildingId = buildings->GetData(i);
        if( _requiredId != -1 && _requiredId != buildingId )
        {
            // We are only permitted to enter building with id _requiredId
            continue;
        }

        Building *building = g_app->m_location->GetBuilding( buildingId );
        AppDebugAssert( building );
        
        if( building->m_type == Building::TypeRadarDish  )
        {
            RadarDish *radarDish = (RadarDish *) building;
            Vector3 entrancePos, entranceFront;
            radarDish->GetEntrance( entrancePos, entranceFront );
            double range = ( m_pos - entrancePos ).Mag();
            if( radarDish->ReadyToSend() &&                             
                range < 15.0 &&
                m_front * entranceFront < 0.0 )
            {
                WorldObjectId id(m_id);
                radarDish->EnterTeleport( id );
                g_app->m_soundSystem->TriggerEntityEvent( this, "EnterTeleport" );
                return buildingId;
            }
        }
        else if( building->m_type == Building::TypeBridge )
        {
            Bridge *bridge = (Bridge *) building;
            Vector3 entrancePos, entranceFront;
            if( bridge->GetEntrance( entrancePos, entranceFront ) )
            {
                double range = ( m_pos - bridge->m_pos ).Mag();
                if( bridge->ReadyToSend() &&
                    range < 25.0 &&
                    m_front * entranceFront < 0.0 )
                {
                    WorldObjectId id( m_id );
                    bridge->EnterTeleport( id );
                    g_app->m_soundSystem->TriggerEntityEvent( this, "EnterTeleport" );
                    return buildingId;
                }
            }
        }        
    }

    return -1;
}


void Entity::AdvanceInAir( Unit *_unit )
{
    m_vel += Vector3(0,-15.0,0) * SERVER_ADVANCE_PERIOD;
    m_pos += m_vel * SERVER_ADVANCE_PERIOD;
    PushFromObstructions(m_pos);
    
    if( !m_dead )
    {
        double groundLevel = g_app->m_location->m_landscape.m_heightMap->GetValue(m_pos.x, m_pos.z) + 0.1;
        if( m_pos.y <= groundLevel )
        {
            m_onGround = true;
            m_vel = g_zeroVector;
            m_pos.y = groundLevel + 0.1;
        }
        if( m_pos.y <= 0.0 /*seaLevel*/ )
        {
            if( m_inWater < 0.0 )
            {
                m_inWater = 0.0;
            }
            m_vel = g_zeroVector;
        }
    }
}


void Entity::AdvanceInWater( Unit *_unit )
{
    m_inWater += SERVER_ADVANCE_PERIOD;
    if( m_inWater > 10.0 /* && m_type != TypeInsertionSquadie */ )
    {
        ChangeHealth( -500 );
    }

    Vector3 targetPos;
    if( _unit )
    {
        targetPos = _unit->GetWayPoint();
        targetPos.y = 0.0;
        Vector3 offset = _unit->GetFormationOffset(Unit::FormationRectangle, m_id.GetIndex() );
        targetPos += offset;
    }

    if( targetPos != g_zeroVector )
    {
        Vector3 distance = targetPos - m_pos;
        distance.y = 0.0;
        m_vel = distance;
        m_vel.Normalise();
        m_front = m_vel;
        m_vel *= m_stats[StatSpeed] * 0.3;

        if (distance.Mag() < m_vel.Mag() * SERVER_ADVANCE_PERIOD)
        {                    
            m_vel = distance / SERVER_ADVANCE_PERIOD;
        }
    }

    m_vel.SetLength( 5.0 );
    m_pos += m_vel * SERVER_ADVANCE_PERIOD;
    m_pos.y = 0.0 - iv_sin(m_inWater * 3.0) - 4.0;

    double groundLevel = g_app->m_location->m_landscape.m_heightMap->GetValue(m_pos.x, m_pos.z);
    if( groundLevel > 0.0 )
    {
        m_inWater = -1;
    }
}


void Entity::Begin()
{
    g_app->m_soundSystem->TriggerEntityEvent( this, "Create" );

    if( m_shape )
    {
        m_centrePos = m_shape->CalculateCentre( g_identityMatrix34 );
        m_radius = m_shape->CalculateRadius( g_identityMatrix34, m_centrePos );
    }
}


bool Entity::Advance(Unit *_unit)
{    
    if( !m_enabled ) return false;
    if( m_destroyed ) return true;

    if( m_dead ) 
    {
        bool amIDead = AdvanceDead( _unit );
        if( amIDead ) 
		{
			return true;
		}
    }
    
    //m_pos = PushFromObstructions( m_pos );
    //m_pos = PushFromEachOther( m_pos );
        
    if( m_reloading > 0.0 )
    {
        m_reloading -= SERVER_ADVANCE_PERIOD;
        if( m_reloading < 0.0 ) m_reloading = 0.0;
    }

    double healthFraction = (double) m_stats[StatHealth] / (double) EntityBlueprint::GetStat( m_type, StatHealth );
    double timeIndex = g_gameTime + m_id.GetUniqueId() * 10;
    m_renderDamaged = ( frand(0.75) * (1.0 - iv_abs(iv_sin(timeIndex))*0.8) > healthFraction );

    if( m_inWater != -1 )   AdvanceInWater(_unit);

    double worldSizeX = g_app->m_location->m_landscape.GetWorldSizeX();
    double worldSizeZ = g_app->m_location->m_landscape.GetWorldSizeZ();
    if( m_pos.x < 0.0 ) m_pos.x = 0.0;
    if( m_pos.z < 0.0 ) m_pos.z = 0.0;
    if( m_pos.x >= worldSizeX ) m_pos.x = worldSizeX;
    if( m_pos.z >= worldSizeZ ) m_pos.z = worldSizeZ;
    
    if (_unit && !m_dead )
	{
		_unit->UpdateEntityPosition( m_pos, m_radius );
	}

	if( m_routeId != -1 )
	{
		FollowRoute();
	}
	
    return false;
}


void Entity::Render(double predictionTime)
{    
}


bool Entity::IsInView ()
{
    return true;
}


Vector3 Entity::PushFromEachOther( Vector3 const &_pos )
{
    Vector3 result = _pos;

    int numFound;
    g_app->m_location->m_entityGrid->GetNeighbours( s_neighbours, _pos.x, _pos.z, 2.0, &numFound );


    for( int i = 0; i < numFound; ++i )   
    {        
        WorldObjectId id = s_neighbours[i];
        if( !( id == m_id ) )
        {
            WorldObject *obj = g_app->m_location->GetEntity( id );
//            double distance = (obj->m_pos - thisPos).Mag();
//            while( distance < 1.0 )
//            {
//                Vector3 pushForce = (obj->m_pos - thisPos).Normalise() * 0.1;
//                if( obj->m_pos == thisPos ) pushForce = Vector3(0.0,0.0,1.0);
//                thisPos -= pushForce;                
//                distance = (obj->m_pos - thisPos).Mag();
//            }

            Vector3 diff = ( _pos - obj->m_pos );        
            double force = 0.1 / diff.Mag();
            diff.Normalise();        

            Vector3 thisDiff = ( diff * force );
            result += thisDiff;            
            
        }
    }

    result.y = g_app->m_location->m_landscape.m_heightMap->GetValue( result.x, result.z );
    return result;
}


Vector3 Entity::PushFromCliffs( Vector3 const &pos, Vector3 const &oldPos )
{
    Vector3 horiz = ( pos - oldPos );
    horiz.y = 0.0;
    double horizDistance = horiz.Mag();
    double gradient = (pos.y - oldPos.y) / horizDistance;

    Vector3 result = pos;

    if( gradient > 1.0 )
    {
        double distance = 0.0;
        while( distance < 50.0 )
        {
            double angle = distance * 2.0 * M_PI;
            Vector3 offset( iv_cos(angle) * distance, 0.0, iv_sin(angle) * distance );
            Vector3 newPos = result + offset;
            newPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( newPos.x, newPos.z );
            horiz = ( newPos - oldPos );
            horiz.y = 0.0;
            horizDistance = horiz.Mag();
            double newGradient = ( newPos.y - oldPos.y ) / horizDistance;
            if( newGradient < 1.0 )
            {
                result = newPos;
                break;
            }
            distance += 0.2;
        }
    }

    return result;
}


Vector3 Entity::PushFromObstructions( Vector3 const &pos, bool killem )
{
    Vector3 result = pos;    
    if( m_onGround )
    {
        result.y = g_app->m_location->m_landscape.m_heightMap->GetValue( result.x, result.z );
    }

    Matrix34 transform( m_front, g_upVector, result );

    //
    // Push from Water

    if( result.y <= 1.0 )
    {        
        double pushAngle = syncsfrand(1.0);
        double distance = 0.0;
        while( distance < 50.0 )
        {
            double angle = distance * pushAngle * M_PI;
            Vector3 offset( iv_cos(angle) * distance, 0.0, iv_sin(angle) * distance );
            Vector3 newPos = result + offset;
            double height = g_app->m_location->m_landscape.m_heightMap->GetValue( newPos.x, newPos.z );
            if( height > 1.0 )
            {
                result = newPos;
                result.y = height;
                break;
            }
            distance += 1.0;
        }
    }
    

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
            if( m_shape && building->DoesShapeHit( m_shape, transform ) ) hit = true;
            if( (!m_shape || m_type == TypeOfficer ) && building->DoesSphereHit( result, 1.0 ) ) hit = true;
            // cheap hack, but no point overriding the entire function for this one line
            if( !hit )
            {
                Vector3 oldPos = m_pos - m_vel * SERVER_ADVANCE_PERIOD;
                if( building->DoesRayHit( oldPos, m_front, (m_pos - oldPos).Mag() ) ) hit = true;
            }

            if( hit )
            {            
                if( building->m_type == Building::TypeLaserFence && killem &&
                    ((LaserFence *) building)->IsEnabled())
                {
                    if( !g_app->m_location->IsFriend(building->m_id.GetTeamId(), m_id.GetTeamId() ) )
                    {
                        ChangeHealth( -9999 );
                        ((LaserFence *) building)->Electrocute( m_pos );
                    }
                }
                else
                {               
                    Vector3 pushForce = (building->m_pos - result);
                    pushForce.y = 0.0f;
                    pushForce.SetLength(4.0f);
                    while( building->DoesSphereHit( result, 2.0f ) )
                    {
                        result -= pushForce;   
                        //result.y = g_app->m_location->m_landscape.m_heightMap->GetValue( result.x, result.z );
                    }
                }
            }
        }
    }

    return result;
}


static double s_nearPlaneStart;

void Entity::BeginRenderShadow()
{
    glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/glow.bmp" ) );
	glTexParameteri	( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri	( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
    glDisable       ( GL_CULL_FACE );
    glDepthMask     ( false );

    glEnable        ( GL_BLEND );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR );
    glColor4f       ( 0.6, 0.6, 0.6, 0.0 );

	s_nearPlaneStart = g_app->m_renderer->GetNearPlane();
	g_app->m_camera->SetupProjectionMatrix(s_nearPlaneStart * 1.05,
							 			   g_app->m_renderer->GetFarPlane());
}


void Entity::EndRenderShadow()
{
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glDisable       ( GL_BLEND );

    glDepthMask     ( true );
    glEnable        ( GL_CULL_FACE );
    glDisable       ( GL_TEXTURE_2D );   

	g_app->m_camera->SetupProjectionMatrix(s_nearPlaneStart,
								 		   g_app->m_renderer->GetFarPlane());
}


void Entity::RenderShadow( Vector3 const &_pos, double _size )
{
    Vector3 shadowPos = _pos;
    Vector3 shadowR = Vector3(_size,0,0);
    Vector3 shadowU = Vector3(0,0,_size);
        
    Vector3 posA = shadowPos - shadowR - shadowU;
    Vector3 posB = shadowPos + shadowR - shadowU;
    Vector3 posC = shadowPos + shadowR + shadowU;
    Vector3 posD = shadowPos - shadowR + shadowU;

    posA.y = g_app->m_location->m_landscape.m_heightMap->GetValue( posA.x, posA.z ) + 0.9;
    posB.y = g_app->m_location->m_landscape.m_heightMap->GetValue( posB.x, posB.z ) + 0.9;
    posC.y = g_app->m_location->m_landscape.m_heightMap->GetValue( posC.x, posC.z ) + 0.9;
    posD.y = g_app->m_location->m_landscape.m_heightMap->GetValue( posD.x, posD.z ) + 0.9;
    
    posA.y = max( posA.y, 1.0 );
    posB.y = max( posB.y, 1.0 );
    posC.y = max( posC.y, 1.0 );
    posD.y = max( posD.y, 1.0 );

    if( posA.y > _pos.y && posB.y > _pos.y && posC.y > _pos.y && posD.y > _pos.y )
    {
        // The object casting the shadow is beneath the ground
        return;
    }

    glBegin( GL_QUADS );
        glTexCoord2f( 0.0, 0.0 );     glVertex3dv     ( posA.GetData() );
        glTexCoord2f( 1.0, 0.0 );     glVertex3dv     ( posB.GetData() );
        glTexCoord2f( 1.0, 1.0 );     glVertex3dv     ( posC.GetData() );
        glTexCoord2f( 0.0, 1.0 );     glVertex3dv     ( posD.GetData() );
    glEnd();
}

Entity *Entity::NewEntity( int _troopType )
{
    Entity *entity = NULL;

    switch(_troopType)
    {
        case Entity::TypeLaserTroop:            entity = new LaserTrooper();        break;
        case Entity::TypeInsertionSquadie:      entity = new Squadie();             break;
        case Entity::TypeEngineer:              entity = new Engineer();            break;            
        case Entity::TypeVirii:                 entity = new Virii();               break;
        case Entity::TypeEgg:                   entity = new Egg();                 break;
        case Entity::TypeSporeGenerator:        entity = new SporeGenerator();      break;
        case Entity::TypeLander:                entity = new Lander();              break;
		case Entity::TypeTripod:				entity = new Tripod();				break;
        case Entity::TypeCentipede:             entity = new Centipede();           break;
        case Entity::TypeSpaceInvader:          entity = new SpaceInvader();        break;
		case Entity::TypeSpider:				entity = new Spider();				break;
        case Entity::TypeDarwinian:             entity = new Darwinian();           break;
        case Entity::TypeOfficer:               entity = new Officer();             break;
        case Entity::TypeArmyAnt:               entity = new ArmyAnt();             break;
        case Entity::TypeArmour:                entity = new Armour();              break;
        case Entity::TypeSoulDestroyer:         entity = new SoulDestroyer();       break;
        case Entity::TypeTriffidEgg:            entity = new TriffidEgg();          break;
        case Entity::TypeAI:                    entity = new AI();                  break;
		case Entity::TypeShaman:				entity = new Shaman();				break;
        case Entity::TypeHarvester:             entity = new Harvester();           break;
        case Entity::TypeNuke:                  entity = new Nuke();                break;
        case Entity::TypeTentacle:              entity = new Tentacle();            break;
        case Entity::TypeMeteor:                entity = new Meteor();              break;
        case Entity::TypeTank:                  entity = new Tank();                break;
        case Entity::TypeSpaceShip:             entity = new SpaceShip();           break;
        case Entity::TypeDropShip:              entity = new DropShip();            break;

        default:                                AppDebugAssert(false);
    }
    
    entity->m_id.GenerateUniqueId();

#ifdef TRACK_SYNC_RAND
	void TrackEntity( Entity *_entity );
    TrackEntity( entity );
#endif

    return entity;
}


int Entity::GetTypeId( char const *_typeName )
{
    for( int i = 0; i < NumEntityTypes; ++i )
    {
        if( stricmp( _typeName, GetTypeName(i) ) == 0 )
        {
            return i;
        }
    }
    return -1;
}


char *Entity::GetTypeName( int _troopType )
{
    static char *typeNames[NumEntityTypes] = {
                                                "InvalidType",
                                                "LaserTroop",
                                                "Engineer",
                                                "Virii",
                                                "Squadie",
                                                "Egg",
                                                "SporeGenerator",
                                                "Lander",
												"Tripod",
                                                "Centipede",
                                                "SpaceInvader",
												"Spider",
                                                "Darwinian",
                                                "Officer",
                                                "ArmyAnt",
                                                "Armour",
                                                "SoulDestroyer",
                                                "TriffidEgg",
                                                "AI",
												"Shaman",
                                                "Harvester",
                                                "Nuke",
                                                "Tentacle",
                                                "Meteor",
                                                "Tank",
                                                "SpaceShip",
                                                "DropShip"
                                                };   

    AppDebugAssert( _troopType >= 0 && _troopType < NumEntityTypes );
    return typeNames[ _troopType ];
}


void Entity::GetTypeNameTranslated ( int _troopType, UnicodeString &_dest )
{
    char *typeName = GetTypeName( _troopType );

    char stringId[256];
    sprintf( stringId, "entityname_%s", typeName );

    if( ISLANGUAGEPHRASE( stringId ) )
    {
        _dest = LANGUAGEPHRASE( stringId );
    }
    else
    {
        _dest = UnicodeString(typeName);
    }
}


void Entity::ListSoundEvents( LList<char *> *_list )
{
    _list->PutData( "Create" );
    _list->PutData( "Attack" );
    _list->PutData( "LoseHealth" );
    _list->PutData( "EnterTeleport" );
    _list->PutData( "ExitTeleport" );
    _list->PutData( "Die" );
    _list->PutData( "Selected" );
}


bool Entity::RayHit(Vector3 const &_rayStart, Vector3 const &_rayDir)
{
	if (m_shape)
	{
		RayPackage package(_rayStart, _rayDir);
		Matrix34 mat(m_front, g_upVector, m_pos);
		return m_shape->RayHit(&package, mat);
	}
	else
	{
		return RaySphereIntersection(_rayStart, _rayDir, m_pos, 5.0);
	}
}


void Entity::DirectControl( TeamControls const& _teamControls )
{
}

bool Entity::IsSelectable()
{
    return false;
}

Vector3 Entity::GetCameraFocusPoint()
{
	return m_pos + m_vel;
}

void Entity::SetWaypoint( Vector3 const _waypoint )
{
}

void Entity::FollowRoute()
{
	AppDebugAssert(m_routeId != -1);
	Route *route = g_app->m_location->m_levelFile->GetRoute(m_routeId);
    
    if( g_app->Multiplayer() ) route = g_app->m_location->m_routingSystem.GetRoute( m_routeId );

	if(!route)
    {
        m_routeId = -1;
        return;
    }

	if (m_routeWayPointId == -1)
	{
		m_routeWayPointId = 0;
	}

    WayPoint *waypoint = route->m_wayPoints.GetData(m_routeWayPointId);
    if( !waypoint )
    {
        m_routeId = -1;
        return;
    }

    SetWaypoint ( waypoint->GetPos() );
	Vector3 targetVect = waypoint->GetPos() - m_pos;

	m_spawnPoint = waypoint->GetPos();

    if (waypoint->m_type != WayPoint::TypeBuilding &&
        targetVect.Mag() < m_routeTriggerDistance )
	{

		m_routeWayPointId++;
		if (m_routeWayPointId >= route->m_wayPoints.Size())
		{
			m_routeWayPointId = -1;
			m_routeId = -1;
		}
	}

    //
    // If its a building instead of a 3D pos, this unit will never
    // get to the next waypoint.  A new unit is created when the unit
    // enters the teleport, and taht new unit will automatically
    // continue towards the next waypoint instead.
    //
}


char *HashDouble( double value, char *buffer );


char *Entity::LogState( char *_message )
{
    static char s_result[1024];

    static char buf1[32], buf2[32], buf3[32], buf4[32], buf5[32];

    sprintf( s_result, "%sENT Type[%s] Id[%d] pos[%s,%s,%s], vel[%s] front[%s]",
                        (_message)?_message:"",
                        GetTypeName(m_type),
                        m_id.GetUniqueId(),
                        HashDouble( m_pos.x, buf1 ),
                        HashDouble( m_pos.y, buf2 ),
                        HashDouble( m_pos.z, buf3 ),
                        HashDouble( m_vel.x + m_vel.y + m_vel.z, buf4 ),
                        HashDouble( m_front.x + m_front.y + m_front.z, buf5 ) );

    return s_result;
}

void Entity::SetShape(char *_shapeName, bool _colourAll )
{
    char filename[256];
    Team *team = g_app->m_location->m_teams[m_id.GetTeamId()];
    int colourId = team->m_lobbyTeam->m_colourId;
    int groupId = team->m_lobbyTeam->m_coopGroupPosition;
    
	if( g_app->IsSinglePlayer() )
	{
		sprintf( filename, "%s", _shapeName  );
	}
	else
	{
		sprintf( filename, "%s_%d_%d", _shapeName, colourId, groupId  );
	}
    m_shape = g_app->m_resource->GetShape(filename, false);
    if( !m_shape )
    {
        SetupTeamShape( _shapeName, _colourAll );        
    }
}

void Entity::ConvertFragmentColours ( ShapeFragment *_frag, RGBAColour _col, bool _convertAll )
{
    RGBAColour *newColours = new RGBAColour[_frag->m_numColours];
    for( int i = 0; i < _frag->m_numColours; ++i )
    {
        RGBAColour col(_frag->m_colours[i].r, _frag->m_colours[i].g, _frag->m_colours[i].b );
        // if the colour is mostly red, assume its safe to be converted
        bool convert = col.r > col.g * 2 &&
                       col.r > col.b * 2;
        if( !convert ) convert = _convertAll;
        //if( !convert ) convert = col.r + col.g > col.b * 2.0;

        if( convert )
        {
            double totalColour = col.r + col.g + col.b;
            double totalTeamCol = _col.r + _col.g + _col.b;
            double ratio = totalColour / totalTeamCol;
            /*double r_factor = (double)col.r / totalColour;
            double g_factor = (double)col.g / totalColour;
            double b_factor = (double)col.b / totalColour;*/

            if( _convertAll )
            {
                newColours[i].r = _col.r;
                newColours[i].g = _col.g;
                newColours[i].b = _col.b;
            }
            else
            {
                newColours[i].r = _col.r * ratio;
                newColours[i].g = _col.g * ratio;
                newColours[i].b = _col.b * ratio;
            }            

            // scale colours down - colours on 3d models appear brighter than 2d equivalents due to lighting
            newColours[i] *= 0.7;
        }
        else
        {
            newColours[i].r = _frag->m_colours[i].r;        
            newColours[i].g = _frag->m_colours[i].g;
            newColours[i].b = _frag->m_colours[i].b;
        }

    }
    _frag->RegisterColours( newColours, _frag->m_numColours );
}

void Entity::ConvertShapeColoursToTeam( Shape *_shape, int _teamId, bool _convertAll )
{
    if( _teamId == 255 ) return;

    RGBAColour teamCol = g_app->m_location->m_teams[_teamId]->m_colour;
	if( g_app->m_location->m_teams[_teamId]->m_lobbyTeam->m_colourId == 0 &&
		stricmp(_shape->m_name, "data/shapes/darwinian.shp") == 0 )
	{
		teamCol *= 0.5f;
	}
    ConvertFragmentColours( _shape->m_rootFragment, teamCol, _convertAll );
    for( int i = 0; i < _shape->m_rootFragment->m_childFragments.Size(); ++i )
    {
        ConvertFragmentColours( _shape->m_rootFragment->m_childFragments[i], teamCol, _convertAll );
    }

    _shape->BuildDisplayList();
}

void Entity::SetupTeamShape( char *_name, bool _convertAll )
{
    if( g_app->IsSinglePlayer() || m_id.GetTeamId() == 255 )
    {
        // LEANDER : Does this crash? It _should_ stop objects being coloured to the team colour in SP...
        m_shape = g_app->m_resource->GetShape( _name, false );
        if( !m_shape )
        {
            m_shape = g_app->m_resource->GetShapeCopy( _name, false, false );
            g_app->m_resource->AddShape( m_shape, _name );
        }
    }
    else
    {
        RGBAColour teamCol = g_app->m_location->m_teams[m_id.GetTeamId()]->m_colour;
        char filename[512];
        sprintf( filename, "%s_%d_%d_%d_%d", _name, (int)teamCol.r, (int)teamCol.g, (int)teamCol.b, (int)teamCol.a );
        m_shape = g_app->m_resource->GetShape( filename, false );
        if( !m_shape )
        {
            m_shape = g_app->m_resource->GetShapeCopy( _name, false, false );
            ConvertShapeColoursToTeam( m_shape, m_id.GetTeamId(), _convertAll );
            g_app->m_resource->AddShape( m_shape, filename );
        }
    }
}

bool Entity::EntityIsBlocked(int _entityId)
{
#ifdef BLOCK_OLD_DARWINIA_OBJECTS
    return( _entityId == Entity::TypeAI ||
            _entityId == Entity::TypeArmyAnt ||
            _entityId == Entity::TypeDropShip ||
            _entityId == Entity::TypeEgg ||
            _entityId == Entity::TypeLander ||
            _entityId == Entity::TypeLaserTroop ||
            _entityId == Entity::TypeMeteor ||
            _entityId == Entity::TypeNuke ||
            _entityId == Entity::TypeOfficer ||
            _entityId == Entity::TypeShaman ||
            _entityId == Entity::TypeSpaceInvader ||
            _entityId == Entity::TypeTentacle ||
            _entityId == Entity::TypeTriffidEgg ||
            _entityId == Entity::TypeTripod ||
            _entityId == Entity::TypeInvalid ||
            _entityId == Entity::TypeSpaceShip);
#endif

    return false;
}