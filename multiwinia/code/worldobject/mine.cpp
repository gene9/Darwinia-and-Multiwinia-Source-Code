#include "lib/universal_include.h"

#include "lib/filesys/text_stream_readers.h"
#include "lib/shape.h"
#include "lib/debug_utils.h"
#include "lib/filesys/text_file_writer.h"
#include "lib/resource.h"
#include "lib/math_utils.h"
#include "lib/profiler.h"
#include "lib/debug_render.h"
#include "lib/text_renderer.h"
#include "lib/3d_sprite.h"
#include "lib/hi_res_time.h"
#include "lib/preferences.h"
#include "lib/language_table.h"
#include "lib/math/random_number.h"

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

double MineBuilding::s_refineryPopulation = 0.0;
double MineBuilding::s_refineryRecalculateTimer = 0.0;


MineBuilding::MineBuilding()
:   Building(),
    m_trackLink(-1),
    m_trackMarker1(NULL),
    m_trackMarker2(NULL),
    m_previousMineSpeed(0.0),
    m_wheelRotate(0.0)
{    
    if( !s_cartShape )
    {
        s_wheelShape = g_app->m_resource->GetShape( "wheel.shp" );
        s_cartShape = g_app->m_resource->GetShape( "minecart.shp" );

        const char cartMarker1Name[] = "MarkerTrack1";
        s_cartMarker1   = s_cartShape->m_rootFragment->LookupMarker( cartMarker1Name );
        AppReleaseAssert( s_cartMarker1, "MineBuilding: Can't get Marker(%s) from shape(%s), probably a corrupted file\n", cartMarker1Name, s_cartShape->m_name );

        const char cartMarker2Name[] = "MarkerTrack2";
        s_cartMarker2   = s_cartShape->m_rootFragment->LookupMarker( cartMarker2Name );
        AppReleaseAssert( s_cartMarker2, "MineBuilding: Can't get Marker(%s) from shape(%s), probably a corrupted file\n", cartMarker2Name, s_cartShape->m_name );

        const char cartContents0Name[] = "MarkerContents1";
        s_cartContents[0] = s_cartShape->m_rootFragment->LookupMarker( cartContents0Name );
        AppReleaseAssert( s_cartContents[0], "MineBuilding: Can't get Marker(%s) from shape(%s), probably a corrupted file\n", cartContents0Name, s_cartShape->m_name );

        const char cartContents1Name[] = "MarkerContents2";
        s_cartContents[1] = s_cartShape->m_rootFragment->LookupMarker( cartContents1Name );
        AppReleaseAssert( s_cartContents[1], "MineBuilding: Can't get Marker(%s) from shape(%s), probably a corrupted file\n", cartContents1Name, s_cartShape->m_name );

        const char cartContents2Name[] = "MarkerContents3";
        s_cartContents[2] = s_cartShape->m_rootFragment->LookupMarker( cartContents2Name );
        AppReleaseAssert( s_cartContents[2], "MineBuilding: Can't get Marker(%s) from shape(%s), probably a corrupted file\n", cartContents2Name, s_cartShape->m_name );

        s_polygon1 = g_app->m_resource->GetShape( "minepolygon1.shp" );
        s_primitive1 = g_app->m_resource->GetShape( "mineprimitive1.shp" );
    }
}


void MineBuilding::Initialise( Building *_template )
{
    Building::Initialise( _template );
    m_trackLink = ((MineBuilding *) _template)->m_trackLink;

    double cart = 0.0;

    while( true )
    {
        cart += 0.2;
        cart += syncfrand(1.5);
        if( cart >= 1.0 ) break;
        
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
        Vector3 midPoint = (trackLink->m_centrePos + m_centrePos) / 2.0;
        double radius = (trackLink->m_centrePos - m_centrePos).Mag() / 2.0;
        radius += m_radius;        
        
        if( g_app->m_camera->SphereInViewFrustum( midPoint, radius ) )
        {
            return true;
        }                
    }

    return Building::IsInView();
}


void MineBuilding::RenderAlphas( double _predictionTime )
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

            glColor4f( 0.85, 0.4, 0.4, 1.0 );
        
            double size = 2.0;
            if( buildingDetail > 1 ) size = 1.0;

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
                glTexCoord2f(0.1,0);      glVertex3dv( (ourPos1 - lineOurPos1).GetData() );
                glTexCoord2f(0.1,1);      glVertex3dv( (ourPos1 + lineOurPos1).GetData() );
                glTexCoord2f(0.9,1);      glVertex3dv( (theirPos1 + lineTheirPos1).GetData() );
                glTexCoord2f(0.9,0);      glVertex3dv( (theirPos1 - lineTheirPos1).GetData() );
            glEnd();

            glBegin( GL_QUADS );
                glTexCoord2f(0.1,0);      glVertex3dv( (ourPos2 - lineOurPos1).GetData() );
                glTexCoord2f(0.1,1);      glVertex3dv( (ourPos2 + lineOurPos1).GetData() );
                glTexCoord2f(0.9,1);      glVertex3dv( (theirPos2 + lineTheirPos1).GetData() );
                glTexCoord2f(0.9,0);      glVertex3dv( (theirPos2 - lineTheirPos1).GetData() );
            glEnd();
            
            glDepthMask ( true );
            glBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
            
            glDisable   ( GL_TEXTURE_2D );
            glEnable    ( GL_CULL_FACE );
        }
    }
}


void MineBuilding::Render( double _predictionTime )
{            
    _predictionTime -= 0.1;
    
    for( int i = 0; i < m_carts.Size(); ++i )
    {
        MineCart *thisCart = m_carts[i];
        RenderCart( thisCart, _predictionTime );
    }

    Building::Render( _predictionTime );    
}


void MineBuilding::RenderCart( MineCart *_cart, double _predictionTime )
{
	//START_PROFILE( "Mine Cart");

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

        double mineSpeed = RefinerySpeed();

        double predictedProgress = _cart->m_progress;
        predictedProgress += _predictionTime * mineSpeed;
        if( predictedProgress < 0.0 ) predictedProgress = 0.0;
        if( predictedProgress > 1.0 ) predictedProgress = 1.0;

        Vector3 trackLeft = ourMarker1 + (theirMarker1 - ourMarker1) * predictedProgress;
        Vector3 trackRight = ourMarker2 + (theirMarker2 - ourMarker2) * predictedProgress;

        Vector3 cartPos = (trackLeft + trackRight)/2.0;
        cartPos += Vector3(0,-40,0);

        if( g_app->m_camera->PosInViewFrustum( cartPos ) )
        {
            Vector3 cartFront = ( trackLeft - trackRight ) ^ g_upVector;
            cartFront.y = 0.0;
            cartFront.Normalise();

            //START_PROFILE( "RenderCartShape" );
            Matrix34 transform( cartFront, g_upVector, cartPos );
            s_cartShape->Render( 0.0, transform );
            //END_PROFILE( "RenderCartShape" );
        
            Vector3 cartLinkLeft = s_cartMarker1->GetWorldMatrix( transform ).pos;
            Vector3 cartLinkRight = s_cartMarker2->GetWorldMatrix( transform ).pos;

            //START_PROFILE( "RenderLines" );
            Vector3 camRight = g_app->m_camera->GetRight() * 0.5;            
            glBegin( GL_QUADS );
                glVertex3dv( (trackLeft - camRight).GetData() );
                glVertex3dv( (trackLeft + camRight).GetData() );
                glVertex3dv( (cartLinkLeft + camRight).GetData() );
                glVertex3dv( (cartLinkLeft - camRight).GetData() );

                glVertex3dv( (trackRight - camRight).GetData() );
                glVertex3dv( (trackRight + camRight).GetData() );
                glVertex3dv( (cartLinkRight + camRight).GetData() );
                glVertex3dv( (cartLinkRight - camRight).GetData() );
            glEnd();
            //END_PROFILE( "RenderLines" );
        
            //START_PROFILE( "RenderPolygons" );
            for( int i = 0; i < 3; ++i )
            {            
                if( _cart->m_polygons[i] )
                {
                    Matrix34 polyMat = s_cartContents[i]->GetWorldMatrix( transform );            
                    s_polygon1->Render( 0.0, polyMat );
                }

                if( _cart->m_primitives[i] )
                {
                    Matrix34 polyMat = s_cartContents[i]->GetWorldMatrix( transform );            
                    s_primitive1->Render( 0.0, polyMat );

                    if( buildingDetail < 3 )
                    {
                        glEnable( GL_BLEND );
                        glBlendFunc( GL_SRC_ALPHA, GL_ONE );
                        glDepthMask( false );
                        //glDisable( GL_DEPTH_TEST );
                        glDisable( GL_LIGHTING );

                        glColor4f( 1.0, 0.7, 0.0, 0.75 );
                      
	                    double nearPlaneStart = g_app->m_renderer->GetNearPlane();
	                    g_app->m_camera->SetupProjectionMatrix(nearPlaneStart * 1.1,
							 			                       g_app->m_renderer->GetFarPlane());                    
                    
                        Render3DSprite( polyMat.pos - Vector3(0,25,0), 50.0, 50.0, g_app->m_resource->GetTexture( "textures/glow.bmp" ) );                                                           

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
            //END_PROFILE( "RenderPolygons" );
        }
    }

	//END_PROFILE( "Mine Cart");
}


bool MineBuilding::Advance()
{
    double mineSpeed = RefinerySpeed();
    if( m_previousMineSpeed <= 0.1 && mineSpeed > 0.1 )
    {
        g_app->m_soundSystem->TriggerBuildingEvent( this, "CogTurn" );
    }
    m_previousMineSpeed = mineSpeed;

    if( mineSpeed > 0.0 )
    {
        for( int i = m_carts.Size()-1; i >= 0; --i )
        {
            MineCart *thisCart = m_carts[i];
            thisCart->m_progress += mineSpeed * SERVER_ADVANCE_PERIOD;

            if( thisCart->m_progress >= 1.0 )
            {
                double remainder = thisCart->m_progress - 1.0;
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
    
    m_wheelRotate -= 3.0 * mineSpeed * SERVER_ADVANCE_PERIOD;

    return Building::Advance();
}


void MineBuilding::ListSoundEvents( LList<char *> *_list )
{
    Building::ListSoundEvents( _list );

    _list->PutData( "CogTurn" );
}


void MineBuilding::TriggerCart ( MineCart *_cart, double _initValue )
{
    _cart->m_progress = _initValue;
    m_carts.PutDataAtStart( _cart );
}


Vector3 MineBuilding::GetTrackMarker1()
{
    if( !m_trackMarker1 || g_app->m_editing )
    {
        const char trackMarker1Name[] = "MarkerTrack1";
        m_trackMarker1 = m_shape->m_rootFragment->LookupMarker( trackMarker1Name );
        AppReleaseAssert( m_trackMarker1, "MineBuilding: Can't get Marker(%s) from shape(%s), probably a corrupted file\n", trackMarker1Name, m_shape->m_name );
        Matrix34 rootMat( m_front, g_upVector, m_pos );
        m_trackMatrix1 = m_trackMarker1->GetWorldMatrix( rootMat );
    }
    
    return m_trackMatrix1.pos;
}


Vector3 MineBuilding::GetTrackMarker2()
{
    if( !m_trackMarker2 || g_app->m_editing )
    {
        const char trackMarker2Name[] = "MarkerTrack2";
        m_trackMarker2 = m_shape->m_rootFragment->LookupMarker( trackMarker2Name );
        AppReleaseAssert( m_trackMarker2, "MineBuilding: Can't get Marker(%s) from shape(%s), probably a corrupted file\n", trackMarker2Name, m_shape->m_name );
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


void MineBuilding::Write( TextWriter *_out )
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


double MineBuilding::RefinerySpeed()
{
    if( GetNetworkTime() >= s_refineryRecalculateTimer )
    {
        //
        // Find the refinery
        // If not a refinery, look for a construction yard
        // If not, look for a fuel generator

        Building *driver = NULL;
        
        int numFuelGenerators = 0;
        double fuelGeneratorFactor = 0.0;

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
            s_refineryPopulation = (double) numPortsOccupied / (double) numPorts;
        }
        else if( numFuelGenerators > 0 )
        {
            s_refineryPopulation = fuelGeneratorFactor / (double) numFuelGenerators;
        }
        else
        {
            int mineLocationId = g_app->m_globalWorld->GetLocationId("mine");
            s_refineryPopulation = 0.0;

            GlobalBuilding *globalRefinery = NULL;
            for( int i = 0; i < g_app->m_globalWorld->m_buildings.Size(); ++i )
            {
                if( g_app->m_globalWorld->m_buildings.ValidIndex(i) )
                {
                    GlobalBuilding *gb = g_app->m_globalWorld->m_buildings[i];
                    if( gb && gb->m_locationId == mineLocationId && 
                        gb->m_type == TypeRefinery && gb->m_online )
                    {
                        s_refineryPopulation = 1.0;
                        break;
                    }
                }
            }
        }

        s_refineryRecalculateTimer = GetNetworkTime() + 0.1;
    }

    double speed = s_refineryPopulation * iv_abs(iv_sin( g_gameTime * 2.0 )) * 0.5;   
    
#ifdef CHEATMENU_ENABLED
    if( !g_app->Multiplayer() )
    {
        if( g_inputManager->controlEvent( ControlScrollSpeedup ) )
        {
            speed *= 10.0;
        }
    }
#endif

    return speed;
}

// ****************************************************************************
// Class MineCart
// ****************************************************************************

MineCart::MineCart()
:   m_progress(0.0)
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


void TrackJunction::Render( double _predictionTime )
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
			    start.y += 10.0;
			    Vector3 end = linkBuilding->m_pos;
			    end.y += 10.0;
			    RenderArrow(start, end, 6.0, RGBAColour(255,0,255));
            }
        }
    }
#endif
}


void TrackJunction::TriggerCart( MineCart *_cart, double _initValue )
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


void TrackJunction::Write( TextWriter *_out )
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
        
            if( cart->m_progress > 0.5 )
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


void TrackStart::RenderAlphas( double _predictionTime )
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
                         2.0, RGBAColour(255,0,0) );
        }
    }
#endif
}


void TrackStart::Read( TextReader *_in, bool _dynamic )
{
    MineBuilding::Read( _in, _dynamic );

    m_reqBuildingId = atoi( _in->GetNextToken() );    
}


void TrackStart::Write( TextWriter *_out )
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
    if( building->m_type == TypeTrunkPort && ((TrunkPort *)building)->m_openTimer > 0.0 ) online = true;
    if( building->m_type == TypeYard ) online = true;
    if( building->m_type == TypeFuelGenerator ) online = true;

    if( online )
    {
        for( int i = 0; i < m_carts.Size(); ++i )
        {
            MineCart *cart = m_carts[i];    

            if( cart->m_progress > 0.5 )
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


void TrackEnd::RenderAlphas( double _predictionTime )
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
                         2.0, RGBAColour(255,0,0) );
        }
    }
#endif
}


void TrackEnd::Read( TextReader *_in, bool _dynamic )
{
    MineBuilding::Read( _in, _dynamic );

    m_reqBuildingId = atoi( _in->GetNextToken() );    
}


void TrackEnd::Write( TextWriter *_out )
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

    const char wheel1Name[] = "MarkerWheel01";
    m_wheel1 = m_shape->m_rootFragment->LookupMarker( wheel1Name );
    AppReleaseAssert( m_wheel1, "Refinery: Can't get Marker(%s) from shape(%s), probably a corrupted file\n", wheel1Name, m_shape->m_name );

    const char wheel2Name[] = "MarkerWheel02";
    m_wheel2 = m_shape->m_rootFragment->LookupMarker( wheel2Name);
    AppReleaseAssert( m_wheel2, "Refinery: Can't get Marker(%s) from shape(%s), probably a corrupted file\n", wheel2Name, m_shape->m_name );

    const char wheel3Name[] = "MarkerWheel03";
    m_wheel3 = m_shape->m_rootFragment->LookupMarker( wheel3Name );
    AppReleaseAssert( m_wheel3, "Refinery: Can't get Marker(%s) from shape(%s), probably a corrupted file\n", wheel3Name, m_shape->m_name );

    const char counter1Name[] = "MarkerCounter";
    m_counter1 = m_shape->m_rootFragment->LookupMarker( counter1Name);
    AppReleaseAssert( m_counter1, "Refinery: Can't get Marker(%s) from shape(%s), probably a corrupted file\n", counter1Name, m_shape->m_name );
}


void Refinery::GetObjectiveCounter( UnicodeString& _dest )
{
    GlobalBuilding *gb = g_app->m_globalWorld->GetBuilding( m_id.GetUniqueId(), g_app->m_locationId );
    int numRefined = 0;
    if( gb ) numRefined = gb->m_link;
    
    static wchar_t result[256];
	swprintf( result, sizeof(result)/sizeof(wchar_t),
			  L"%ls : %d", LANGUAGEPHRASE("objective_refined").m_unicodestring, numRefined );
    _dest = UnicodeString(result);
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


void Refinery::TriggerCart( MineCart *_cart, double _initValue )
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


void Refinery::Render( double _predictionTime )
{
    MineBuilding::Render( _predictionTime );
    
    //
    // Render wheels

    Matrix34 refineryMat( m_front, g_upVector, m_pos );
    Matrix34 wheel1Mat = m_wheel1->GetWorldMatrix( refineryMat );
    Matrix34 wheel2Mat = m_wheel2->GetWorldMatrix( refineryMat );
    Matrix34 wheel3Mat = m_wheel3->GetWorldMatrix( refineryMat );

    double refinerySpeed = RefinerySpeed();
    double predictedWheelRotate = m_wheelRotate - 3.0 * refinerySpeed * _predictionTime;

    wheel1Mat.RotateAroundF( predictedWheelRotate );
    wheel2Mat.RotateAroundF( predictedWheelRotate * -1.0 );
    wheel3Mat.RotateAroundF( predictedWheelRotate );

    wheel1Mat.r *= 1.3;
    wheel1Mat.u *= 1.3;
    wheel1Mat.f *= 1.3;

    wheel2Mat.r *= 0.8;
    wheel2Mat.u *= 0.8;
    
    wheel3Mat.r *= 1.6;
    wheel3Mat.u *= 1.6;
    wheel3Mat.f *= 1.6;
    
    s_wheelShape->Render( _predictionTime, wheel1Mat );
    s_wheelShape->Render( _predictionTime, wheel2Mat );
    s_wheelShape->Render( _predictionTime, wheel3Mat );

    GlobalBuilding *gb = g_app->m_globalWorld->GetBuilding( m_id.GetUniqueId(), g_app->m_locationId );
    int numRefined = 0;
    if( gb ) numRefined = gb->m_link;

    Matrix34 counterMat = m_counter1->GetWorldMatrix(refineryMat);
    glColor4f( 0.6, 0.8, 0.9, 1.0 );
    g_gameFont.DrawText3D( counterMat.pos, counterMat.f, counterMat.u, 10.0, "%d", numRefined );
    counterMat.pos += counterMat.f * 0.1;
    counterMat.pos += ( counterMat.f ^ counterMat.u ) * 0.2;
    counterMat.pos += counterMat.u * 0.2;    
    g_gameFont.SetRenderShadow(true);
    glColor4f( 0.6, 0.8, 0.9, 0.0 );
    g_gameFont.DrawText3D( counterMat.pos, counterMat.f, counterMat.u, 10.0, "%d", numRefined );
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

    const char wheel1Name[] = "MarkerWheel01";
    m_wheel1 = m_shape->m_rootFragment->LookupMarker( wheel1Name );
    AppReleaseAssert( m_wheel1, "Mine: Can't get Marker(%s) from shape(%s), probably a corrupted file\n", wheel1Name, m_shape->m_name );

    const char wheel2Name[] = "MarkerWheel02";
    m_wheel2 = m_shape->m_rootFragment->LookupMarker( wheel2Name );
    AppReleaseAssert( m_wheel2, "Mine: Can't get Marker(%s) from shape(%s), probably a corrupted file\n", wheel2Name, m_shape->m_name );
}


void Mine::Render( double _predictionTime )
{
    MineBuilding::Render( _predictionTime );

    //
    // Render wheels
    
    Matrix34 mineMat( m_front, g_upVector, m_pos );
    Matrix34 wheel1Mat = m_wheel1->GetWorldMatrix( mineMat );
    Matrix34 wheel2Mat = m_wheel2->GetWorldMatrix( mineMat );

    double refinerySpeed = RefinerySpeed();
    double predictedWheelRotate = m_wheelRotate - 3.0 * refinerySpeed * _predictionTime;

    wheel1Mat.RotateAroundF( predictedWheelRotate * -1.0 );
    wheel2Mat.RotateAroundF( predictedWheelRotate );

    wheel1Mat.r *= 0.5;
    wheel1Mat.u *= 0.5;
    
    wheel2Mat.r *= 0.9;
    wheel2Mat.u *= 0.9;
        
    s_wheelShape->Render( _predictionTime, wheel1Mat );
    s_wheelShape->Render( _predictionTime, wheel2Mat );
}


void Mine::TriggerCart( MineCart *_cart, double _initValue )
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
        double fractionOccupied = (double) GetNumPortsOccupied() / (double) GetNumPorts();

        if( syncfrand(1) <= fractionOccupied )
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
