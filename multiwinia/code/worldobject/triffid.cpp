#include "lib/universal_include.h"
#include "lib/filesys/text_file_writer.h"
#include "lib/resource.h"
#include "lib/shape.h"
#include "lib/math_utils.h"
#include "lib/filesys/text_stream_readers.h"
#include "lib/debug_render.h"
#include "lib/text_renderer.h"
#include "lib/profiler.h"
#include "lib/language_table.h"
#include "lib/hi_res_time.h"
#include "lib/math/random_number.h"

#include "sound/soundsystem.h"

#include "worldobject/triffid.h"
#include "worldobject/souldestroyer.h"
#include "worldobject/virii.h"

#include "app.h"
#include "renderer.h"
#include "location.h"
#include "explosion.h"
#include "team.h"
#include "main.h"
#include "particle_system.h"
#include "location_editor.h"
#include "entity_grid.h"
#include "unit.h"
#include "multiwinia.h"


Triffid::Triffid()
:   Building(),
    m_launchPoint(NULL),
    m_stem(NULL),
    m_timerSync(0.0),
    m_pitch( 0.6 ),
    m_force( 250.0 ),
    m_variance( M_PI / 8.0 ),
    m_reloadTime(60.0),
    m_size(1.0),
    m_damage(50.0),
    m_useTrigger(0),
    m_triggerRadius(100.0),
    m_triggered(false),
    m_triggerTimer(0.0),
    m_numEggs(3)
{
    m_type = TypeTriffid;
    
    SetShape( g_app->m_resource->GetShape("triffidhead.shp") );

    const char launchPointName[] = "MarkerLaunchPoint";
    m_launchPoint = m_shape->m_rootFragment->LookupMarker( launchPointName );
    AppReleaseAssert( m_launchPoint, "Triffid: Can't get Marker(%s) from shape(%s), probably a corrupted file\n", launchPointName, m_shape->m_name );

    const char stemName[] = "MarkerTriffidStem";
    m_stem = m_shape->m_rootFragment->LookupMarker( stemName );
    AppReleaseAssert( m_stem, "Triffid: Can't get Marker(%s) from shape(%s), probably a corrupted file\n", stemName, m_shape->m_name );

    m_triggerTimer = syncfrand(5.0);

    memset( m_spawn, 0, NumSpawnTypes * sizeof(bool) );
}


Matrix34 Triffid::GetHead( double _predictionTime )
{
    Vector3 _pos = m_pos + g_upVector * m_size * 30.0;
    Vector3 _front = m_front;
    Vector3 _up = g_upVector;

    double timeNow = GetNetworkTime();
    if( g_app->m_editing ) timeNow = GetHighResTime();
    double timer = timeNow + _predictionTime + m_id.GetUniqueId() * 10.0;

    if( m_damage > 0.0 )
    {
        _front.RotateAroundY( iv_sin(timer) * m_variance * 0.f );
        Vector3 right = _front ^ _up;

        double pitchVariation = 1.0 + iv_sin(timer) * 0.1;
        _front.RotateAround( right * m_pitch * 0.5 * pitchVariation );

        _pos += Vector3( iv_sin(timer/1.3)*4, iv_sin(timer/1.5), iv_sin(timer/1.2)*3 );

        double justFiredFraction = ( m_timerSync - GetNetworkTime() ) / m_reloadTime;
        justFiredFraction = min( justFiredFraction, 1.0 );
        justFiredFraction = max( justFiredFraction, 0.0 );
        justFiredFraction = iv_pow( justFiredFraction, 5 );
    
        _pos -= _front * justFiredFraction * 10.0;
    }
    else
    {
        // We are on fire, so thrash around a lot
        Vector3 right = _front ^ _up;
        _front = g_upVector;
        _up = right ^ _front;        

        _front.RotateAround( right * iv_sin(timer*5.0) * m_variance );
        _up.RotateAroundY( iv_sin(timer*3.5) * m_variance );
        
        _pos += Vector3( iv_sin(timer*3.3)*8, iv_sin(timer*4.5), iv_sin(timer*3.2)*6 );

    }

    Matrix34 result( _front, _up, _pos );

	// Ordering important for sync
	double size = iv_sin(timer);
	size *= 0.1;
	size += 1.0;
	size *= m_size;

    result.f *= size;
    result.u *= size;
    result.r *= size;
    
    return result;
}


bool Triffid::DoesRayHit(Vector3 const &_rayStart, Vector3 const &_rayDir, 
                         double _rayLen, Vector3 *_pos, Vector3 *_norm)
{
    Matrix34 mat = GetHead(0.0);

	RayPackage ray(_rayStart, _rayDir, _rayLen);

    return m_shape->RayHit(&ray, mat, true);
}


    
void Triffid::Render( double _predictionTime )
{
    Matrix34 mat = GetHead(_predictionTime);

    //RenderArrow( m_pos, mat.pos, 1.0, RGBAColour(100,0,0,255) );

    Vector3 stemPos = m_stem->GetWorldMatrix(mat).pos;
    Vector3 midPoint = mat.pos + ( stemPos - mat.pos ).SetLength(10.0);
    Vector3 midPoint2 = midPoint - Vector3(0,20,0);
    glColor4f( 1.0, 1.0, 0.5, 1.0 );
    glLineWidth( 2.0 );
    glDisable( GL_TEXTURE_2D );
    glBegin( GL_LINES );
        glVertex3dv( mat.pos.GetData() );
        glVertex3dv( midPoint.GetData() );
        glVertex3dv( midPoint.GetData() );
        glVertex3dv( m_pos.GetData() );
    glEnd();

    //
    // If we are damaged, flicked in and out based on our health

    if( m_renderDamaged && !g_app->m_editing && m_damage > 0.0 )
    {
        double timeIndex = g_gameTime + m_id.GetUniqueId() * 10;
        double thefrand = frand();
        if      ( thefrand > 0.7 ) mat.f *= ( 1.0 - iv_sin(timeIndex) * 0.5 );
        else if ( thefrand > 0.4 ) mat.u *= ( 1.0 - iv_sin(timeIndex) * 0.2 );    
        else                        mat.r *= ( 1.0 - iv_sin(timeIndex) * 0.5 );
        glEnable( GL_BLEND );
        glBlendFunc( GL_ONE, GL_ONE );
    }

    glEnable( GL_NORMALIZE );

    m_shape->Render( _predictionTime, mat );

    if( m_triggered && GetNetworkTime() > m_timerSync - m_reloadTime * 0.25 )
    {        
        Matrix34 launchMat = m_launchPoint->GetWorldMatrix(mat);
        Shape *eggShape = g_app->m_resource->GetShape( "triffidegg.shp" );
        Matrix34 eggMat( launchMat.u, -launchMat.f, launchMat.pos );
        eggMat.f *= m_size;
        eggMat.u *= m_size;
        eggMat.r *= m_size;
        eggShape->Render( _predictionTime, eggMat );
    }

    glDisable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    
    glDisable( GL_NORMALIZE );
}


void Triffid::RenderAlphas( double _predictionTime )
{
    if( g_app->m_editing )
    {
        glColor4f( 1.0, 0.0, 0.0, 1.0 );
        glLineWidth( 1.0 );

        Matrix34 headMat = GetHead(_predictionTime);
        Matrix34 mat( m_front, g_upVector, headMat.pos );
        Matrix34 launchMat = m_launchPoint->GetWorldMatrix(mat);
    
        Vector3 point1 = launchMat.pos;
        
        Vector3 angle = launchMat.f;
        angle.HorizontalAndNormalise();
        angle.RotateAroundY( m_variance*0.5 );
        Vector3 right = angle ^ g_upVector;
        angle.RotateAround( right * m_pitch * 0.5 );        
        Vector3 point2 = point1 + angle * m_force * 3.0;

        angle = launchMat.f;
        angle.HorizontalAndNormalise();
        angle.RotateAroundY( m_variance * -0.5 );
        right = angle ^ g_upVector;
        angle.RotateAround( right * m_pitch * 0.5 );
        Vector3 point3 = point1 + angle * m_force * 3.0;

        glBegin( GL_LINE_LOOP );
            glVertex3dv( point1.GetData() );
            glVertex3dv( point2.GetData() );
            glVertex3dv( point3.GetData() );
        glEnd();


        //
        // Create a fake egg and plot its course
        // Render our trigger location

#ifdef LOCATION_EDITOR        
        if( g_app->m_locationEditor->m_mode == LocationEditor::ModeBuilding &&
            g_app->m_locationEditor->m_selectionId == m_id.GetUniqueId() )
        {
            Vector3 velocity = headMat.f;
            velocity.SetLength( m_force * m_size );
            TriffidEgg egg;
            egg.m_pos = headMat.pos;
            egg.m_vel = velocity;
            egg.m_front = headMat.f;
        
            glColor4f( 1.0, 1.0, 1.0, 1.0 );
            glLineWidth( 2.0 );
            glBegin( GL_LINES );
            while( true )
            {
                glVertex3dv( egg.m_pos.GetData() );
                egg.Advance( NULL );
                glVertex3dv( egg.m_pos.GetData() );
            
                if( egg.m_vel.Mag() < 20.0 ) break;
            }
            glEnd();

            if( m_useTrigger )
            {
                Vector3 triggerPos = m_pos + m_triggerLocation;
                int numSteps = 20;
                glBegin( GL_LINE_LOOP );
                glLineWidth( 1.0 );
                glColor4f( 1.0, 0.0, 0.0, 1.0 );
                for( int i = 0; i < numSteps; ++i )
                {
                    double angle = 2.0 * M_PI * (double)i / (double) numSteps;
                    Vector3 thisPos = triggerPos + Vector3( iv_sin(angle)*m_triggerRadius, 0.0,
                                                            iv_cos(angle)*m_triggerRadius );
                    thisPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( thisPos.x, thisPos.z );
                    thisPos.y += 10.0;
                    glVertex3dv( thisPos.GetData() );
                }
                glEnd();

                g_editorFont.DrawText3DCentre( triggerPos+Vector3(0,50,0), 10.0, "UseTrigger: %d", m_useTrigger );
            }
        }
#endif
        
    }
}


void Triffid::Damage( double _damage )
{
    Building::Damage( _damage );

    bool dead = (m_damage <= 0.0);

    m_damage += _damage;

    if( m_damage <= 0.0 && !dead )
    {
        g_app->m_soundSystem->TriggerBuildingEvent( this, "Burn" );
    }
}

void Triffid::Launch()
{
    //
    // Determine what sort of egg to launch

    LList<int> possibleSpawns;
    for( int i = 0; i < NumSpawnTypes; ++i )
    {
        if( m_spawn[i] ) possibleSpawns.PutData(i);
    }

    if( possibleSpawns.Size() == 0 )
    {
        // Can't spawn anything
        return;
    }

    int spawnIndex = syncrand() % possibleSpawns.Size();
    int spawnType = possibleSpawns[ spawnIndex ];


    //
    // Fire the egg

    Matrix34 mat = GetHead(0.0);
    Matrix34 launchMat = m_launchPoint->GetWorldMatrix(mat);    
    Vector3 velocity = launchMat.f;

    velocity.SetLength( m_force * m_size * ( 1.0 + syncsfrand(0.2) ) );

    WorldObjectId wobjId = g_app->m_location->SpawnEntities( launchMat.pos, m_id.GetTeamId(), -1, Entity::TypeTriffidEgg, 1, velocity, 0.0 );
    TriffidEgg *triffidEgg = (TriffidEgg *) g_app->m_location->GetEntitySafe( wobjId, Entity::TypeTriffidEgg );
    if( triffidEgg ) 
    {
        triffidEgg->m_spawnType = spawnType;
        triffidEgg->m_size = 1.0 + syncsfrand(0.3);
        triffidEgg->m_spawnPoint = m_pos + m_triggerLocation;
        triffidEgg->m_roamRange = m_triggerRadius;

        if( g_app->Multiplayer() )
        {
            triffidEgg->SetTimer( 4.0 + syncfrand(4.0) );
        }
    }

    g_app->m_soundSystem->TriggerBuildingEvent( this, "LaunchEgg" );
}


void Triffid::Initialise( Building *_template )
{
    Building::Initialise( _template );

    Triffid *triffid = (Triffid *) _template;
    
    m_size = triffid->m_size;
    m_pitch = triffid->m_pitch;
    m_force = triffid->m_force;
    m_variance = triffid->m_variance;
    m_reloadTime = triffid->m_reloadTime;
    
    m_useTrigger = triffid->m_useTrigger;
    m_triggerLocation.x = triffid->m_triggerLocation.x;
    m_triggerLocation.z = triffid->m_triggerLocation.z;
    m_triggerRadius = triffid->m_triggerRadius;

    m_triggerLocation.y = g_app->m_location->m_landscape.m_heightMap->GetValue( m_triggerLocation.x, m_triggerLocation.z );

    memcpy( m_spawn, triffid->m_spawn, NumSpawnTypes*sizeof(bool) );

    double timeAdd = syncrand() % (int) m_reloadTime;
    timeAdd += 10.0;
    m_timerSync = GetNetworkTime() + timeAdd;    
}


bool Triffid::Advance()
{
    //
    // Burn if we have been damaged

    if( m_damage <= 0.0 )
    {
        Matrix34 headMat = GetHead(0.0);
        Vector3 fireSpawn = headMat.pos;
        fireSpawn += Vector3( sfrand(10.0*m_size), 
                              sfrand(10.0*m_size),
                              sfrand(10.0*m_size) );
        double fireSize = 100.0 + syncsfrand(100.0*m_size);
        g_app->m_particleSystem->CreateParticle( fireSpawn, g_zeroVector, Particle::TypeFire, fireSize );
        
        fireSpawn = m_pos + Vector3( sfrand(10.0*m_size), 
                                     sfrand(10.0*m_size),
                                     sfrand(10.0*m_size) );
        g_app->m_particleSystem->CreateParticle( fireSpawn, g_zeroVector, Particle::TypeFire, fireSize );
        
        if( frand(100.0) < 10.0 )
        {
            g_app->m_particleSystem->CreateParticle( fireSpawn, g_zeroVector, Particle::TypeExplosionDebris );
        }

        m_size -= 0.006;
        if( m_size <= 0.3 )
        {
            g_explosionManager.AddExplosion( m_shape, headMat );
            return true;
        }
        m_timerSync -= 0.5;
    }


    //
    // Look in our trigger area if required

    bool oldTriggered = m_triggered;

    if( m_useTrigger == 0 || m_damage <= 0.0 )
    {
        m_triggered = true;
    }

    if( m_useTrigger > 0 && GetNetworkTime() > m_triggerTimer )
    {
        START_PROFILE( "CheckTrigger" );
        Vector3 triggerPos = m_pos + m_triggerLocation;
        bool enemiesFound = g_app->m_location->m_entityGrid->AreEnemiesPresent( triggerPos.x, triggerPos.z, m_triggerRadius, m_id.GetTeamId() );
        m_triggered = enemiesFound;
        m_triggerTimer = GetNetworkTime() + 5.0;
        END_PROFILE(  "CheckTrigger" );
    }

    
    //
    // If we have just triggered, start our random timer

    if( !oldTriggered && m_triggered )
    {
        double timeAdd = syncrand() % (int) m_reloadTime;
        timeAdd += 10.0;
        m_timerSync = GetNetworkTime() + timeAdd;    
    }


    //
    // Launch an egg if it is time
    
    if( m_triggered && GetNetworkTime() > m_timerSync )
    {
        if( g_app->Multiplayer() )
        {
            if( m_numEggs > 0 )
            {
                Launch();
                m_numEggs--;
                if( m_numEggs <= 0 )
                {
                    Damage(-100);
                }
            }
        }
        else
        {
            Launch();
        }

        m_timerSync = GetNetworkTime() + m_reloadTime * ( 1.0 + syncsfrand(0.1) );
    }


    //
    // Flicker if we are damaged

    double healthFraction = (double) m_damage / 50.0;
    double timeIndex = g_gameTime + m_id.GetUniqueId() * 10;
    m_renderDamaged = ( frand(0.75) * (1.0 - iv_abs(iv_sin(timeIndex))*1.2) > healthFraction );

    return false;
}


void Triffid::Read( TextReader *_in, bool _dynamic )
{
    Building::Read( _in, _dynamic );

    m_size          = iv_atof( _in->GetNextToken() );
    m_pitch         = iv_atof( _in->GetNextToken() );
    m_force         = iv_atof( _in->GetNextToken() );
    m_variance      = iv_atof( _in->GetNextToken() );
    m_reloadTime    = iv_atof( _in->GetNextToken() );
    m_useTrigger    = atoi( _in->GetNextToken() );
    
    m_triggerLocation.x = iv_atof(_in->GetNextToken() );
    m_triggerLocation.z = iv_atof(_in->GetNextToken() );
    m_triggerRadius     = iv_atof(_in->GetNextToken() );

    for( int i = 0; i < NumSpawnTypes; ++i )
    {
        if( _in->TokenAvailable() )
        {
            m_spawn[i] = ( atoi(_in->GetNextToken()) == 1 );
        }
        else
        {
            m_spawn[i] = false;
        }
    }
}


void Triffid::Write( TextWriter *_out )
{
    Building::Write( _out );

    _out->printf( "%-6.2f %-6.2f %-6.2f %-6.2f %-6.2f ", m_size, m_pitch, m_force, m_variance, m_reloadTime );
    _out->printf( "%d %-8.2f %-8.2f %-6.2f ", m_useTrigger, m_triggerLocation.x, m_triggerLocation.z, m_triggerRadius );
    
    for( int i = 0; i < NumSpawnTypes; ++i )
    {
        if( m_spawn[i] )    _out->printf( "1 " );
        else                _out->printf( "0 " );
    }
}


char *Triffid::GetSpawnName( int _spawnType )
{
    static char *names[NumSpawnTypes] = {  
                                            "SpawnVirii",
                                            "SpawnCentipedes",
                                            "SpawnSpider",
                                            "SpawnSpirits",
                                            "SpawnEggs",
                                            "SpawnTriffidEggs",
                                            "SpawnDarwinians",
                                            "SpawnAnts",
                                            "SpawnSoulDestroyer",
                                            "SpawnSporeGenerator"
                                         };

    return names[ _spawnType ];
}


void Triffid::GetSpawnNameTranslated( int _spawnType, UnicodeString& _dest )
{
    char *spawnName = GetSpawnName( _spawnType );

    char stringId[256];
    sprintf( stringId, "spawnname_%s", spawnName );

	if( ISLANGUAGEPHRASE( stringId ) )
    {
        _dest = LANGUAGEPHRASE( stringId );
    }
    else
    {
        _dest = UnicodeString(spawnName);
    }
}


void Triffid::ListSoundEvents( LList<char *> *_list )
{
    Building::ListSoundEvents( _list );

    _list->PutData( "LaunchEgg" );
    _list->PutData( "Burn" );
}


// ============================================================================


TriffidEgg::TriffidEgg()
:   Entity(),
    m_force(1.0),
    m_spawnType(Triffid::SpawnVirii),
    m_size(1.0)
{
    SetType( TypeTriffidEgg );
    
    m_up = g_upVector;
    //m_shape = g_app->m_resource->GetShape( "triffidegg.shp" );

    m_life = 20.0 + syncfrand(10.0);
    m_timerSync = GetNetworkTime() + m_life;
}

void TriffidEgg::Begin()
{
    SetShape( "triffidegg.shp", true );
    Entity::Begin();
}


bool TriffidEgg::ChangeHealth( int _amount, int _damageType )
{
    if( _damageType == Entity::DamageTypeLaser ) return false;

    bool dead = m_dead;
    Entity::ChangeHealth( _amount );

    if( m_dead && !dead )
    {
        // We just died
        Matrix34 transform( m_front, m_up, m_pos );
        transform.f *= m_size;
        transform.u *= m_size;
        transform.r *= m_size;
        g_explosionManager.AddExplosion( m_shape, transform );         
    }

    return true;
}


void TriffidEgg::Spawn()
{
    int teamId = m_id.GetTeamId();

    switch( m_spawnType )
    {
        case Triffid::SpawnSoulDestroyer:
        {
            if(( g_app->m_multiwinia->m_gameType != Multiwinia::GameTypeAssault &&
                SoulDestroyer::s_numSoulDestroyers < 2 ) || g_app->IsSinglePlayer() )
            {
                if( g_app->Multiplayer() ) teamId = g_app->m_location->GetMonsterTeamId();
                WorldObjectId id = g_app->m_location->SpawnEntities( m_pos, teamId, -1, TypeSoulDestroyer, 1, g_zeroVector, 0.0f, 200.0f );
                break;
            }
        }
        // deliberate fall through

        case Triffid::SpawnVirii:    
        {
            int numVirii = 5 + syncrand() % 5;
            if( g_app->Multiplayer() )
            {
                numVirii = 20 + syncrand() % 20;
                if( Virii::s_viriiPopulation[teamId] >= VIRII_POPULATIONLIMIT )
                {
                    numVirii = 0;
                }
            }
            g_app->m_location->SpawnEntities( m_pos, teamId, -1, TypeVirii, numVirii, g_zeroVector, 0.0, 100.0 );
            break;
        }
    
        case Triffid::SpawnCentipede:
        {
            int size = 5 + syncrand() % 5;
            if( g_app->Multiplayer() )
            {
                size = 5 + syncrand() % 10;
            }
            Team *team = g_app->m_location->m_teams[ m_id.GetTeamId() ];
            int unitId;
            Unit *unit = team->NewUnit( TypeCentipede, size, &unitId, m_pos );
            g_app->m_location->SpawnEntities( m_pos, teamId, unitId, TypeCentipede, size, g_zeroVector, 0.0, 200.0 );
            break;
        }

        case Triffid::SpawnSpider:
        {
            WorldObjectId id = g_app->m_location->SpawnEntities( m_pos, teamId, -1, TypeSpider, 1, g_zeroVector, 0.0, 150.0 );
            break;
        }

        case Triffid::SpawnSpirits:
        {
            int numSpirits = 5 + syncrand() % 5;
            for( int i = 0; i < numSpirits; ++i )
            {
                Vector3 vel(SFRAND(1), 0.0, SFRAND(1) );
                vel.SetLength( 20.0 + syncfrand(20.0) );
                g_app->m_location->SpawnSpirit( m_pos, vel, teamId, WorldObjectId() );
            }
            break;
        }

        case Triffid::SpawnEggs:
        {
            int numEggs = 5 + syncrand() % 5;
            for( int i = 0; i < numEggs; ++i )
            {
                Vector3 vel = g_upVector + Vector3(SFRAND(1), 0.0, SFRAND(1) );
                vel.SetLength( 20.0 + syncfrand(20.0) );
                g_app->m_location->SpawnEntities( m_pos, teamId, -1, TypeEgg, 1, vel, 0.0, 0.0 );
            }
            break;
        }

        case Triffid::SpawnTriffidEggs:
        {
            int numEggs = 2 + syncrand() % 3;
            for( int i = 0; i < numEggs; ++i )
            {
                Vector3 vel(SFRAND(1), 0.5, SFRAND(1) );
				vel.y += syncfrand(1);
                vel.SetLength( 75.0 + syncfrand(50.0) );
                WorldObjectId id = g_app->m_location->SpawnEntities( m_pos, teamId, -1, TypeTriffidEgg, 1, vel, 0.0, 0.0 );
                TriffidEgg *egg = (TriffidEgg *) g_app->m_location->GetEntitySafe( id, TypeTriffidEgg );
                if( egg ) 
                {
                    while(egg->m_spawnType = syncrand() % (Triffid::NumSpawnTypes-1))
                    {
                        if( g_app->Multiplayer() )
                        {
                            if( egg->m_spawnType == Triffid::SpawnSoulDestroyer )
                            {
                                if( g_app->m_multiwinia->m_gameType == Multiwinia::GameTypeAssault ||
                                    SoulDestroyer::s_numSoulDestroyers >= 2 )
                                {
                                    continue;
                                }
                            }
                        }

                        break;
                    }
                }
                // The NumSpawnTypes-1 prevents Darwinians from coming out
            }
            break;
        }

        case Triffid::SpawnDarwinians:
        {
            int numDarwinians = 10 + syncrand() % 10;
            for( int i = 0; i < numDarwinians; ++i )
            {
                Vector3 vel = g_upVector + Vector3(SFRAND(1), 0.0, SFRAND(1) );
                vel.SetLength( 10.0 + syncfrand(20.0) );
                WorldObjectId id = g_app->m_location->SpawnEntities( m_pos, teamId, -1, TypeDarwinian, 1, vel, 0.0, 0.0 );
                Entity *entity = g_app->m_location->GetEntity( id );
                entity->m_front.y = 0.0;
                entity->m_front.Normalise();
                entity->m_onGround = false;
            }
            break;
        }

        case Triffid::SpawnAnts:
        {
            int numAnts = 20 + syncrand() % 10;
            for( int i = 0; i < numAnts; ++i )
            {
                Vector3 vel = g_upVector + Vector3(SFRAND(1), 0.0, SFRAND(1) );
                vel.SetLength( 10.0 + syncfrand(20.0) );
                WorldObjectId id = g_app->m_location->SpawnEntities( m_pos, teamId, -1, Entity::TypeArmyAnt, 1, vel, 0.0, 0.0 );
                Entity *entity = g_app->m_location->GetEntity( id );
                entity->m_front.y = 0.0;
                entity->m_front.Normalise();
                entity->m_onGround = false;
            }
            break;
        }

        case Triffid::SpawnSporeGenerator:
        {
            WorldObjectId id = g_app->m_location->SpawnEntities( m_pos, teamId, -1, TypeSporeGenerator, 1, g_zeroVector, 0.0, 200.0 );
            break;
        }

        //case Triffid::SpawnTripod:
        //{
        //    WorldObjectId id = g_app->m_location->SpawnEntities( m_pos, teamId, -1, TypeTripod, 1, g_zeroVector, 0.0, 200.0 );
        //    break;
        //}
    }
}


bool TriffidEgg::Advance( Unit *_unit )
{
    if( m_dead ) return true;

    m_pos += m_vel * SERVER_ADVANCE_PERIOD;

    // Fly through the air, bounce

	m_vel.y -= 9.8 * m_force;    

    Vector3 direction = m_vel;

    Vector3 right = g_upVector ^ direction;

	right.Normalise();	

	// For sync safety (maybe not needed)
	volatile double force = m_force;
	force *= m_force;
	force *= 30.0;
	force *= SERVER_ADVANCE_PERIOD;	

	Vector3 rotateVec = right * force;

	m_up.RotateAround( rotateVec );

    m_front = right ^ m_up;
    m_up.Normalise();
    m_front.Normalise();
	
    double landHeight = g_app->m_location->m_landscape.m_heightMap->GetValue( m_pos.x, m_pos.z );
    if( m_pos.y < landHeight + 3.0 )
    {            

		BounceOffLandscape();

        m_force *= TRIFFIDEGG_BOUNCEFRICTION;
        m_vel *= m_force;

        if( m_pos.y < landHeight + 3.0 ) m_pos.y = landHeight + 3.0;               
        if( m_force > 0.1 && !g_app->m_editing )
        {
            g_app->m_soundSystem->TriggerEntityEvent( this, "Bounce" );
        }
    }

    // Self right ourselves

    if( m_up.y < 0.3 && m_force < 0.4 )
    {
        m_up = m_up * 0.95 + g_upVector * 0.05;
        Vector3 right = m_up ^ m_front;
        m_front = right ^ m_up;
        m_up.Normalise();
        m_front.Normalise();
    }

    //
    // Break open if it is time

    if( GetNetworkTime() > m_timerSync )
    {
        Matrix34 transform( m_front, m_up, m_pos );    
        g_explosionManager.AddExplosion( m_shape, transform );
        Spawn();
        g_app->m_soundSystem->TriggerEntityEvent( this, "BurstOpen" );
        return true;
    }

    return Entity::Advance( _unit );
}


void TriffidEgg::Render( double _predictionTime )
{
    if( m_dead ) return;

    Vector3 predictedPos = m_pos + m_vel * _predictionTime;

    Vector3 direction = m_vel;
    Vector3 right = (g_upVector ^ direction).Normalise();
    Vector3 up = m_up;
    up.RotateAround( right * _predictionTime * m_force * m_force * 30.0 );   
    Vector3 front = right ^ up;
    up.Normalise();
    front.Normalise();

    // 
    // Make our size pulsate a little
    double timeNow = GetNetworkTime() + _predictionTime;
    double age = (m_timerSync - timeNow) / m_life;
    age = max( age, 0.0 );
    age = min( age, 1.0 );
    double size = m_size + iv_abs(iv_sin(timeNow*2.0)) * (1.0-age) * 0.4;

	if( !m_enabled )
	{
		front = m_front;
		up = g_upVector;
		size = m_size;


	}

    predictedPos.y -= size;
    Matrix34 transform( front, up, predictedPos );
    transform.f *= size;
    transform.u *= size;
    transform.r *= size;

    glEnable( GL_NORMALIZE );
    g_app->m_renderer->SetObjectLighting();
    m_shape->Render( _predictionTime, transform );
    g_app->m_renderer->UnsetObjectLighting();
    glDisable( GL_NORMALIZE );

    g_app->m_renderer->MarkUsedCells( m_shape, transform );

    //
    // Render our shadow

    BeginRenderShadow();
    RenderShadow( predictedPos, size*10.0 );
    EndRenderShadow();
}


bool TriffidEgg::RenderPixelEffect( double _predictionTime )
{    
    Render( _predictionTime );
     
    return true;
}


void TriffidEgg::ListSoundEvents( LList<char *> *_list )
{
    Entity::ListSoundEvents( _list );

    _list->PutData( "Bounce" );
    _list->PutData( "BurstOpen" );
}

void TriffidEgg::ResetTimer()
{
	m_timerSync = GetNetworkTime() + m_life;
}

void TriffidEgg::SetTimer( double _time )
{
    m_timerSync = GetNetworkTime() + _time;
}
