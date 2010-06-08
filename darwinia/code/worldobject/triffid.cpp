#include "lib/universal_include.h"

#include "lib/file_writer.h"
#include "lib/resource.h"
#include "lib/shape.h"
#include "lib/hi_res_time.h"
#include "lib/math_utils.h"
#include "lib/text_stream_readers.h"
#include "lib/debug_render.h"
#include "lib/text_renderer.h"
#include "lib/profiler.h"
#include "lib/language_table.h"

#include "sound/soundsystem.h"

#include "worldobject/triffid.h"

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


Triffid::Triffid()
:   Building(),
    m_launchPoint(NULL),
    m_stem(NULL),
    m_timerSync(0.0f),
    m_pitch( 0.6f ),
    m_force( 250.0f ),
    m_variance( M_PI / 8.0f ),
    m_reloadTime(60.0f),
    m_size(1.0f),
    m_damage(50.0f),
    m_useTrigger(0),
    m_triggerRadius(100.0f),
    m_triggered(false),
    m_triggerTimer(0.0f)
{
    m_type = TypeTriffid;
    
    SetShape( g_app->m_resource->GetShape("triffidhead.shp") );

    m_launchPoint = m_shape->m_rootFragment->LookupMarker( "MarkerLaunchPoint" );
    m_stem = m_shape->m_rootFragment->LookupMarker( "MarkerTriffidStem" );

    DarwiniaDebugAssert( m_launchPoint );
    DarwiniaDebugAssert( m_stem );

    m_triggerTimer = syncfrand(5.0f);

    memset( m_spawn, 0, NumSpawnTypes * sizeof(bool) );
}


Matrix34 Triffid::GetHead()
{
    Vector3 _pos = m_pos + g_upVector * m_size * 30.0f;
    Vector3 _front = m_front;
    Vector3 _up = g_upVector;

    float timer = g_gameTime + m_id.GetUniqueId() * 10.0f;

    if( m_damage > 0.0f )
    {
        _front.RotateAroundY( sinf(timer) * m_variance * 0.5f );
        Vector3 right = _front ^ _up;

        float pitchVariation = 1.0f + sinf(timer) * 0.1f;
        _front.RotateAround( right * m_pitch * 0.5f * pitchVariation );

        _pos += Vector3( sinf(timer/1.3f)*4, sinf(timer/1.5f), sinf(timer/1.2f)*3 );

        float justFiredFraction = ( m_timerSync - GetHighResTime() ) / m_reloadTime;
        justFiredFraction = min( justFiredFraction, 1.0f );
        justFiredFraction = max( justFiredFraction, 0.0f );
        justFiredFraction = pow( justFiredFraction, 5 );
    
        _pos -= _front * justFiredFraction * 10.0f;
    }
    else
    {
        // We are on fire, so thrash around a lot
        Vector3 right = _front ^ _up;
        _front = g_upVector;
        _up = right ^ _front;        

        _front.RotateAround( right * sinf(timer*5.0f) * m_variance );
        _up.RotateAroundY( sinf(timer*3.5f) * m_variance );
        
        _pos += Vector3( sinf(timer*3.3f)*8, sinf(timer*4.5f), sinf(timer*3.2f)*6 );

    }

    Matrix34 result( _front, _up, _pos );

    float size = m_size * ( 1.0f + sinf(timer) * 0.1f );

    result.f *= size;
    result.u *= size;
    result.r *= size;
    
    return result;
}


bool Triffid::DoesRayHit(Vector3 const &_rayStart, Vector3 const &_rayDir, 
                         float _rayLen, Vector3 *_pos, Vector3 *_norm)
{
    Matrix34 mat = GetHead();

	RayPackage ray(_rayStart, _rayDir, _rayLen);

    return m_shape->RayHit(&ray, mat, true);
}


    
void Triffid::Render( float _predictionTime )
{
    Matrix34 mat = GetHead();

    //RenderArrow( m_pos, mat.pos, 1.0f, RGBAColour(100,0,0,255) );

    Vector3 stemPos = m_stem->GetWorldMatrix(mat).pos;
    Vector3 midPoint = mat.pos + ( stemPos - mat.pos ).SetLength(10.0f);
    Vector3 midPoint2 = midPoint - Vector3(0,20,0);
    glColor4f( 1.0f, 1.0f, 0.5f, 1.0f );
    glLineWidth( 2.0f );
    glDisable( GL_TEXTURE_2D );
    glBegin( GL_LINES );
        glVertex3fv( mat.pos.GetData() );
        glVertex3fv( midPoint.GetData() );
        glVertex3fv( midPoint.GetData() );
        glVertex3fv( m_pos.GetData() );
    glEnd();

    //
    // If we are damaged, flicked in and out based on our health

    if( m_renderDamaged && !g_app->m_editing && m_damage > 0.0f )
    {
        float timeIndex = g_gameTime + m_id.GetUniqueId() * 10;
        float thefrand = frand();
        if      ( thefrand > 0.7f ) mat.f *= ( 1.0f - sinf(timeIndex) * 0.5f );
        else if ( thefrand > 0.4f ) mat.u *= ( 1.0f - sinf(timeIndex) * 0.2f );    
        else                        mat.r *= ( 1.0f - sinf(timeIndex) * 0.5f );
        glEnable( GL_BLEND );
        glBlendFunc( GL_ONE, GL_ONE );
    }

    glEnable( GL_NORMALIZE );

    m_shape->Render( _predictionTime, mat );

    if( m_triggered && GetHighResTime() > m_timerSync - m_reloadTime * 0.25f )
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


void Triffid::RenderAlphas( float _predictionTime )
{
    if( g_app->m_editing )
    {
        glColor4f( 1.0f, 0.0f, 0.0f, 1.0f );
        glLineWidth( 1.0f );

        Matrix34 headMat = GetHead();
        Matrix34 mat( m_front, g_upVector, headMat.pos );
        Matrix34 launchMat = m_launchPoint->GetWorldMatrix(mat);
    
        Vector3 point1 = launchMat.pos;
        
        Vector3 angle = launchMat.f;
        angle.HorizontalAndNormalise();
        angle.RotateAroundY( m_variance*0.5f );
        Vector3 right = angle ^ g_upVector;
        angle.RotateAround( right * m_pitch * 0.5f );        
        Vector3 point2 = point1 + angle * m_force * 3.0f;

        angle = launchMat.f;
        angle.HorizontalAndNormalise();
        angle.RotateAroundY( m_variance * -0.5f );
        right = angle ^ g_upVector;
        angle.RotateAround( right * m_pitch * 0.5f );
        Vector3 point3 = point1 + angle * m_force * 3.0f;

        glBegin( GL_LINE_LOOP );
            glVertex3fv( point1.GetData() );
            glVertex3fv( point2.GetData() );
            glVertex3fv( point3.GetData() );
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
        
            glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
            glLineWidth( 2.0f );
            glBegin( GL_LINES );
            while( true )
            {
                glVertex3fv( egg.m_pos.GetData() );
                egg.Advance( NULL );
                glVertex3fv( egg.m_pos.GetData() );
            
                if( egg.m_vel.Mag() < 20.0f ) break;
            }
            glEnd();

            if( m_useTrigger )
            {
                Vector3 triggerPos = m_pos + m_triggerLocation;
                int numSteps = 20;
                glBegin( GL_LINE_LOOP );
                glLineWidth( 1.0f );
                glColor4f( 1.0f, 0.0f, 0.0f, 1.0f );
                for( int i = 0; i < numSteps; ++i )
                {
                    float angle = 2.0f * M_PI * (float)i / (float) numSteps;
                    Vector3 thisPos = triggerPos + Vector3( sinf(angle)*m_triggerRadius, 0.0f,
                                                            cosf(angle)*m_triggerRadius );
                    thisPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( thisPos.x, thisPos.z );
                    thisPos.y += 10.0f;
                    glVertex3fv( thisPos.GetData() );
                }
                glEnd();

                g_editorFont.DrawText3DCentre( triggerPos+Vector3(0,50,0), 10.0f, "UseTrigger: %d", m_useTrigger );
            }
        }
#endif
        
    }
}


void Triffid::Damage( float _damage )
{
    Building::Damage( _damage );

    bool dead = (m_damage <= 0.0f);

    m_damage += _damage;

    if( m_damage <= 0.0f && !dead )
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

    Matrix34 mat = GetHead();
    Matrix34 launchMat = m_launchPoint->GetWorldMatrix(mat);    
    Vector3 velocity = launchMat.f;
    velocity.SetLength( m_force * m_size * ( 1.0f + syncsfrand(0.2f) ) );
    
    WorldObjectId wobjId = g_app->m_location->SpawnEntities( launchMat.pos, m_id.GetTeamId(), -1, Entity::TypeTriffidEgg, 1, velocity, 0.0f );
    TriffidEgg *triffidEgg = (TriffidEgg *) g_app->m_location->GetEntitySafe( wobjId, Entity::TypeTriffidEgg );
    if( triffidEgg ) 
    {
        triffidEgg->m_spawnType = spawnType;
        triffidEgg->m_size = 1.0f + syncsfrand(0.3f);
        triffidEgg->m_spawnPoint = m_pos + m_triggerLocation;
        triffidEgg->m_roamRange = m_triggerRadius;
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

    float timeAdd = syncrand() % (int) m_reloadTime;
    timeAdd += 10.0f;
    m_timerSync = GetHighResTime() + timeAdd;    
}


bool Triffid::Advance()
{    
    //
    // Burn if we have been damaged

    if( m_damage <= 0.0f )
    {
        Matrix34 headMat = GetHead();
        Vector3 fireSpawn = headMat.pos;
        fireSpawn += Vector3( sfrand(10.0f*m_size), 
                              sfrand(10.0f*m_size),
                              sfrand(10.0f*m_size) );
        float fireSize = 100.0f + sfrand(100.0f*m_size);
        g_app->m_particleSystem->CreateParticle( fireSpawn, g_zeroVector, Particle::TypeFire, fireSize );
        
        fireSpawn = m_pos + Vector3( sfrand(10.0f*m_size), 
                                     sfrand(10.0f*m_size),
                                     sfrand(10.0f*m_size) );
        g_app->m_particleSystem->CreateParticle( fireSpawn, g_zeroVector, Particle::TypeFire, fireSize );
        
        if( frand(100.0f) < 10.0f )
        {
            g_app->m_particleSystem->CreateParticle( fireSpawn, g_zeroVector, Particle::TypeExplosionDebris );
        }

        m_size -= 0.006f;
        if( m_size <= 0.3f )
        {
            g_explosionManager.AddExplosion( m_shape, headMat );
            return true;
        }
        m_timerSync -= 0.5f;
    }


    //
    // Look in our trigger area if required

    bool oldTriggered = m_triggered;

    if( m_useTrigger == 0 || m_damage <= 0.0f )
    {
        m_triggered = true;
    }

    if( m_useTrigger > 0 && GetHighResTime() > m_triggerTimer )
    {
        START_PROFILE( g_app->m_profiler, "CheckTrigger" );
        Vector3 triggerPos = m_pos + m_triggerLocation;
        bool enemiesFound = g_app->m_location->m_entityGrid->AreEnemiesPresent( triggerPos.x, triggerPos.z, m_triggerRadius, m_id.GetTeamId() );
        m_triggered = enemiesFound;
        m_triggerTimer = GetHighResTime() + 5.0f;
        END_PROFILE( g_app->m_profiler, "CheckTrigger" );
    }

    
    //
    // If we have just triggered, start our random timer

    if( !oldTriggered && m_triggered )
    {
        float timeAdd = syncrand() % (int) m_reloadTime;
        timeAdd += 10.0f;
        m_timerSync = GetHighResTime() + timeAdd;    
    }


    //
    // Launch an egg if it is time
    
    if( m_triggered && GetHighResTime() > m_timerSync )
    {
        Launch();
        m_timerSync = GetHighResTime() + m_reloadTime * ( 1.0f + syncsfrand(0.1f) );
    }


    //
    // Flicker if we are damaged

    float healthFraction = (float) m_damage / 50.0f;
    float timeIndex = g_gameTime + m_id.GetUniqueId() * 10;
    m_renderDamaged = ( frand(0.75f) * (1.0f - fabs(sinf(timeIndex))*1.2f) > healthFraction );

    return false;
}


void Triffid::Read( TextReader *_in, bool _dynamic )
{
    Building::Read( _in, _dynamic );

    m_size          = atof( _in->GetNextToken() );
    m_pitch         = atof( _in->GetNextToken() );
    m_force         = atof( _in->GetNextToken() );
    m_variance      = atof( _in->GetNextToken() );
    m_reloadTime    = atof( _in->GetNextToken() );
    m_useTrigger    = atoi( _in->GetNextToken() );
    
    m_triggerLocation.x = atof(_in->GetNextToken() );
    m_triggerLocation.z = atof(_in->GetNextToken() );
    m_triggerRadius     = atof(_in->GetNextToken() );

    for( int i = 0; i < NumSpawnTypes; ++i )
    {
        m_spawn[i] = ( atoi(_in->GetNextToken()) == 1 );
    }
}


void Triffid::Write( FileWriter *_out )
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
                                            "SpawnDarwinians"
                                         };

    return names[ _spawnType ];
}


char *Triffid::GetSpawnNameTranslated( int _spawnType )
{
    char *spawnName = GetSpawnName( _spawnType );

    char stringId[256];
    sprintf( stringId, "spawnname_%s", spawnName );

    if( ISLANGUAGEPHRASE( stringId ) )
    {
        return LANGUAGEPHRASE( stringId );
    }
    else
    {
        return spawnName;
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
    m_force(1.0f),
    m_spawnType(Triffid::SpawnVirii),
    m_size(1.0f)
{
    SetType( TypeTriffidEgg );
    
    m_up = g_upVector;
    m_shape = g_app->m_resource->GetShape( "triffidegg.shp" );

    m_life = 20.0f + syncfrand(10.0f);
    m_timerSync = GetHighResTime() + m_life;
}


void TriffidEgg::ChangeHealth( int _amount )
{
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
}


void TriffidEgg::Spawn()
{
    int teamId = m_id.GetTeamId();

    switch( m_spawnType )
    {
        case Triffid::SpawnVirii:    
        {
            int numVirii = 5 + syncrand() % 5;
            g_app->m_location->SpawnEntities( m_pos, teamId, -1, TypeVirii, numVirii, g_zeroVector, 0.0f, 100.0f );
            break;
        }
    
        case Triffid::SpawnCentipede:
        {
            int size = 5 + syncrand() % 5;
            Team *team = &g_app->m_location->m_teams[ m_id.GetTeamId() ];
            int unitId;
            Unit *unit = team->NewUnit( TypeCentipede, size, &unitId, m_pos );
            g_app->m_location->SpawnEntities( m_pos, teamId, unitId, TypeCentipede, size, g_zeroVector, 0.0f, 200.0f );
            break;
        }

        case Triffid::SpawnSpider:
        {
            WorldObjectId id = g_app->m_location->SpawnEntities( m_pos, teamId, -1, TypeSpider, 1, g_zeroVector, 0.0f, 150.0f );
            break;
        }

        case Triffid::SpawnSpirits:
        {
            int numSpirits = 5 + syncrand() % 5;
            for( int i = 0; i < numSpirits; ++i )
            {
                Vector3 vel(syncsfrand(), 0.0f, syncsfrand() );
                vel.SetLength( 20.0f + syncfrand(20.0f) );
                g_app->m_location->SpawnSpirit( m_pos, vel, teamId, WorldObjectId() );
            }
            break;
        }

        case Triffid::SpawnEggs:
        {
            int numEggs = 5 + syncrand() % 5;
            for( int i = 0; i < numEggs; ++i )
            {
                Vector3 vel = g_upVector + Vector3(syncsfrand(), 0.0f, syncsfrand() );
                vel.SetLength( 20.0f + syncfrand(20.0f) );
                g_app->m_location->SpawnEntities( m_pos, teamId, -1, TypeEgg, 1, vel, 0.0f, 0.0f );
            }
            break;
        }

        case Triffid::SpawnTriffidEggs:
        {
            int numEggs = 2 + syncrand() % 3;
            for( int i = 0; i < numEggs; ++i )
            {
                Vector3 vel(syncsfrand(), 0.5f + syncfrand(), syncsfrand() );
                vel.SetLength( 75.0f + syncfrand(50.0f) );
                WorldObjectId id = g_app->m_location->SpawnEntities( m_pos, teamId, -1, TypeTriffidEgg, 1, vel, 0.0f, 0.0f );
                TriffidEgg *egg = (TriffidEgg *) g_app->m_location->GetEntitySafe( id, TypeTriffidEgg );
                if( egg ) egg->m_spawnType = syncrand() % (Triffid::NumSpawnTypes-1);                                                                    
                // The NumSpawnTypes-1 prevents Darwinians from coming out
            }
            break;
        }

        case Triffid::SpawnDarwinians:
        {
            int numDarwinians = 10 + syncrand() % 10;
            for( int i = 0; i < numDarwinians; ++i )
            {
                Vector3 vel = g_upVector + Vector3(syncsfrand(), 0.0f, syncsfrand() );
                vel.SetLength( 10.0f + syncfrand(20.0f) );
                WorldObjectId id = g_app->m_location->SpawnEntities( m_pos, teamId, -1, TypeDarwinian, 1, vel, 0.0f, 0.0f );
                Entity *entity = g_app->m_location->GetEntity( id );
                entity->m_front.y = 0.0f;
                entity->m_front.Normalise();
                entity->m_onGround = false;
            }
            break;
        }
    }
}


bool TriffidEgg::Advance( Unit *_unit )
{
    if( m_dead ) return true;

    m_pos += m_vel * SERVER_ADVANCE_PERIOD;

    // Fly through the air, bounce
    m_vel.y -= 9.8f * m_force;    
    Vector3 direction = m_vel;
    Vector3 right = (g_upVector ^ direction).Normalise();
    m_up.RotateAround( right * SERVER_ADVANCE_PERIOD * m_force * m_force * 30.0f );
    m_front = right ^ m_up;
    m_up.Normalise();
    m_front.Normalise();

    float landHeight = g_app->m_location->m_landscape.m_heightMap->GetValue( m_pos.x, m_pos.z );
    if( m_pos.y < landHeight + 3.0f )
    {            
        BounceOffLandscape();
        m_force *= TRIFFIDEGG_BOUNCEFRICTION;
        m_vel *= m_force;
        if( m_pos.y < landHeight + 3.0f ) m_pos.y = landHeight + 3.0f;               
        if( m_force > 0.1f )
        {
            g_app->m_soundSystem->TriggerEntityEvent( this, "Bounce" );
        }
    }

    // Self right ourselves

    if( m_up.y < 0.3f && m_force < 0.4f )
    {
        m_up = m_up * 0.95f + g_upVector * 0.05f;
        Vector3 right = m_up ^ m_front;
        m_front = right ^ m_up;
        m_up.Normalise();
        m_front.Normalise();
    }

    //
    // Break open if it is time

    if( GetHighResTime() > m_timerSync )
    {
        Matrix34 transform( m_front, m_up, m_pos );    
        g_explosionManager.AddExplosion( m_shape, transform );
        Spawn();
        g_app->m_soundSystem->TriggerEntityEvent( this, "BurstOpen" );
        return true;
    }

    return Entity::Advance( _unit );
}


void TriffidEgg::Render( float _predictionTime )
{
    if( m_dead ) return;

    Vector3 predictedPos = m_pos + m_vel * _predictionTime;

    Vector3 direction = m_vel;
    Vector3 right = (g_upVector ^ direction).Normalise();
    Vector3 up = m_up;
    up.RotateAround( right * _predictionTime * m_force * m_force * 30.0f );   
    Vector3 front = right ^ up;
    up.Normalise();
    front.Normalise();

    // 
    // Make our size pulsate a little
    float age = (m_timerSync - GetHighResTime()) / m_life;
    age = max( age, 0.0f );
    age = min( age, 1.0f );
    float size = m_size + fabs(sinf(g_gameTime*2.0f)) * (1.0f-age) * 0.4f;

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
    RenderShadow( predictedPos, size*10.0f );
    EndRenderShadow();
}


bool TriffidEgg::RenderPixelEffect( float _predictionTime )
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
