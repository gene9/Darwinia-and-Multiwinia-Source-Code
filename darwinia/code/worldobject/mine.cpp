#include "lib/universal_include.h"

#include "lib/text_stream_readers.h"
#include "lib/shape.h"
#include "lib/debug_utils.h"
#include "lib/file_writer.h"
#include "lib/resource.h"
#include "lib/math_utils.h"
#include "lib/profiler.h"
#include "lib/debug_render.h"
#include "lib/text_renderer.h"
#include "lib/3d_sprite.h"
#include "lib/hi_res_time.h"
#include "lib/preferences.h"
#include "lib/language_table.h"

#ifdef CHEATMENU_ENABLED
#include "lib/input/input.h"
#include "lib/input/input_types.h"
#endif

#include "app.h"
#include "camera.h"
#include "global_world.h"
#include "location.h"
#include "main.h"
#include "particle_system.h"
#include "renderer.h"
#include "team.h"
#include "taskmanager.h"
#include "gamecursor.h"

#include "sound/soundsystem.h"

#include "worldobject/mine.h"
#include "worldobject/constructionyard.h"
#include "worldobject/trunkport.h"

// ****************************************************************************
// Class MineBuilding
// ****************************************************************************

Shape *MineBuilding::s_wheelShape = NULL;

Shape *MineBuilding::s_cartShape = NULL;
ShapeMarker *MineBuilding::s_cartMarker1 = NULL;
ShapeMarker *MineBuilding::s_cartMarker2 = NULL;
ShapeMarker *MineBuilding::s_cartContents[] = { NULL, NULL, NULL };

Shape *MineBuilding::s_polygon1 = NULL;
Shape *MineBuilding::s_primitive1 = NULL;

float MineBuilding::s_refineryPopulation = 0.0f;
float MineBuilding::s_refineryRecalculateTimer = 0.0f;


MineBuilding::MineBuilding()
:   Building(),
    m_trackLink(-1),
    m_trackMarker1(NULL),
    m_trackMarker2(NULL),
    m_previousMineSpeed(0.0f),
    m_wheelRotate(0.0f)
{    
    if( !s_cartShape )
    {
        s_wheelShape = g_app->m_resource->GetShape( "wheel.shp" );
        s_cartShape = g_app->m_resource->GetShape( "minecart.shp" );
        s_cartMarker1   = s_cartShape->m_rootFragment->LookupMarker( "MarkerTrack1" );
        s_cartMarker2   = s_cartShape->m_rootFragment->LookupMarker( "MarkerTrack2" );
        s_cartContents[0] = s_cartShape->m_rootFragment->LookupMarker( "MarkerContents1" );
        s_cartContents[1] = s_cartShape->m_rootFragment->LookupMarker( "MarkerContents2" );
        s_cartContents[2] = s_cartShape->m_rootFragment->LookupMarker( "MarkerContents3" );

        s_polygon1 = g_app->m_resource->GetShape( "minepolygon1.shp" );
        s_primitive1 = g_app->m_resource->GetShape( "mineprimitive1.shp" );
    }
}


void MineBuilding::Initialise( Building *_template )
{
    Building::Initialise( _template );
    m_trackLink = ((MineBuilding *) _template)->m_trackLink;

    float cart = 0.0f;

    while( true )
    {
        cart += 0.2f;
        cart += syncfrand(1.5f);
        if( cart >= 1.0f ) break;
        
        MineCart *mineCart = new MineCart();
        mineCart->m_progress = cart;
        m_carts.PutData( mineCart );               
    }
}


bool MineBuilding::IsInView()
{   
    Building *trackLink = g_app->m_location->GetBuilding( m_trackLink );
    if( trackLink )
    {
        Vector3 midPoint = (trackLink->m_centrePos + m_centrePos) / 2.0f;
        float radius = (trackLink->m_centrePos - m_centrePos).Mag() / 2.0f;
        radius += m_radius;        
        
        if( g_app->m_camera->SphereInViewFrustum( midPoint, radius ) )
        {
            return true;
        }                
    }

    return Building::IsInView();
}


void MineBuilding::RenderAlphas( float _predictionTime )
{
    Building::RenderAlphas( _predictionTime );
        
    Vector3 camPos = g_app->m_camera->GetPos();
    Vector3 camFront = g_app->m_camera->GetFront();
    Vector3 camUp = g_app->m_camera->GetUp();
    
    if( m_trackLink != -1 )
    {
        Building *trackLink = g_app->m_location->GetBuilding( m_trackLink );
        if( trackLink )
        {
            int buildingDetail = g_prefsManager->GetInt( "RenderBuildingDetail", 1 );

            MineBuilding *mineBuilding = (MineBuilding *) trackLink;

            Vector3 ourPos1 = GetTrackMarker1();
            Vector3 theirPos1 = mineBuilding->GetTrackMarker1();        

            Vector3 ourPos2 = GetTrackMarker2();
            Vector3 theirPos2 = mineBuilding->GetTrackMarker2();        

            glColor4f( 0.85f, 0.4f, 0.4f, 1.0f );
        
            float size = 2.0f;
            if( buildingDetail > 1 ) size = 1.0f;

            Vector3 camToOurPos1 = g_app->m_camera->GetPos() - ourPos1;
            Vector3 lineOurPos1 = camToOurPos1 ^ ( ourPos1 - theirPos1 );
            lineOurPos1.SetLength( size );

            Vector3 camToTheirPos1 = g_app->m_camera->GetPos() - theirPos1;
            Vector3 lineTheirPos1 = camToTheirPos1 ^ ( ourPos1 - theirPos1 );
            lineTheirPos1.SetLength( size );
            
            glDisable       ( GL_CULL_FACE );
            
            if( buildingDetail == 1 )
            {
                glEnable        ( GL_TEXTURE_2D );
                glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/laser.bmp" ) );
                glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
                glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
        
                glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );
                glEnable        ( GL_BLEND );
            }

            glDepthMask     ( false );

            glBegin( GL_QUADS );
                glTexCoord2f(0.1,0);      glVertex3fv( (ourPos1 - lineOurPos1).GetData() );
                glTexCoord2f(0.1,1);      glVertex3fv( (ourPos1 + lineOurPos1).GetData() );
                glTexCoord2f(0.9,1);      glVertex3fv( (theirPos1 + lineTheirPos1).GetData() );
                glTexCoord2f(0.9,0);      glVertex3fv( (theirPos1 - lineTheirPos1).GetData() );
            glEnd();

            glBegin( GL_QUADS );
                glTexCoord2f(0.1,0);      glVertex3fv( (ourPos2 - lineOurPos1).GetData() );
                glTexCoord2f(0.1,1);      glVertex3fv( (ourPos2 + lineOurPos1).GetData() );
                glTexCoord2f(0.9,1);      glVertex3fv( (theirPos2 + lineTheirPos1).GetData() );
                glTexCoord2f(0.9,0);      glVertex3fv( (theirPos2 - lineTheirPos1).GetData() );
            glEnd();
            
            glDepthMask ( true );
            glBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
            
            glDisable   ( GL_TEXTURE_2D );
            glEnable    ( GL_CULL_FACE );
        }
    }
}


void MineBuilding::Render( float _predictionTime )
{            
    _predictionTime -= 0.1f;
    
    for( int i = 0; i < m_carts.Size(); ++i )
    {
        MineCart *thisCart = m_carts[i];
        RenderCart( thisCart, _predictionTime );
    }

    Building::Render( _predictionTime );    
}


void MineBuilding::RenderCart( MineCart *_cart, float _predictionTime )
{
	//START_PROFILE(g_app->m_profiler, "Mine Cart");

    if( m_trackLink != -1 )
    {
        Building *trackLink = g_app->m_location->GetBuilding( m_trackLink );
        if( !trackLink ) return;
        MineBuilding *mineBuilding = (MineBuilding *) trackLink;

        int buildingDetail = g_prefsManager->GetInt( "RenderBuildingDetail", 1 );

        Vector3 ourMarker1 = GetTrackMarker1();
        Vector3 ourMarker2 = GetTrackMarker2();
        Vector3 theirMarker1 = mineBuilding->GetTrackMarker1();
        Vector3 theirMarker2 = mineBuilding->GetTrackMarker2();

        float mineSpeed = RefinerySpeed();

        float predictedProgress = _cart->m_progress;
        predictedProgress += _predictionTime * mineSpeed;
        if( predictedProgress < 0.0f ) predictedProgress = 0.0f;
        if( predictedProgress > 1.0f ) predictedProgress = 1.0f;

        Vector3 trackLeft = ourMarker1 + (theirMarker1 - ourMarker1) * predictedProgress;
        Vector3 trackRight = ourMarker2 + (theirMarker2 - ourMarker2) * predictedProgress;

        Vector3 cartPos = (trackLeft + trackRight)/2.0f;
        cartPos += Vector3(0,-40,0);

        if( g_app->m_camera->PosInViewFrustum( cartPos ) )
        {
            Vector3 cartFront = ( trackLeft - trackRight ) ^ g_upVector;
            cartFront.y = 0.0f;
            cartFront.Normalise();

            //START_PROFILE(g_app->m_profiler, "RenderCartShape" );
            Matrix34 transform( cartFront, g_upVector, cartPos );
            s_cartShape->Render( 0.0f, transform );
            //END_PROFILE(g_app->m_profiler, "RenderCartShape" );
        
            Vector3 cartLinkLeft = s_cartMarker1->GetWorldMatrix( transform ).pos;
            Vector3 cartLinkRight = s_cartMarker2->GetWorldMatrix( transform ).pos;

            //START_PROFILE(g_app->m_profiler, "RenderLines" );
            Vector3 camRight = g_app->m_camera->GetRight() * 0.5f;            
            glBegin( GL_QUADS );
                glVertex3fv( (trackLeft - camRight).GetData() );
                glVertex3fv( (trackLeft + camRight).GetData() );
                glVertex3fv( (cartLinkLeft + camRight).GetData() );
                glVertex3fv( (cartLinkLeft - camRight).GetData() );

                glVertex3fv( (trackRight - camRight).GetData() );
                glVertex3fv( (trackRight + camRight).GetData() );
                glVertex3fv( (cartLinkRight + camRight).GetData() );
                glVertex3fv( (cartLinkRight - camRight).GetData() );
            glEnd();
            //END_PROFILE(g_app->m_profiler, "RenderLines" );
        
            //START_PROFILE(g_app->m_profiler, "RenderPolygons" );
            for( int i = 0; i < 3; ++i )
            {            
                if( _cart->m_polygons[i] )
                {
                    Matrix34 polyMat = s_cartContents[i]->GetWorldMatrix( transform );            
                    s_polygon1->Render( 0.0f, polyMat );
                }

                if( _cart->m_primitives[i] )
                {
                    Matrix34 polyMat = s_cartContents[i]->GetWorldMatrix( transform );            
                    s_primitive1->Render( 0.0f, polyMat );

                    if( buildingDetail < 3 )
                    {
                        glEnable( GL_BLEND );
                        glBlendFunc( GL_SRC_ALPHA, GL_ONE );
                        glDepthMask( false );
                        //glDisable( GL_DEPTH_TEST );
                        glDisable( GL_LIGHTING );

                        glColor4f( 1.0f, 0.7f, 0.0f, 0.75f );
                      
	                    float nearPlaneStart = g_app->m_renderer->GetNearPlane();
	                    g_app->m_camera->SetupProjectionMatrix(nearPlaneStart * 1.1f,
							 			                       g_app->m_renderer->GetFarPlane());                    
                    
                        Render3DSprite( polyMat.pos - Vector3(0,25,0), 50.0f, 50.0f, g_app->m_resource->GetTexture( "textures/glow.bmp" ) );                                                           

                        g_app->m_camera->SetupProjectionMatrix(nearPlaneStart,
								 		                       g_app->m_renderer->GetFarPlane());
                    
                        glEnable( GL_LIGHTING );
                        glEnable( GL_DEPTH_TEST );
                        glDepthMask( true );
                        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
                        glDisable( GL_BLEND );
                    }
                }        
            }
            //END_PROFILE(g_app->m_profiler, "RenderPolygons" );
        }
    }

	//END_PROFILE(g_app->m_profiler, "Mine Cart");
}


bool MineBuilding::Advance()
{
    float mineSpeed = RefinerySpeed();
    if( m_previousMineSpeed <= 0.1f && mineSpeed > 0.1f )
    {
        g_app->m_soundSystem->TriggerBuildingEvent( this, "CogTurn" );
    }
    m_previousMineSpeed = mineSpeed;

    if( mineSpeed > 0.0f )
    {
        for( int i = m_carts.Size()-1; i >= 0; --i )
        {
            MineCart *thisCart = m_carts[i];
            thisCart->m_progress += mineSpeed * SERVER_ADVANCE_PERIOD;

            if( thisCart->m_progress >= 1.0f )
            {
                float remainder = thisCart->m_progress - 1.0f;
                m_carts.RemoveData(i);
                --i;
            
                Building *trackLink = g_app->m_location->GetBuilding( m_trackLink );
                if( trackLink )
                {
                    MineBuilding *mineBuilding = (MineBuilding *) trackLink;
                    mineBuilding->TriggerCart( thisCart, remainder );
                }
            }
        }
    }

    //
    // Rotate our wheels
    
    m_wheelRotate -= 3.0f * mineSpeed * SERVER_ADVANCE_PERIOD;

    return Building::Advance();
}


void MineBuilding::ListSoundEvents( LList<char *> *_list )
{
    Building::ListSoundEvents( _list );

    _list->PutData( "CogTurn" );
}


void MineBuilding::TriggerCart ( MineCart *_cart, float _initValue )
{
    _cart->m_progress = _initValue;
    m_carts.PutDataAtStart( _cart );
}


Vector3 MineBuilding::GetTrackMarker1()
{
    if( !m_trackMarker1 || g_app->m_editing )
    {
        m_trackMarker1 = m_shape->m_rootFragment->LookupMarker( "MarkerTrack1" );
        DarwiniaDebugAssert( m_trackMarker1 );
        Matrix34 rootMat( m_front, g_upVector, m_pos );
        m_trackMatrix1 = m_trackMarker1->GetWorldMatrix( rootMat );
    }
    
    return m_trackMatrix1.pos;
}


Vector3 MineBuilding::GetTrackMarker2()
{
    if( !m_trackMarker2 || g_app->m_editing )
    {
        m_trackMarker2 = m_shape->m_rootFragment->LookupMarker( "MarkerTrack2" );
        DarwiniaDebugAssert( m_trackMarker2 );
        Matrix34 rootMat( m_front, g_upVector, m_pos );
        m_trackMatrix2 = m_trackMarker2->GetWorldMatrix( rootMat );
    }
    
    return m_trackMatrix2.pos;
}


void MineBuilding::Read( TextReader *_in, bool _dynamic )
{
    Building::Read( _in, _dynamic );
    m_trackLink = atoi( _in->GetNextToken() );
}


void MineBuilding::Write( FileWriter *_out )
{
    Building::Write( _out );
    _out->printf( "%-8d", m_trackLink );
}


int MineBuilding::GetBuildingLink()
{
    return m_trackLink;
}


void MineBuilding::SetBuildingLink( int _buildingId )
{
    m_trackLink = _buildingId;
}


float MineBuilding::RefinerySpeed()
{
    if( GetHighResTime() >= s_refineryRecalculateTimer )
    {
        //
        // Find the refinery
        // If not a refinery, look for a construction yard
        // If not, look for a fuel generator

        Building *driver = NULL;
        
        int numFuelGenerators = 0;
        float fuelGeneratorFactor = 0.0f;

        for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
        {
            if( g_app->m_location->m_buildings.ValidIndex(i) )
            {
                Building *building = g_app->m_location->m_buildings[i];
                if( building->m_type == TypeRefinery ||
                    building->m_type == TypeYard )
                {
                    driver = building;
                    break;
                }
            }
        }

        //
        // If we found a refinery calculate our speed based on that
        // If not, there may be a construction yard driving us
        // If not, there may be a Fuel Generator driving us
        // Otherwise assume the refinery is on a different level, and use global state data
         
        if( driver )
        {
            int numPorts = driver->GetNumPorts();
            int numPortsOccupied = driver->GetNumPortsOccupied();
            s_refineryPopulation = (float) numPortsOccupied / (float) numPorts;
        }
        else if( numFuelGenerators > 0 )
        {
            s_refineryPopulation = fuelGeneratorFactor / (float) numFuelGenerators;
        }
        else
        {
            int mineLocationId = g_app->m_globalWorld->GetLocationId("mine");
            s_refineryPopulation = 0.0f;

            GlobalBuilding *globalRefinery = NULL;
            for( int i = 0; i < g_app->m_globalWorld->m_buildings.Size(); ++i )
            {
                if( g_app->m_globalWorld->m_buildings.ValidIndex(i) )
                {
                    GlobalBuilding *gb = g_app->m_globalWorld->m_buildings[i];
                    if( gb && gb->m_locationId == mineLocationId && 
                        gb->m_type == TypeRefinery && gb->m_online )
                    {
                        s_refineryPopulation = 1.0f;
                        break;
                    }
                }
            }
        }

        s_refineryRecalculateTimer = GetHighResTime() + 0.1f;
    }

    float speed = s_refineryPopulation * fabs(sinf( g_gameTime * 2.0f )) * 0.5f;   
    
#ifdef CHEATMENU_ENABLED
    if( g_inputManager->controlEvent( ControlScrollSpeedup ) )
    {
        speed *= 10.0f;
    }
#endif

    return speed;
}

// ****************************************************************************
// Class MineCart
// ****************************************************************************

MineCart::MineCart()
:   m_progress(0.0f)
{
    for( int i = 0; i < 3; ++i )
    {
        m_polygons[i] = false;
        m_primitives[i] = false;
    }
}


// ****************************************************************************
// Class TrackLink
// ****************************************************************************

TrackLink::TrackLink()
:   MineBuilding()
{
    m_type = TypeTrackLink;
    SetShape( g_app->m_resource->GetShape( "tracklink.shp" ) );
}


bool TrackLink::Advance()
{
    return MineBuilding::Advance();
}


// ****************************************************************************
// Class TrackJunction
// ****************************************************************************

TrackJunction::TrackJunction()
:   MineBuilding()
{
    m_type = TypeTrackJunction;
    SetShape( g_app->m_resource->GetShape( "tracklink.shp" ) );
}


void TrackJunction::Initialise( Building *_template )
{
    MineBuilding::Initialise( _template );

    TrackJunction *trackJunction = (TrackJunction *) _template;
    for( int i = 0; i < trackJunction->m_trackLinks.Size(); ++i )
    {
        int trackLink = trackJunction->m_trackLinks[i];
        m_trackLinks.PutData( trackLink );
    }
}


void TrackJunction::Render( float _predictionTime )
{
    Building::Render( _predictionTime );
}


void TrackJunction::RenderLink()
{
#ifdef DEBUG_RENDER_ENABLED
    for( int i = 0; i < m_trackLinks.Size(); ++i )
    {
        int buildingId = m_trackLinks[i];
        if( buildingId != -1 )
        {
            Building *linkBuilding = g_app->m_location->GetBuilding( buildingId );
            if( linkBuilding )
            {
			    Vector3 start = m_pos;
			    start.y += 10.0f;
			    Vector3 end = linkBuilding->m_pos;
			    end.y += 10.0f;
			    RenderArrow(start, end, 6.0f, RGBAColour(255,0,255));
            }
        }
    }
#endif
}


void TrackJunction::TriggerCart( MineCart *_cart, float _initValue )
{
    if( m_trackLinks.Size() > 0 )
    {
        int chosenLink = syncrand() % m_trackLinks.Size();
        int buildingId = m_trackLinks[ chosenLink ];
        Building *linkBuilding = g_app->m_location->GetBuilding( buildingId );
        if( linkBuilding )
        {
            MineBuilding *mine = (MineBuilding *) linkBuilding;
            mine->TriggerCart( _cart, _initValue );
        }
    }
}


void TrackJunction::SetBuildingLink( int _buildingId )
{
    m_trackLinks.PutData( _buildingId );
}


void TrackJunction::Read( TextReader *_in, bool _dynamic )
{
    MineBuilding::Read( _in, _dynamic );

    while( _in->TokenAvailable() )
    {
        int trackLink = atoi( _in->GetNextToken() );
        m_trackLinks.PutData( trackLink );
    }
}


void TrackJunction::Write( FileWriter *_out )
{
    MineBuilding::Write( _out );

    for( int i = 0; i < m_trackLinks.Size(); ++i )
    {
        _out->printf( "%-4d", m_trackLinks[i] );
    }
}


// ****************************************************************************
// Class TrackStart
// ****************************************************************************

TrackStart::TrackStart()
:   MineBuilding(),
    m_reqBuildingId(-1)
{
    m_type = TypeTrackStart;
    SetShape( g_app->m_resource->GetShape( "tracklink.shp" ) );
}


void TrackStart::Initialise( Building *_template )
{
    MineBuilding::Initialise( _template );

    m_reqBuildingId = ((TrackStart *) _template)->m_reqBuildingId;
}


bool TrackStart::Advance()
{
    //
    // Is our required building online yet?
    // Fill carts with primitives when they reach 50%

    GlobalBuilding *globalBuilding = g_app->m_globalWorld->GetBuilding( m_reqBuildingId, g_app->m_locationId );
    
    if( globalBuilding && globalBuilding->m_online )
    {
        for( int i = 0; i < m_carts.Size(); ++i )
        {
            MineCart *cart = m_carts[i];    
        
            if( cart->m_progress > 0.5f )
            {
                bool primitiveFound = false;
                for( int j = 0; j < 3; ++j )
                {
                    if( cart->m_primitives[j] )
                    {
                        primitiveFound = true;
                        break;
                    }
                }

                if( !primitiveFound )
                {
                    int primIndex = syncrand() % 3;
                    cart->m_primitives[primIndex] = true;                   
                }
            }
        }
    }
    
    return MineBuilding::Advance();
}


void TrackStart::RenderAlphas( float _predictionTime )
{
    MineBuilding::RenderAlphas( _predictionTime );

#ifdef DEBUG_RENDER_ENABLED
    if( g_app->m_editing )
    {
        Building *req = g_app->m_location->GetBuilding( m_reqBuildingId );
        if( req )
        {
            RenderArrow( m_pos+Vector3(0,50,0), 
                         req->m_pos+Vector3(0,50,0), 
                         2.0f, RGBAColour(255,0,0) );
        }
    }
#endif
}


void TrackStart::Read( TextReader *_in, bool _dynamic )
{
    MineBuilding::Read( _in, _dynamic );

    m_reqBuildingId = atoi( _in->GetNextToken() );    
}


void TrackStart::Write( FileWriter *_out )
{
    MineBuilding::Write( _out );

    _out->printf( "%-8d", m_reqBuildingId );
}


// ****************************************************************************
// Class TrackEnd
// ****************************************************************************

TrackEnd::TrackEnd()
:   MineBuilding(),
    m_reqBuildingId(-1)
{
    m_type = TypeTrackEnd;
    SetShape( g_app->m_resource->GetShape( "trackLink.shp" ) );
}


void TrackEnd::Initialise( Building *_template )
{
    MineBuilding::Initialise( _template );

    m_reqBuildingId = ((TrackEnd *) _template)->m_reqBuildingId;
}


bool TrackEnd::Advance()
{
    //
    // Is our required building online yet?
    // Empty carts of primitives when they reach 50%

    //GlobalBuilding *globalBuilding = g_app->m_globalWorld->GetBuilding( m_reqBuildingId, g_app->m_locationId );
    Building *building = g_app->m_location->GetBuilding( m_reqBuildingId );
    
    bool online = false;
    if( building->m_type == TypeTrunkPort && ((TrunkPort *)building)->m_openTimer > 0.0f ) online = true;
    if( building->m_type == TypeYard ) online = true;
    if( building->m_type == TypeFuelGenerator ) online = true;

    if( online )
    {
        for( int i = 0; i < m_carts.Size(); ++i )
        {
            MineCart *cart = m_carts[i];    

            if( cart->m_progress > 0.5f )
            {
                for( int j = 0; j < 3; ++j )
                {
                    if( cart->m_primitives[j] )
                    {
                        if( building && building->m_type == TypeYard )
                        {
                            ConstructionYard *yard = (ConstructionYard *) building;
                            bool added = yard->AddPrimitive();
                            if( added ) cart->m_primitives[j] = false;
                        }
                        else
                        {
                            cart->m_primitives[j] = false;
                        }
                    }

                    if( cart->m_polygons[j] )
                    {
                        cart->m_polygons[j] = false;
                    }
                }
            }
        }
    }

    return MineBuilding::Advance();
}


void TrackEnd::RenderAlphas( float _predictionTime )
{
    MineBuilding::RenderAlphas( _predictionTime );

#ifdef DEBUG_RENDER_ENABLED
    if( g_app->m_editing )
    {
        Building *req = g_app->m_location->GetBuilding( m_reqBuildingId );
        if( req )
        {
            RenderArrow( m_pos+Vector3(0,50,0), 
                         req->m_pos+Vector3(0,50,0), 
                         2.0f, RGBAColour(255,0,0) );
        }
    }
#endif
}


void TrackEnd::Read( TextReader *_in, bool _dynamic )
{
    MineBuilding::Read( _in, _dynamic );

    m_reqBuildingId = atoi( _in->GetNextToken() );    
}


void TrackEnd::Write( FileWriter *_out )
{
    MineBuilding::Write( _out );

    _out->printf( "%-8d", m_reqBuildingId );
}


// ****************************************************************************
// Class Refinery
// ****************************************************************************

Refinery::Refinery()
:   MineBuilding(),
    m_wheel1(NULL),
    m_wheel2(NULL),
    m_wheel3(NULL),
    m_counter1(NULL)
{
    m_type = TypeRefinery;
    SetShape( g_app->m_resource->GetShape( "refinery.shp" ) );

    m_wheel1 = m_shape->m_rootFragment->LookupMarker( "MarkerWheel01" );
    m_wheel2 = m_shape->m_rootFragment->LookupMarker( "MarkerWheel02" );
    m_wheel3 = m_shape->m_rootFragment->LookupMarker( "MarkerWheel03" );

    m_counter1 = m_shape->m_rootFragment->LookupMarker( "MarkerCounter" );
}


char *Refinery::GetObjectiveCounter()
{
    GlobalBuilding *gb = g_app->m_globalWorld->GetBuilding( m_id.GetUniqueId(), g_app->m_locationId );
    int numRefined = 0;
    if( gb ) numRefined = gb->m_link;
    
    static char result[256];
    sprintf( result, "%s : %d", LANGUAGEPHRASE("objective_refined"), numRefined );
    return result;
}


bool Refinery::Advance()
{
    GlobalBuilding *gb = g_app->m_globalWorld->GetBuilding( m_id.GetUniqueId(), g_app->m_locationId );
    
    if(gb && gb->m_link == -1 )
    {
        // This occurs the first time
        gb->m_link = 0;
    }

    if(gb && gb->m_link >= 20)
    {
        if( !gb->m_online )
        {
            gb->m_online = true;
            g_app->m_globalWorld->EvaluateEvents();
        }        
    }
    
    return MineBuilding::Advance();
}


void Refinery::TriggerCart( MineCart *_cart, float _initValue )
{
    // The refinery MUST be online at this point,
    // otherwise the carts would never have reached us
    
    bool polygonsFound = false;
    bool primitivesFound = false;

    for( int i = 0; i < 3; ++i )
    {
        if( _cart->m_polygons[i] ) polygonsFound = true;
        if( _cart->m_primitives[i] ) primitivesFound = true;
    }

    if( !primitivesFound && polygonsFound )
    {
        for( int i = 0; i < 3; ++i )
        {
            _cart->m_polygons[i] = false;
        }

        int primIndex = syncrand() % 3;
        _cart->m_primitives[primIndex] = true;
        
        GlobalBuilding *gb = g_app->m_globalWorld->GetBuilding( m_id.GetUniqueId(), g_app->m_locationId );

        if( gb ) gb->m_link++;
    }

    MineBuilding::TriggerCart( _cart, _initValue );
}


void Refinery::Render( float _predictionTime )
{
    MineBuilding::Render( _predictionTime );
    
    //
    // Render wheels

    Matrix34 refineryMat( m_front, g_upVector, m_pos );
    Matrix34 wheel1Mat = m_wheel1->GetWorldMatrix( refineryMat );
    Matrix34 wheel2Mat = m_wheel2->GetWorldMatrix( refineryMat );
    Matrix34 wheel3Mat = m_wheel3->GetWorldMatrix( refineryMat );

    float refinerySpeed = RefinerySpeed();
    float predictedWheelRotate = m_wheelRotate - 3.0f * refinerySpeed * _predictionTime;

    wheel1Mat.RotateAroundF( predictedWheelRotate );
    wheel2Mat.RotateAroundF( predictedWheelRotate * -1.0f );
    wheel3Mat.RotateAroundF( predictedWheelRotate );

    wheel1Mat.r *= 1.3f;
    wheel1Mat.u *= 1.3f;
    wheel1Mat.f *= 1.3f;

    wheel2Mat.r *= 0.8f;
    wheel2Mat.u *= 0.8f;
    
    wheel3Mat.r *= 1.6f;
    wheel3Mat.u *= 1.6f;
    wheel3Mat.f *= 1.6f;
    
    s_wheelShape->Render( _predictionTime, wheel1Mat );
    s_wheelShape->Render( _predictionTime, wheel2Mat );
    s_wheelShape->Render( _predictionTime, wheel3Mat );

    GlobalBuilding *gb = g_app->m_globalWorld->GetBuilding( m_id.GetUniqueId(), g_app->m_locationId );
    int numRefined = 0;
    if( gb ) numRefined = gb->m_link;

    Matrix34 counterMat = m_counter1->GetWorldMatrix(refineryMat);
    glColor4f( 0.6f, 0.8f, 0.9f, 1.0f );
    g_gameFont.DrawText3D( counterMat.pos, counterMat.f, counterMat.u, 10.0f, "%d", numRefined );
    counterMat.pos += counterMat.f * 0.1f;
    counterMat.pos += ( counterMat.f ^ counterMat.u ) * 0.2f;
    counterMat.pos += counterMat.u * 0.2f;    
    g_gameFont.SetRenderShadow(true);
    glColor4f( 0.6f, 0.8f, 0.9f, 0.0f );
    g_gameFont.DrawText3D( counterMat.pos, counterMat.f, counterMat.u, 10.0f, "%d", numRefined );
    g_gameFont.SetRenderShadow(false);
}


// ****************************************************************************
// Class Mine
// ****************************************************************************

Mine::Mine()
:   MineBuilding(),
    m_wheel1(NULL),
    m_wheel2(NULL)
{
    m_type = TypeMine;
    SetShape( g_app->m_resource->GetShape( "mine.shp" ) );

    m_wheel1 = m_shape->m_rootFragment->LookupMarker( "MarkerWheel01" );
    m_wheel2 = m_shape->m_rootFragment->LookupMarker( "MarkerWheel02" );
}


void Mine::Render( float _predictionTime )
{
    MineBuilding::Render( _predictionTime );

    //
    // Render wheels
    
    Matrix34 mineMat( m_front, g_upVector, m_pos );
    Matrix34 wheel1Mat = m_wheel1->GetWorldMatrix( mineMat );
    Matrix34 wheel2Mat = m_wheel2->GetWorldMatrix( mineMat );

    float refinerySpeed = RefinerySpeed();
    float predictedWheelRotate = m_wheelRotate - 3.0f * refinerySpeed * _predictionTime;

    wheel1Mat.RotateAroundF( predictedWheelRotate * -1.0f );
    wheel2Mat.RotateAroundF( predictedWheelRotate );

    wheel1Mat.r *= 0.5f;
    wheel1Mat.u *= 0.5f;
    
    wheel2Mat.r *= 0.9f;
    wheel2Mat.u *= 0.9f;
        
    s_wheelShape->Render( _predictionTime, wheel1Mat );
    s_wheelShape->Render( _predictionTime, wheel2Mat );
}


void Mine::TriggerCart( MineCart *_cart, float _initValue )
{
    //
    // If there is anything already in here, don't do anything

    bool somethingFound = false;
    for( int i = 0; i < 3; ++i )
    {
        if( _cart->m_primitives[i] || _cart->m_polygons[i] )
        {
            somethingFound = true;
            break;
        }
    }


    if( !somethingFound )
    {
        float fractionOccupied = (float) GetNumPortsOccupied() / (float) GetNumPorts();

        if( syncfrand() <= fractionOccupied )
        {
            for( int i = 0; i < 3; ++i )
            {
                _cart->m_primitives[i] = false;
                int cartIndex = syncrand() % 3;
                _cart->m_polygons[cartIndex] = true;
            }
        }
    }

    MineBuilding::TriggerCart( _cart, _initValue );
}
