#include "lib/universal_include.h"
#include "lib/resource.h"
#include "lib/file_writer.h"
#include "lib/text_stream_readers.h"
#include "lib/text_renderer.h"
#include "lib/shape.h"
#include "lib/debug_render.h"
#include "lib/preferences.h"
#include "lib/language_table.h"

#ifdef CHEATMENU_ENABLED
#include "lib/input/input.h"
#include "lib/input/input_types.h"
#endif

#include "worldobject/rocket.h"
#include "worldobject/darwinian.h"

#include "app.h"
#include "location.h"
#include "camera.h"
#include "team.h"
#include "main.h"
#include "particle_system.h"
#include "explosion.h"
#include "global_world.h"
#include "script.h"
#include "entity_grid.h"
#include "renderer.h"
#include "taskmanager_interface.h"

#include "sound/soundsystem.h"


Shape *FuelBuilding::s_fuelPipe = NULL;


FuelBuilding::FuelBuilding()
:   Building(),
    m_fuelLink(-1),
    m_currentLevel(0.0f),
    m_fuelMarker(NULL)
{
    if( !s_fuelPipe )
    {
        s_fuelPipe = g_app->m_resource->GetShape( "fuelpipe.shp" );
        DarwiniaDebugAssert( s_fuelPipe );
    }
}


void FuelBuilding::Initialise( Building *_template )
{
    Building::Initialise( _template );

    m_fuelLink = ((FuelBuilding *) _template)->m_fuelLink;
}


Vector3 FuelBuilding::GetFuelPosition()
{
    if( !m_fuelMarker )
    {
        m_fuelMarker = m_shape->m_rootFragment->LookupMarker( "MarkerFuel" );
        DarwiniaDebugAssert( m_fuelMarker );
    }   

    Matrix34 mat( m_front, m_up, m_pos );
    return m_fuelMarker->GetWorldMatrix(mat).pos;
}


void FuelBuilding::ProvideFuel( float _level )
{
    float factor2 = 0.2f * SERVER_ADVANCE_PERIOD;
    float factor1 = 1.0f - factor2;

    m_currentLevel = m_currentLevel * factor1 +
                     _level * factor2;

    m_currentLevel = min( m_currentLevel, 1.0f );
    m_currentLevel = max( m_currentLevel, 0.0f );
}


FuelBuilding *FuelBuilding::GetLinkedBuilding()
{
    Building *building = g_app->m_location->GetBuilding( m_fuelLink );
    if( building )
    {
        if( building->m_type == TypeFuelGenerator ||
            building->m_type == TypeFuelPipe ||
            building->m_type == TypeEscapeRocket ||
            building->m_type == TypeFuelStation )
        {
            FuelBuilding *fuelBuilding = (FuelBuilding *) building;
            return fuelBuilding;
        }
    }

    return NULL;
}


bool FuelBuilding::Advance()
{
    FuelBuilding *fuelBuilding = GetLinkedBuilding();
    if( fuelBuilding )
    {
        fuelBuilding->ProvideFuel( m_currentLevel );
    }

    return Building::Advance();
}


bool FuelBuilding::IsInView()
{
    FuelBuilding *fuelBuilding = GetLinkedBuilding();

    if( fuelBuilding )
    {
        Vector3 ourPipePos = GetFuelPosition();
        Vector3 theirPipePos = fuelBuilding->GetFuelPosition();
        
        Vector3 midPoint = ( theirPipePos + ourPipePos ) / 2.0f;
        float radius = ( theirPipePos - ourPipePos ).Mag() / 2.0f;;
        radius += m_radius;
        
        return( g_app->m_camera->SphereInViewFrustum( midPoint, radius ) );
    }
    else
    {
        return Building::IsInView();
    }
}


void FuelBuilding::Render( float _predictionTime )
{
    Building::Render( _predictionTime );
    
    FuelBuilding *fuelBuilding = GetLinkedBuilding();
    if( fuelBuilding )
    {
        Vector3 ourPipePos = GetFuelPosition();
        Vector3 theirPipePos = fuelBuilding->GetFuelPosition();

        Vector3 pipeVector = (theirPipePos - ourPipePos).Normalise();
        Vector3 right = pipeVector ^ g_upVector;
        Vector3 up = pipeVector ^ right;

        ourPipePos += pipeVector * 10;
        
        Matrix34 pipeMat( up, pipeVector, ourPipePos );
        DarwiniaDebugAssert( s_fuelPipe );
        s_fuelPipe->Render( _predictionTime, pipeMat );
    }
}


void FuelBuilding::RenderAlphas( float _predictionTime )
{
    Building::RenderAlphas( _predictionTime );

    if( m_currentLevel > 0.0f )
    {
        FuelBuilding *fuelBuilding = GetLinkedBuilding();
        if( fuelBuilding )
        {
            Vector3 startPos = GetFuelPosition();
            Vector3 endPos = fuelBuilding->GetFuelPosition();

            Vector3 midPos = ( startPos + endPos ) / 2.0f;
            Vector3 rightAngle = ( g_app->m_camera->GetPos() - midPos ) ^ ( startPos - endPos );
            rightAngle.SetLength( 25.0f );

            glBindTexture       ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/fuel.bmp" ) );
            glEnable            ( GL_TEXTURE_2D );
            glTexParameteri     ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
            glTexParameteri     ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );    
            glEnable            ( GL_BLEND );
            glBlendFunc         ( GL_SRC_ALPHA, GL_ONE );
            glDepthMask         ( false );
        
            float tx = g_gameTime * -0.5f;
            float tw = 1.0f;

            glColor4f( 1.0f, 0.4f, 0.1f, 0.4f * m_currentLevel );

	        float nearPlaneStart = g_app->m_renderer->GetNearPlane();
	        g_app->m_camera->SetupProjectionMatrix(nearPlaneStart * 1.2f,
							 			           g_app->m_renderer->GetFarPlane());

            int buildingDetail = g_prefsManager->GetInt( "RenderBuildingDetail" );
            float maxLoops = 4 - buildingDetail;
            maxLoops = max( maxLoops, 1 );
            maxLoops = min( maxLoops, 3 );
            
            for( int i = 0; i < maxLoops; ++i )
            {
                glBegin( GL_QUADS );
                    glTexCoord2f( tx, 0 );      glVertex3fv( (startPos - rightAngle).GetData() );
                    glTexCoord2f( tx, 1 );      glVertex3fv( (startPos + rightAngle).GetData() );
                    glTexCoord2f( tx+tw, 1 );   glVertex3fv( (endPos + rightAngle).GetData() );
                    glTexCoord2f( tx+tw, 0 );   glVertex3fv( (endPos - rightAngle).GetData() );
                glEnd();
                rightAngle *= 0.7f;
            }

	        g_app->m_camera->SetupProjectionMatrix(nearPlaneStart,
								 		           g_app->m_renderer->GetFarPlane());

            glEnable( GL_DEPTH_TEST );
            glDisable( GL_TEXTURE_2D );
        }
    }
    
//    glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
//    g_editorFont.DrawText3DCentre( m_pos+Vector3(0,70,0), 5, "Fuel Pressure : %2.2f", m_currentLevel );
}


void FuelBuilding::Read( TextReader *_in, bool _dynamic )
{
    Building::Read( _in, _dynamic );

    m_fuelLink = atoi( _in->GetNextToken() );
}


void FuelBuilding::Write( FileWriter *_out )
{
    Building::Write( _out );
    
    _out->printf( "%6d ", m_fuelLink );
}


int FuelBuilding::GetBuildingLink()
{
    return m_fuelLink;
}


void FuelBuilding::SetBuildingLink( int _buildingId )
{
    m_fuelLink = _buildingId;   
}

void FuelBuilding::Destroy( float _intensity )
{
	Building::Destroy( _intensity );
	FuelBuilding *fuelBuilding = GetLinkedBuilding();

    if( fuelBuilding )
    {
        Vector3 ourPipePos = GetFuelPosition();
        Vector3 theirPipePos = fuelBuilding->GetFuelPosition();

        Vector3 pipeVector = (theirPipePos - ourPipePos).Normalise();
        Vector3 right = pipeVector ^ g_upVector;
        Vector3 up = pipeVector ^ right;

        ourPipePos += pipeVector * 10;
        
        Matrix34 pipeMat( up, pipeVector, ourPipePos );
		g_explosionManager.AddExplosion( s_fuelPipe, pipeMat );
    }
}
// ============================================================================


FuelGenerator::FuelGenerator()
:   FuelBuilding(),
    m_surges(0.0f),
    m_pump(NULL),
    m_pumpTip(NULL),
    m_pumpMovement(0.0f),
    m_previousPumpPos(0.0f)
{
    m_type = TypeFuelGenerator;

    SetShape( g_app->m_resource->GetShape( "fuelgenerator.shp" ) );
    
    m_pump = g_app->m_resource->GetShape( "fuelgeneratorpump.shp" );
    m_pumpTip = m_pump->m_rootFragment->LookupMarker( "MarkerTip" );
}


void FuelGenerator::ProvideSurge()
{
    m_surges++;
}


bool FuelGenerator::Advance()
{   
    //
    // Advance surges

    m_surges -= SERVER_ADVANCE_PERIOD * 1.0f;
    m_surges = min( m_surges, 10 );
    m_surges = max( m_surges, 0 );
 
    if( m_surges > 8 )
    {
        GlobalBuilding *gb = g_app->m_globalWorld->GetBuilding( m_id.GetUniqueId(), g_app->m_locationId );
        if( gb ) gb->m_online = true;
    }


    //
    // Pump fuel

    float fuelVal = m_surges / 10.0f;
    ProvideFuel( fuelVal );
    

    //
    // Spawn particles

    float previousPumpPos = m_previousPumpPos;
    Vector3 pumpPos = GetPumpPos();
    m_previousPumpPos = (pumpPos.y - m_pos.y) / -80.0f;

    if( fuelVal > 0.0f && pumpPos.y > m_pos.y - 20.0f )
    {
        pumpPos.x += sfrand(10.0f);        
        pumpPos.z += sfrand(10.0f);
     
        for( int i = 0; i < int(m_surges); ++i )
        {
            Vector3 pumpVel = g_upVector * 20;
            pumpVel += g_upVector * frand(10);

            Matrix34 mat( m_front, g_upVector, pumpPos );
            Vector3 particlePos = m_pumpTip->GetWorldMatrix(mat).pos;
            float size = 150.0f + frand(150.0f);

            g_app->m_particleSystem->CreateParticle( particlePos, pumpVel, Particle::TypeDarwinianFire, size );            
        }
    }


    //
    // Play sounds

    if( previousPumpPos >= 0.1f && m_previousPumpPos < 0.1f )
    {
        g_app->m_soundSystem->TriggerBuildingEvent( this, "PumpUp" );
    }
    else if( previousPumpPos <= 0.9f && m_previousPumpPos > 0.9f )
    {
        g_app->m_soundSystem->TriggerBuildingEvent( this, "PumpDown" );
    }
    
    return FuelBuilding::Advance();
}


void FuelGenerator::ListSoundEvents( LList<char *> *_list )
{
    FuelBuilding::ListSoundEvents( _list );

    _list->PutData( "PumpUp" );
    _list->PutData( "PumpDown" );
}


Vector3 FuelGenerator::GetPumpPos()
{
    Vector3 pumpPos = m_pos;
    float pumpHeight = 80;

    pumpPos.y -= pumpHeight;
    pumpPos.y += fabs( cosf( m_pumpMovement ) ) * pumpHeight;

    return pumpPos;
}

void FuelGenerator::Render( float _predictionTime )
{
    FuelBuilding::Render( _predictionTime );

    //
    // Render the pump
    
    float fuelVal = m_surges / 10.0f;
    m_pumpMovement += g_advanceTime * fuelVal * 2;

    Vector3 pumpPos = GetPumpPos();
    Matrix34 mat( m_front, g_upVector, pumpPos );

    m_pump->Render( _predictionTime, mat );
}


void FuelGenerator::RenderAlphas( float _predictionTime )
{
    FuelBuilding::RenderAlphas( _predictionTime );
    
//    glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
//    g_editorFont.DrawText3DCentre( m_pos+Vector3(0,90,0), 10, "Surges : %2.2f", m_surges );
}


char *FuelGenerator::GetObjectiveCounter()
{
    static char buffer[256];
    sprintf( buffer, "%s %d%%", LANGUAGEPHRASE("objective_fuelpressure"), int( m_currentLevel * 100 ) );
    return buffer;
}



// ============================================================================


FuelPipe::FuelPipe()
:   FuelBuilding()
{
    m_type = TypeFuelPipe;

    SetShape( g_app->m_resource->GetShape( "fuelpipebase.shp" ) );
}


bool FuelPipe::Advance()
{
    //
    // Ensure our sound ambiences are playing

    int numInstances = g_app->m_soundSystem->NumInstances( m_id, "FuelPipe PumpFuel" );

    if( m_currentLevel > 0.2f && numInstances == 0 )
    {
        g_app->m_soundSystem->TriggerBuildingEvent( this, "PumpFuel" );
    }
    else if( m_currentLevel <= 0.2f && numInstances > 0 )
    {
        g_app->m_soundSystem->StopAllSounds( m_id, "FuelPipe PumpFuel" );
    }

    
    return FuelBuilding::Advance();
}


void FuelPipe::ListSoundEvents( LList<char *> *_list )
{
    FuelBuilding::ListSoundEvents( _list );

    _list->PutData( "PumpFuel" );
}


// ============================================================================


FuelStation::FuelStation()
:   FuelBuilding(),
    m_entrance(NULL)
{
    m_type = TypeFuelStation;

    SetShape( g_app->m_resource->GetShape( "fuelstation.shp" ) );

    m_entrance = m_shape->m_rootFragment->LookupMarker( "MarkerEntrance" );
}


bool FuelStation::IsLoading()
{
    Building *building = g_app->m_location->GetBuilding( m_fuelLink );
    if( building && building->m_type == TypeEscapeRocket )
    {
        EscapeRocket *rocket = (EscapeRocket *) building;
        if( rocket->m_state == EscapeRocket::StateLoading )
        {
            return true;
        }
    }

    return false;
}


bool FuelStation::Advance()
{
    Building *building = g_app->m_location->GetBuilding( m_fuelLink );
    if( building && building->m_type == TypeEscapeRocket )
    {
        EscapeRocket *rocket = (EscapeRocket *) building;
        if( rocket->m_state == EscapeRocket::StateLoading &&
            rocket->SafeToLaunch() )
        {
            //
            // Find a random Darwinian and make him board

            Team *team = &g_app->m_location->m_teams[0];
            int numOthers = team->m_others.Size();
            if( numOthers > 0 )
            {
                int randomIndex = syncrand() % numOthers;
                if( team->m_others.ValidIndex(randomIndex) )
                {
                    Entity *entity = team->m_others[randomIndex];               
                    if( entity && entity->m_type == Entity::TypeDarwinian )
                    {
                        Darwinian *darwinian = (Darwinian *) entity;
                        float distance = (entity->m_pos - m_pos).Mag();
                        if( distance < 300.0f && 
                            (darwinian->m_state == Darwinian::StateIdle ||
                             darwinian->m_state == Darwinian::StateWorshipSpirit) )
                        {
                            darwinian->BoardRocket( m_id.GetUniqueId() );
                        }
                    }
                }          
            }
        }        
    }

    return FuelBuilding::Advance();
}


Vector3 FuelStation::GetEntrance()
{
    Matrix34 mat( m_front, g_upVector, m_pos );
    return m_entrance->GetWorldMatrix(mat).pos;
}


bool FuelStation::BoardRocket( WorldObjectId _id )
{
    Building *building = g_app->m_location->GetBuilding( m_fuelLink );
    if( building && building->m_type == TypeEscapeRocket )
    {
        EscapeRocket *rocket = (EscapeRocket *) building;
        bool result = rocket->BoardRocket( _id );

        if( result )
        {
            Entity *entity = g_app->m_location->GetEntity( _id );
            Vector3 entityPos = entity ? entity->m_pos : g_zeroVector;
            entityPos.y += 2;

            int numFlashes = 4 + darwiniaRandom() % 4;
            for( int i = 0; i < numFlashes; ++i )
            {
                Vector3 vel( sfrand(15.0f), frand(35.0f), sfrand(15.0f) );
                g_app->m_particleSystem->CreateParticle( entityPos, vel, Particle::TypeControlFlash );
            }

            g_app->m_soundSystem->TriggerBuildingEvent( this, "LoadPassenger" );
        }

        return result;
    }

    return false;
}


void FuelStation::ListSoundEvents( LList<char *> *_list )
{
    FuelBuilding::ListSoundEvents( _list );

    _list->PutData( "LoadPassenger" );
}


void FuelStation::Render( float _predictionTime )
{
    Building::Render( _predictionTime );        
}


void FuelStation::RenderAlphas( float _predictionTime )
{
    // Prevent FuelBuilding::RenderAlphas from being called
       


    //
    // Render countdown

    Building *building = g_app->m_location->GetBuilding( m_fuelLink );
    if( building && building->m_type == TypeEscapeRocket )
    {
        EscapeRocket *rocket = (EscapeRocket *) building;
        if( (rocket->m_state == EscapeRocket::StateCountdown && rocket->m_countdown <= 10.0f)
            || rocket->m_state == EscapeRocket::StateFlight )
        {
            float screenSize = 60.0f;
            Vector3 screenFront = m_front;
            //screenFront.RotateAroundY( 0.33f * M_PI );
            Vector3 screenRight = screenFront ^ g_upVector;
            Vector3 screenPos = m_pos + Vector3(0,150,0);
            screenPos -= screenRight * screenSize * 0.5f;
            screenPos += screenFront * 30;
            Vector3 screenUp = g_upVector;

            //
            // Render lines for over effect

            glBindTexture( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/interface_grey.bmp" ) );
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
            glEnable( GL_TEXTURE_2D );
            glEnable( GL_BLEND );
            glDepthMask( false );
            glShadeModel( GL_SMOOTH );
            glDisable( GL_CULL_FACE );

            float texX = 0.0f;
            float texW = 3.0f;
            float texY = g_gameTime*0.01f;
            float texH = 0.3f;

            glColor4f( 0.0f, 0.0f, 0.0f, 0.5f );
            glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
            
            glBegin( GL_QUADS );
                glVertex3fv( screenPos.GetData() );
                glVertex3fv( (screenPos + screenRight * screenSize).GetData() );
                glVertex3fv( (screenPos + screenRight * screenSize + screenUp * screenSize).GetData() );
                glVertex3fv( (screenPos + screenUp * screenSize).GetData() );
            glEnd();

            glColor4f( 1.0f, 0.4f, 0.2f, 1.0f );

            glBlendFunc( GL_SRC_ALPHA, GL_ONE );
            
            for( int i = 0; i < 2; ++i )
            {
                glBegin( GL_QUADS );
                    glTexCoord2f(texX,texY);            
                    glVertex3fv( screenPos.GetData() );

                    glTexCoord2f(texX+texW,texY);       
                    glVertex3fv( (screenPos + screenRight * screenSize).GetData() );
                   
                    glTexCoord2f(texX+texW,texY+texH);  
                    glVertex3fv( (screenPos + screenRight * screenSize + screenUp * screenSize).GetData() );

                    glTexCoord2f(texX,texY+texH);       
                    glVertex3fv( (screenPos + screenUp * screenSize).GetData() );
                glEnd();

                texY *= 1.5f;
                texH = 0.1f;
            }


            glDepthMask( false );
            
            //
            // Render countdown

            glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
            Vector3 textPos = screenPos 
                                + screenRight * screenSize * 0.5f
                                + screenUp * screenSize * 0.5f;

            if( rocket->m_state == EscapeRocket::StateCountdown )
            {
                int countdown = (int) rocket->m_countdown + 1;            
                g_gameFont.DrawText3D( textPos, screenFront, g_upVector, 50, "%d", countdown );
            }
            else
            {
                if( fmod( g_gameTime, 2 ) < 1 )
                {
                    g_gameFont.DrawText3D( textPos, screenFront, g_upVector, 50, "0" );
                }
            }

            
  
            //
            // Render projection effect

            glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/laser.bmp" ) );

            Vector3 ourPos = m_pos + Vector3(0,90,0);
            Vector3 theirPos = m_pos + Vector3(0,200,0);
            theirPos += screenFront * 30.0f;

            Vector3 camToTheirPos = g_app->m_camera->GetPos() - theirPos;
            Vector3 lineTheirPos = camToTheirPos ^ ( ourPos - theirPos );
            lineTheirPos.SetLength( m_radius * 0.5f );
        
            for( int i = 0; i < 3; ++i )
            {
                Vector3 pos = theirPos;
                pos.x += sinf( g_gameTime + i ) * 15;
                pos.y += sinf( g_gameTime + i ) * 15;
                pos.z += cosf( g_gameTime + i ) * 15;

                float blue = 0.5f + fabs( sinf(g_gameTime * i) ) * 0.5f;

                glBegin( GL_QUADS );
                    glColor4f( 1.0f, 0.4f, 0.2f, 0.4f );
                    glTexCoord2f(1,0);      glVertex3fv( (ourPos - lineTheirPos).GetData() );
                    glTexCoord2f(1,1);      glVertex3fv( (ourPos + lineTheirPos).GetData() );
                    glColor4f( 1.0f, 0.4f, 0.2f, 0.2f );
                    glTexCoord2f(0,1);      glVertex3fv( (pos + lineTheirPos).GetData() );
                    glTexCoord2f(0,0);      glVertex3fv( (pos - lineTheirPos).GetData() );
                glEnd();
            }
        
        
            glDepthMask     ( true );
            glEnable        ( GL_DEPTH_TEST );
            glDisable       ( GL_TEXTURE_2D );
            glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
            glDisable       ( GL_BLEND );        
            glShadeModel    ( GL_FLAT );
        }
    }

}


bool FuelStation::PerformDepthSort( Vector3 &_centrePos )
{
    _centrePos = m_centrePos;
    return true;
}



// ============================================================================


EscapeRocket::EscapeRocket()
:   FuelBuilding(),
    m_fuel(0.0f),
    m_pipeCount(0),
    m_passengers(0),
    m_countdown(-1.0f),
    m_booster(NULL),
    m_shadowTimer(0.0f),
    m_state(StateRefueling),
    m_damage(0.0f),
    m_spawnBuildingId(-1),
    m_spawnCompleted(false),
    m_cameraShake(0.0f)
{
    m_type = TypeEscapeRocket;

    SetShape( g_app->m_resource->GetShape( "rocket.shp" ) );

    m_booster = m_shape->m_rootFragment->LookupMarker( "MarkerBooster" );
    DarwiniaReleaseAssert( m_booster, "MarkerBooster not found in rocket.shp" );

    for( int i = 0; i < 3; ++i )
    {
        char name[256];
        sprintf( name, "MarkerWindow0%d", i+1 );
        m_window[i] = m_shape->m_rootFragment->LookupMarker( name );
        DarwiniaReleaseAssert( m_window[i], "%s not found", name );
    }
}


void EscapeRocket::ListSoundEvents( LList<char *> *_list )
{
    FuelBuilding::ListSoundEvents( _list );
    
    _list->PutData( "Refueling" );
    _list->PutData( "Happy" );
    _list->PutData( "Unhappy" );
    _list->PutData( "Flight" );
    _list->PutData( "Malfunction" );
    _list->PutData( "Explode" );
    _list->PutData( "EngineBurn" );
}


void EscapeRocket::SetupSounds()
{
    char *requiredSoundName = NULL;

    //
    // What ambience should be playing?

    switch( m_state )
    {
        case StateRefueling:   
            if( m_currentLevel > 0.2f )     requiredSoundName = "Refueling";                       
            else                            requiredSoundName = "Unhappy";           
                                            break;
        
        case StateLoading:
        case StateIgnition:
        case StateReady:                
        case StateCountdown:   
            if( m_damage < 10 )             requiredSoundName = "Happy";               
            else                            requiredSoundName = "Malfunction";
                                            break;
    
        case StateExploding:
            if( m_fuel > 0.0f )             requiredSoundName = "Malfunction";           
            else                            requiredSoundName = "Unhappy";            
                                            break;
            
        case StateFlight:                   requiredSoundName = "Flight";
                                            break;            
    }

    char fullName[256];
    if( requiredSoundName ) sprintf( fullName, "EscapeRocket %s", requiredSoundName );


    //
    // If we're not set up right, kill all sounds first


    int numInstances = requiredSoundName ? g_app->m_soundSystem->NumInstances( m_id, fullName ) : 0;

    if( !requiredSoundName || numInstances == 0 )
    {
        g_app->m_soundSystem->StopAllSounds( m_id, "EscapeRocket Refueling" );
        g_app->m_soundSystem->StopAllSounds( m_id, "EscapeRocket Happy" );
        g_app->m_soundSystem->StopAllSounds( m_id, "EscapeRocket Unhappy" );
        g_app->m_soundSystem->StopAllSounds( m_id, "EscapeRocket Malfunction" );
        g_app->m_soundSystem->StopAllSounds( m_id, "EscapeRocket Flight" );
    }


    //
    // Spawn the correct sound

    if( requiredSoundName && numInstances == 0 )
    {
        g_app->m_soundSystem->TriggerBuildingEvent( this, requiredSoundName );
    }


    //
    // If our engines are on then trigger the event

    int numEngineInstances = g_app->m_soundSystem->NumInstances( m_id, "EscapeRocket EngineBurn" );

    if( m_state == StateReady ||
        m_state == StateCountdown || 
        m_state == StateFlight )
    {
        if( numEngineInstances == 0 )
        {
            g_app->m_soundSystem->TriggerBuildingEvent( this, "EngineBurn" );
        }
    }
    else
    {
        if( numEngineInstances > 0 )
        {
            g_app->m_soundSystem->StopAllSounds( m_id, "EscapeRocket EngineBurn" );
        }
    }
}


void EscapeRocket::Initialise( Building *_template )
{
    FuelBuilding::Initialise( _template );

    m_fuel              = ((EscapeRocket *) _template)->m_fuel;
    m_passengers        = ((EscapeRocket *) _template)->m_passengers;
    m_spawnBuildingId   = ((EscapeRocket *) _template)->m_spawnBuildingId;
    m_spawnCompleted    = ((EscapeRocket *) _template)->m_spawnCompleted;
}


char *EscapeRocket::GetObjectiveCounter()
{
    static char buffer[256];
    sprintf( buffer, "%s %d%%, %s %d%%", LANGUAGEPHRASE("objective_fuel"),
                                         (int) m_fuel, 
                                         LANGUAGEPHRASE("objective_passengers"),
                                         (int) m_passengers );
    return buffer;
}


bool EscapeRocket::BoardRocket( WorldObjectId _id )
{
    if( m_state == StateLoading )
    {
        ++m_passengers;
        return true;
    }

    return false;
}


void EscapeRocket::ProvideFuel( float _level )
{
#ifdef CHEATMENU_ENABLED
    if( g_inputManager->controlEvent( ControlScrollSpeedup ) )
    {
        _level *= 100;
    }
#endif

    FuelBuilding::ProvideFuel( _level );

    if( _level > 0.1f )
    {
        ++m_pipeCount;
    }
}


void EscapeRocket::Refuel()
{
    switch( m_pipeCount )
    {
        case 0:                                                     // No incoming fuel
        case 1:                                                     // 1 pipe providing fuel
        {
            float targetFuel = 50.0f;
            if( m_fuel < targetFuel )
            {
                float factor1 = m_currentLevel * SERVER_ADVANCE_PERIOD * 0.005f;
                float factor2 = 1.0f - factor1;
                m_fuel = m_fuel * factor2 + targetFuel * factor1;            
            }
            break;
        }

        case 2:                                                     // 2 pipes providing fuel
        {
            float targetFuel = 100.0f;
            float factor1 = m_currentLevel * SERVER_ADVANCE_PERIOD * 0.01f;
            float factor2 = 1.0f - factor1;
            m_fuel = m_fuel * factor2 + targetFuel * factor1;
            m_fuel = min( m_fuel, 100.0f );
            break;
        }

        case 3:                                                     // 3 pipes providing fuel
        {
            float targetFuel = 100.0f;
            float factor1 = m_currentLevel * SERVER_ADVANCE_PERIOD * 0.1;
            float factor2 = 1.0f - factor1;
            m_fuel = m_fuel * factor2 + targetFuel * factor1;
            m_fuel = min( m_fuel, 100.0f );
            break;
        }
    };

    m_pipeCount = 0;
}


void EscapeRocket::AdvanceRefueling()
{
    Refuel();

    m_damage = 0.0f;

    if( m_fuel >= 95.0f )
    {
        m_state = StateLoading;
    }
}


void EscapeRocket::AdvanceLoading()
{
    Refuel();

    if( m_fuel > 95.0f && m_passengers >= 100 )
    {
        m_state = StateIgnition;
        m_countdown = 18.0f;
    }
}


void EscapeRocket::AdvanceIgnition()
{
    m_countdown -= SERVER_ADVANCE_PERIOD;
   
    SetupSpectacle();

    if( m_countdown <= 0.0f )
    {        
        m_state = StateReady;
        m_countdown = 120.0f;

        m_cameraShake = 5.0f;

        if( m_spawnCompleted )
        {
            m_countdown = 10.0f;
            g_app->m_taskManagerInterface->SetVisible( false );
            if( g_app->m_script->IsRunningScript() )
            {
                g_app->m_script->Skip();
            }
#ifdef DEMOBUILD
            g_app->m_script->RunScript( "launchpad_victory_demo.txt" );
#else
			g_app->m_script->RunScript( "launchpad_victory.txt");
#endif
        }
    }    
}


void EscapeRocket::AdvanceReady()
{
    //
    // Spawn attacking Darwinians

    if( !m_spawnCompleted )
    {
        if( m_countdown > 100.0f && m_countdown < 110.0f )
        {
            Building *spawnBuilding = g_app->m_location->GetBuilding( m_spawnBuildingId );
            if( spawnBuilding )
            {
                Vector3 spawnPos = spawnBuilding->m_pos + spawnBuilding->m_front * 40.0f;
                g_app->m_location->SpawnEntities( spawnPos, 1, -1, Entity::TypeDarwinian, 1, g_zeroVector, 40.0f );
            }
        }    
    }

    SetupAttackers();
    SetupSpectacle();
    
    m_countdown -= SERVER_ADVANCE_PERIOD;

    if( m_countdown <= 0.0f )
    {
        m_state = StateCountdown;
        m_countdown = 10.0f;
    }
}


void EscapeRocket::AdvanceCountdown()
{
    m_countdown -= SERVER_ADVANCE_PERIOD * 0.5f;
    m_countdown = max( m_countdown, 0.0f );

    SetupAttackers();
    SetupSpectacle();

    if( m_countdown == 0.0f )
    {
        m_state = StateFlight;

        GlobalBuilding *gb = g_app->m_globalWorld->GetBuilding( m_id.GetUniqueId(), g_app->m_locationId );
        //if( gb ) gb->m_online = true;
    }
}


void EscapeRocket::AdvanceFlight()
{
    float landHeight = g_app->m_location->m_landscape.m_heightMap->GetValue( m_pos.x, m_pos.z );
    float thrust = sqrtf(m_pos.y - landHeight) * 2;
    thrust = max( thrust, 0.1f );

    m_vel.Set( 0, thrust, 0 );

    m_pos += m_vel * SERVER_ADVANCE_PERIOD;
    m_centrePos += m_vel * SERVER_ADVANCE_PERIOD;

    SetupSpectacle();
}


void EscapeRocket::AdvanceExploding()
{
    m_damage -= SERVER_ADVANCE_PERIOD * 1;

    //
    // Burn fuel

    m_fuel -= SERVER_ADVANCE_PERIOD * 10;
    m_fuel = max( m_fuel, 0.0f );
    
    //
    // Kill passengers

    if( m_passengers )
    {
        m_passengers--;
        
        int windowIndex = syncrand() % 3;
        Matrix34 mat( m_front, m_up, m_pos );
        Matrix34 windowMat = m_window[windowIndex]->GetWorldMatrix(mat);

        Vector3 vel = windowMat.f;
        float angle = syncsfrand( M_PI * 0.25f );
        vel.RotateAround( windowMat.u * angle );
        vel.SetLength( 10.0f + syncfrand(30.0f) );

        WorldObjectId id = g_app->m_location->SpawnEntities( windowMat.pos, 0, -1, Entity::TypeDarwinian, 1, vel, 0.0f );
        Darwinian *darwinian = (Darwinian *) g_app->m_location->GetEntity( id );
        darwinian->m_onGround = false;
        darwinian->SetFire();
    }
    
    
    if( m_fuel > 0.0f )
    {
        Matrix34 mat( m_front, g_upVector, m_pos );
        g_explosionManager.AddExplosion( m_shape, mat, 0.001f );
    }


    //
    // Spawn smoke and fire

    for( int i = 0; i < 3; ++i )
    {
        Matrix34 mat( m_front, m_up, m_pos );
        Matrix34 windowMat = m_window[i]->GetWorldMatrix(mat);

        Vector3 vel = windowMat.f;
        float angle = syncsfrand( M_PI * 0.25f );
        vel.RotateAround( windowMat.u * angle );
        vel.SetLength( 5.0f + syncfrand(10.0f) );        
        float fireSize = 150 + syncfrand(150.0f);

        Vector3 smokeVel = vel;
        float smokeSize = fireSize;

        if( m_fuel > 0.0f ) g_app->m_particleSystem->CreateParticle( windowMat.pos, vel, Particle::TypeFire, fireSize );
        g_app->m_particleSystem->CreateParticle( windowMat.pos, smokeVel, Particle::TypeMissileTrail, smokeSize );
    }


    if( m_damage <= 0.0f )
    {
        m_damage = 0.0f;
        m_spawnCompleted = true;
        m_state = StateRefueling;
    }
}


void EscapeRocket::SetupSpectacle()
{
    m_shadowTimer -= SERVER_ADVANCE_PERIOD;
    if( m_shadowTimer <= 0.0f )
    {
        for( int t = 0; t < NUM_TEAMS; ++t )
        {
            Team *team = &g_app->m_location->m_teams[t];
            for( int i = 0; i < team->m_others.Size(); ++i )
            {
                if( team->m_others.ValidIndex(i) )
                {
                    Entity *entity = team->m_others[i];
                    if( entity && entity->m_type == Entity::TypeDarwinian )
                    {
                        Darwinian *darwinian = (Darwinian *) entity;
                        //if( m_state == StateReady ) darwinian->CastShadow( m_id.GetUniqueId() );
                        // Causes too much of a slow down, and doesn't add much visually to the scene
                        if( t == 0 &&
                            darwinian->m_state == Darwinian::StateIdle &&
                            (syncrand() % 10) < 2 )
                        {
                            darwinian->WatchSpectacle( m_id.GetUniqueId() );
                        }
                    }
                }
            }
        }

        m_shadowTimer = 1.0f;
    }
}


bool EscapeRocket::IsSpectacle()
{
    return( m_state == StateIgnition ||
            m_state == StateReady ||
            m_state == StateCountdown ||
            m_state == StateFlight );
}


bool EscapeRocket::IsInView()
{
    return FuelBuilding::IsInView() || m_state == StateFlight;
}


void EscapeRocket::SetupAttackers()
{
    if( !m_spawnCompleted && syncfrand() < 0.2f )
    {
        Team *team = &g_app->m_location->m_teams[1];
        int numOthers = team->m_others.Size();
        if( numOthers > 0 )
        {
            int randomIndex = syncrand() % numOthers;
            if( team->m_others.ValidIndex(randomIndex) )
            {
                Entity *entity = team->m_others[randomIndex];
                if( entity && entity->m_type == Entity::TypeDarwinian )
                {
                    Darwinian *darwinian = (Darwinian *) entity;
                    float range = ( darwinian->m_pos - m_pos ).Mag();
                    if( range < 350.0f )
                    {
                        darwinian->AttackBuilding( m_id.GetUniqueId() );
                    }
                }
            }
        }
    }
}


void EscapeRocket::Damage( float _damage )
{
    FuelBuilding::Damage( _damage );

    if( m_state != StateExploding )
    {
        m_damage -= _damage;

        if( m_damage > 100.0f )
        {
            m_state = StateExploding;
            g_app->m_soundSystem->TriggerBuildingEvent( this, "Explode" );
        }
    }
}


bool EscapeRocket::Advance()
{    
    switch( m_state )
    {
        case StateRefueling:            AdvanceRefueling();             break;
        case StateLoading:              AdvanceLoading();               break;
        case StateIgnition:             AdvanceIgnition();              break;
        case StateReady:                AdvanceReady();                 break;
        case StateCountdown:            AdvanceCountdown();             break;
        case StateFlight:               AdvanceFlight();                break; 
        case StateExploding:            AdvanceExploding();             break;
    }
    

    SetupSounds();

    
    //
    // Create rocket flames
    // Shake the camera

    if( m_cameraShake > 0.0 )
    {
        m_cameraShake -= SERVER_ADVANCE_PERIOD;

        float actualShake = m_cameraShake/5.0f;
        g_app->m_camera->CreateCameraShake( actualShake );
    }


    if( m_state == StateReady ||
        m_state == StateCountdown || 
        m_state == StateFlight )
    {
        Matrix34 mat( m_front, g_upVector, m_pos );
        Vector3 boosterPos = m_booster->GetWorldMatrix(mat).pos;

        for( int i = 0; i < 15; ++i )
        {
            Vector3 pos = boosterPos;
            pos += Vector3( sfrand(20), 10, sfrand(20) );
            
            Vector3 vel( sfrand(50), -frand(150), sfrand(50) );
            float size = 500.0f;

            if( i > 10 )
            {
                vel.x *= 0.75f;
                vel.z *= 0.75f;
                g_app->m_particleSystem->CreateParticle( pos, vel, Particle::TypeMissileTrail, size );
            }
            else
            {
                g_app->m_particleSystem->CreateParticle( pos, vel, Particle::TypeMissileFire, size );
            }
        }
    }

    return FuelBuilding::Advance();
}


bool EscapeRocket::SafeToLaunch()
{
    Vector3 testPos = m_pos + Vector3(330,0,50);
    float testRadius = 100.0f;

    int numEnemies = g_app->m_location->m_entityGrid->GetNumEnemies( testPos.x, testPos.z, testRadius, 0 );

    return( numEnemies < 2 );
}


void EscapeRocket::Render( float _predictionTime )
{    
    Vector3 predictedPos = m_pos + m_vel * _predictionTime;
    
    Matrix34 mat( m_front, m_up, predictedPos );

    m_shape->Render( 0.0f, mat );   
}


void EscapeRocket::RenderAlphas( float _predictionTime )
{
    FuelBuilding::RenderAlphas( _predictionTime );
    
    if( g_app->m_editing )
    {
        Building *spawnBuilding = g_app->m_location->GetBuilding( m_spawnBuildingId );
        if( spawnBuilding )
        {
            RenderArrow( m_pos, spawnBuilding->m_pos, 1.0f );
        }

        return;
    }

    Vector3 predictedPos = m_pos + m_vel * _predictionTime;

    glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
//    g_editorFont.DrawText3DCentre( predictedPos+Vector3(0,120,0), 10, "Fuel : %2.2f", m_fuel );
//    g_editorFont.DrawText3DCentre( predictedPos+Vector3(0,130,0), 10, "Passengers : %d", m_passengers );
//    g_editorFont.DrawText3DCentre( predictedPos+Vector3(0,140,0), 10, "Damage : %2.2f", m_damage );
//    g_editorFont.DrawText3DCentre( predictedPos+Vector3(0,150,0), 10, "Timer : %2.2f", m_countdown );

//    if( m_state == StateCountdown && m_countdown <= 10.0f )
//    {
//        g_editorFont.DrawText3DCentre( predictedPos+Vector3(0,200,0), 100, "%d", int(m_countdown) );
//    }



    //
    // Render rocket glow
    // Calculate alpha value

    float alpha = m_fuel / 100.0f;
    alpha = min( alpha, 1.0f );
    alpha = max( alpha, 0.0f );

    if( alpha > 0.0f )
    {
        Vector3 camUp = g_app->m_camera->GetUp();
        Vector3 camRight = g_app->m_camera->GetRight() * 0.75f;
        
        glDepthMask     ( false );
        glEnable        ( GL_BLEND );
        glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );
        glEnable        ( GL_TEXTURE_2D );
        glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/starburst.bmp" ) );
        //glDisable       ( GL_DEPTH_TEST );

        float timeIndex = g_gameTime * 2;

        int buildingDetail = g_prefsManager->GetInt( "RenderBuildingDetail", 1 );
        int maxBlobs = 50;
        if( buildingDetail == 2 ) maxBlobs = 20;
        if( buildingDetail == 3 ) maxBlobs = 10;

        Vector3 boosterPos = predictedPos;
        boosterPos.y += 100;

    
        //
        // Central glow effect

        for( int i = 30; i < maxBlobs; ++i )
        {        
            Vector3 pos = boosterPos;
            pos.x += sinf(timeIndex*0.5+i) * i * 3.7f;
            pos.y += cosf(timeIndex*0.5+i) * cosf(i*20) * 50;
            pos.y += 40;
            pos.z += cosf(timeIndex*0.3+i) * i * 3.7f;

            float size = 20.0f * sinf(timeIndex+i*2);
            size = max( size, 5.0f );
        
            for( int j = 0; j < 2; ++j )
            {
                size *= 0.75f;
                glColor4f( 1.0f, 0.6f, 0.2f, alpha);        
                glBegin( GL_QUADS );
                    glTexCoord2i(0,0);      glVertex3fv( (pos - camRight * size + camUp * size).GetData() );
                    glTexCoord2i(1,0);      glVertex3fv( (pos + camRight * size + camUp * size).GetData() );
                    glTexCoord2i(1,1);      glVertex3fv( (pos + camRight * size - camUp * size).GetData() );
                    glTexCoord2i(0,1);      glVertex3fv( (pos - camRight * size - camUp * size).GetData() );
                glEnd();
            }
        }

    
        //
        // Central starbursts

        glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/starburst.bmp" ) );

        int numStars = 10;
        if( buildingDetail == 2 ) numStars = 5;
        if( buildingDetail == 3 ) numStars = 2;
    
        for( int i = 8; i < numStars; ++i )
        {
            Vector3 pos = boosterPos;
            pos.x += sinf(timeIndex+i) * i * 1.7f;
            pos.y += fabs(cosf(timeIndex+i) * cosf(i*20) * 64);
            pos.z += cosf(timeIndex+i) * i * 1.7f;

            float size = i * 30.0f;
        
            glColor4f( 1.0f, 0.4f, 0.2f, alpha );
            glBegin( GL_QUADS );
                glTexCoord2i(0,0);      glVertex3fv( (pos - camRight * size + camUp * size).GetData() );
                glTexCoord2i(1,0);      glVertex3fv( (pos + camRight * size + camUp * size).GetData() );
                glTexCoord2i(1,1);      glVertex3fv( (pos + camRight * size - camUp * size).GetData() );
                glTexCoord2i(0,1);      glVertex3fv( (pos - camRight * size - camUp * size).GetData() );
            glEnd();
        }

        glEnable( GL_DEPTH_TEST );
        glDisable( GL_TEXTURE_2D );    

    }

}


void EscapeRocket::Read( TextReader *_in, bool _dynamic )
{
    FuelBuilding::Read( _in, _dynamic );

    m_fuel              = atof( _in->GetNextToken() );
    m_passengers        = atoi( _in->GetNextToken() );
    m_spawnBuildingId   = atoi( _in->GetNextToken() );
    m_spawnCompleted    = atoi( _in->GetNextToken() );
}


void EscapeRocket::Write( FileWriter *_out )
{
    FuelBuilding::Write( _out );

    _out->printf( "%6.2f %6d %6d %6d", m_fuel, m_passengers, m_spawnBuildingId, (int) m_spawnCompleted );
}


int EscapeRocket::GetStateId( char *_state )
{
    static char *stateNames[] = {       
                                    "Refueling",
                                    "Loading",
                                    "Ignition",
                                    "Ready",
                                    "Countdown",
                                    "Exploding",
                                    "Flight"
                                };
    
    for( int i = 0; i < NumStates; ++i )
    {
        if( stricmp( stateNames[i], _state ) == 0 )
        {
            return i;
        }
    }

    return -1;
}



