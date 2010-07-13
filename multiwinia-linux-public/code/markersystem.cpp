#include "lib/universal_include.h"
#include "lib/resource.h"
#include "lib/profiler.h"
#include "lib/targetcursor.h"
#include "lib/debug_render.h"
#include "lib/shape.h"

#include "app.h"
#include "renderer.h"
#include "camera.h"
#include "markersystem.h"
#include "location.h"
#include "team.h"
#include "main.h"
#include "unit.h"
#include "global_world.h"
#include "multiwinia.h"
#include "gamecursor.h"
#include "taskmanager_interface_multiwinia.h"

#include "worldobject/crate.h"
#include "worldobject/statue.h"
#include "worldobject/researchitem.h"
#include "worldobject/armour.h"
#include "worldobject/officer.h"


MarkerSystem::MarkerSystem()
:   m_isOrderMarkerUnderMouse(false)
{
}


void MarkerSystem::FindScreenEdge( Vector2 const &_line, float *_posX, float *_posY )
{
    //    y = mx + c
    //    c = y - mx
    //    x = (y - c) / m

    int screenH = g_app->m_renderer->ScreenH();
    int screenW = g_app->m_renderer->ScreenW();

    float m = _line.y / _line.x;
    float c = ( screenH / 2.0f ) - m * ( screenW / 2.0f );

    if( _line.y < 0 )
    {
        // Intersect with top view plane
        float x = ( 0 - c ) / m;
        if( x >= 0 && x <= screenW )
        {
            *_posX = x;
            *_posY = 0;
            return;
        }
    }
    else
    {
        // Intersect with the bottom view plane
        float x = ( screenH - c ) / m;
        if( x >= 0 && x <= screenW )
        {
            *_posX = x;
            *_posY = screenH;
            return;
        }        
    }

    if( _line.x < 0 )
    {
        // Intersect with left view plane 
        float y = m * 0 + c;
        if( y >= 0 && y <= screenH ) 
        {
            *_posX = 0;
            *_posY = y;
            return;
        }
    }
    else
    {
        // Intersect with right view plane
        float y = m * screenW + c;
        if( y >= 0 && y <= screenH ) 
        {
            *_posX = screenW;
            *_posY = y;
            return;
        }        
    }

    // We should never ever get here
    //AppDebugAssert( false );
    *_posX = 0;
    *_posY = 0;
}

bool MarkerSystem::CalculateScreenPosition( Vector3 const &worldPos, float &screenX, float &screenY )
{
    int screenH = g_app->m_renderer->ScreenH();
    int screenW = g_app->m_renderer->ScreenW();

    //
    // Project worldTarget into screen co-ordinates
    // Is the _pos on screen or not?

    g_app->m_camera->Get2DScreenPos( worldPos, &screenX, &screenY );
    screenY = screenH - screenY;

    Vector3 toCam = g_app->m_camera->GetPos() - worldPos;
    float angle = toCam * g_app->m_camera->GetFront();
    Vector3 rotationVector = toCam ^ g_app->m_camera->GetFront();

    if( angle <= 0.0f &&
        screenX >= 0 && screenX < screenW &&
        screenY >= 0 && screenY < screenH )
    {
        // _pos is onscreen
        return true;
    }
    else
    {
        // _pos is offscreen
        Vector3 camPos = g_app->m_camera->GetPos() + g_app->m_camera->GetFront() * 1000;
        Vector3 camToTarget = ( worldPos - camPos ).SetLength( 100 );

        float camX = screenW / 2.0f;
        float camY = screenH / 2.0f;
        float posX, posY;
        g_app->m_camera->Get2DScreenPos( camPos + camToTarget, &posX, &posY );

        Vector2 lineNormal( posX - camX, posY - camY );
        lineNormal.Normalise();

        FindScreenEdge( lineNormal, &screenX, &screenY );

        return false;        
    }
}


void MarkerSystem::CalculateScreenBox( float screenX, float screenY, float screenDX, float screenDY, float size,
                                       float &minX, float &minY, float &maxX, float &maxY )
{
    Vector2 pos( screenX, screenY );
    Vector2 gradient( screenDX, screenDY );
    if( gradient.MagSquared() > 0 )
    {
        gradient.Normalise();
        pos += gradient * 50;
    }

    gradient.Set(0,1);
    Vector2 rightAngle = gradient;
    float tempX = rightAngle.x;
    rightAngle.x = rightAngle.y;
    rightAngle.y = tempX * -1;

    Vector2 tl = pos - rightAngle * size / 2.0f - gradient * size/2.0f;
    Vector2 bl = pos - rightAngle * size / 2.0f + gradient * size/2.0f;
    Vector2 br = pos + rightAngle * size / 2.0f + gradient * size/2.0f;
    Vector2 tr = pos + rightAngle * size / 2.0f - gradient * size/2.0f;

    minX = min( tl.x, min( bl.x, min( br.x, tr.x ) ) );
    minY = min( tl.y, min( bl.y, min( br.y, tr.y ) ) );
    maxX = max( tl.x, max( bl.x, max( br.x, tr.x ) ) );
    maxY = max( tl.y, max( bl.y, max( br.y, tr.y ) ) );
}


bool MarkerSystem::IsMOuseInside( float _screenX, float _screenY, float _screenDX, float _screenDY, float _size )
{
    float minX, minY, maxX, maxY;
    CalculateScreenBox( _screenX, _screenY, _screenDX, _screenDY, _size, minX, minY, maxX, maxY );

    int screenX = g_target->X();
    int screenY = g_target->Y();

    return( screenX >= minX && screenX <= maxX &&
            screenY >= minY && screenY <= maxY );
}


void MarkerSystem::RenderShadow( float _screenX, float _screenY, float _screenDX, float _screenDY, float _size, float _alpha )
{
    Vector2 pos( _screenX, _screenY );
    Vector2 gradient( _screenDX, _screenDY );
    if( gradient.MagSquared() > 0 )
    {
        gradient.Normalise();
        pos += gradient * 50;
    }
    else
    {
        gradient.Set(0,1);
    }
    Vector2 rightAngle = gradient;
    float tempX = rightAngle.x;
    rightAngle.x = rightAngle.y;
    rightAngle.y = tempX * -1;

    glEnable        ( GL_BLEND );
    glDisable       ( GL_CULL_FACE );
    glDepthMask     ( false );
    glColor4f       ( 0.0f, 0.0f, 0.0f, _alpha );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTextureWithAlpha( "icons/marker_shadow.bmp" ) );

    glBegin( GL_QUADS );
        glTexCoord2i( 0, 1 );       glVertex2dv( (pos - rightAngle * _size / 2.0f - gradient * _size/2.0f).GetData() );
        glTexCoord2i( 0, 0 );       glVertex2dv( (pos - rightAngle * _size / 2.0f + gradient * _size/2.0f).GetData() );
        glTexCoord2i( 1, 0 );       glVertex2dv( (pos + rightAngle * _size / 2.0f + gradient * _size/2.0f).GetData() );
        glTexCoord2i( 1, 1 );       glVertex2dv( (pos + rightAngle * _size / 2.0f - gradient * _size/2.0f).GetData() );
    glEnd();

    glDepthMask     ( true );
    glDisable       ( GL_TEXTURE_2D );
    glEnable        ( GL_CULL_FACE );
}


void MarkerSystem::RenderCompass( float _screenX, float _screenY, float _screenDX, float _screenDY, float _size, float _alpha )
{
    Vector2 pos( _screenX, _screenY );
    Vector2 gradient( _screenDX, _screenDY );
    if( gradient.MagSquared() > 0 )
    {
        gradient.Normalise();
        pos += gradient * 50;
    }
    else
    {
        gradient.Set(0,1);
    }
   Vector2 rightAngle = gradient;
    float tempX = rightAngle.x;
    rightAngle.x = rightAngle.y;
    rightAngle.y = tempX * -1;

    glColor4f       ( 0.0f, 0.0f, 0.0f, _alpha );

    glEnable        ( GL_BLEND );
    glDisable       ( GL_CULL_FACE );
    glDepthMask     ( false );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTextureWithAlpha( "icons/compass_shadow.bmp" ) );

    glBegin( GL_QUADS );
        glTexCoord2i( 0, 1 );       glVertex2dv( (pos - rightAngle * _size / 2.0f - gradient * _size/2.0f).GetData() );
        glTexCoord2i( 0, 0 );       glVertex2dv( (pos - rightAngle * _size / 2.0f + gradient * _size/2.0f).GetData() );
        glTexCoord2i( 1, 0 );       glVertex2dv( (pos + rightAngle * _size / 2.0f + gradient * _size/2.0f).GetData() );
        glTexCoord2i( 1, 1 );       glVertex2dv( (pos + rightAngle * _size / 2.0f - gradient * _size/2.0f).GetData() );
    glEnd();

    glColor4f       ( 1.0f, 1.0f, 1.0f, _alpha );

    if( _screenDX == 0.0f && _screenDY == 0.0f )
    {
        glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "icons/compass_empty.bmp" ) );
    }
    else
    {
        glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "icons/compass.bmp" ) );
        glBlendFunc		( GL_SRC_ALPHA, GL_ONE );
    }

    glEnable        ( GL_TEXTURE_2D );

    glBegin( GL_QUADS );
        glTexCoord2i( 0, 1 );       glVertex2dv( (pos - rightAngle * _size / 2.0f - gradient * _size/2.0f).GetData() );
        glTexCoord2i( 0, 0 );       glVertex2dv( (pos - rightAngle * _size / 2.0f + gradient * _size/2.0f).GetData() );
        glTexCoord2i( 1, 0 );       glVertex2dv( (pos + rightAngle * _size / 2.0f + gradient * _size/2.0f).GetData() );
        glTexCoord2i( 1, 1 );       glVertex2dv( (pos + rightAngle * _size / 2.0f - gradient * _size/2.0f).GetData() );
    glEnd();

    glDepthMask     ( true );
    glDisable       ( GL_TEXTURE_2D );
    glEnable        ( GL_CULL_FACE );
}


void MarkerSystem::RenderCompassEmpty( float _screenX, float _screenY, float _screenDX, float _screenDY, float _size, float _alpha )
{
    Vector2 pos( _screenX, _screenY );
    Vector2 gradient( _screenDX, _screenDY );
    if( gradient.MagSquared() > 0 )
    {
        gradient.Normalise();
        pos += gradient * 50;
    }
    else
    {
        gradient.Set(0,1);
    }
   Vector2 rightAngle = gradient;
    float tempX = rightAngle.x;
    rightAngle.x = rightAngle.y;
    rightAngle.y = tempX * -1;

    glColor4f       ( 0.0f, 0.0f, 0.0f, _alpha );

    glEnable        ( GL_BLEND );
    glDisable       ( GL_CULL_FACE );
    glDepthMask     ( false );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTextureWithAlpha( "icons/compass_shadow.bmp" ) );

    glBegin( GL_QUADS );
    glTexCoord2i( 0, 1 );       glVertex2dv( (pos - rightAngle * _size / 2.0f - gradient * _size/2.0f).GetData() );
    glTexCoord2i( 0, 0 );       glVertex2dv( (pos - rightAngle * _size / 2.0f + gradient * _size/2.0f).GetData() );
    glTexCoord2i( 1, 0 );       glVertex2dv( (pos + rightAngle * _size / 2.0f + gradient * _size/2.0f).GetData() );
    glTexCoord2i( 1, 1 );       glVertex2dv( (pos + rightAngle * _size / 2.0f - gradient * _size/2.0f).GetData() );
    glEnd();

    glColor4f       ( 1.0f, 1.0f, 1.0f, _alpha );

    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "icons/compass_empty.bmp" ) );
    glEnable        ( GL_TEXTURE_2D );

    glBegin( GL_QUADS );
        glTexCoord2i( 0, 1 );       glVertex2dv( (pos - rightAngle * _size / 2.0f - gradient * _size/2.0f).GetData() );
        glTexCoord2i( 0, 0 );       glVertex2dv( (pos - rightAngle * _size / 2.0f + gradient * _size/2.0f).GetData() );
        glTexCoord2i( 1, 0 );       glVertex2dv( (pos + rightAngle * _size / 2.0f + gradient * _size/2.0f).GetData() );
        glTexCoord2i( 1, 1 );       glVertex2dv( (pos + rightAngle * _size / 2.0f - gradient * _size/2.0f).GetData() );
    glEnd();

    glDepthMask     ( true );
    glDisable       ( GL_TEXTURE_2D );
    glEnable        ( GL_CULL_FACE );
}


void MarkerSystem::RenderIcon( float _screenX, float _screenY, float _screenDX, float _screenDY, float _size, char *iconFilename, RGBAColour colour, float rotation )
{
    Vector2 pos( _screenX, _screenY );
    Vector2 gradient( _screenDX, _screenDY );
    if( gradient.MagSquared() > 0 )
    {
        gradient.Normalise();
        pos += gradient * 50;
    }
    gradient.Set(0,1);
    Vector2 rightAngle = gradient;
    float tempX = rightAngle.x;
    rightAngle.x = rightAngle.y;
    rightAngle.y = tempX * -1;

    glColor4ubv( colour.GetData() );

    glEnable        ( GL_BLEND );
    glDisable       ( GL_CULL_FACE );
    glDepthMask     ( false );
    glBlendFunc		( GL_SRC_ALPHA, GL_ONE );

    glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( iconFilename ) );

    Vector3 texPos(0.5f,0,-0.5f);
    Vector3 texOff(0.5f,0,0.5f);

    texPos.RotateAroundY( rotation );

    glBegin( GL_QUADS );
        glTexCoord2f( texOff.x+texPos.x, texOff.z+texPos.z );       glVertex2dv( (pos + rightAngle * _size / 2.0f + gradient * _size/2.0f).GetData() );     texPos.RotateAroundY(-0.5f * M_PI);
        glTexCoord2f( texOff.x+texPos.x, texOff.z+texPos.z );       glVertex2dv( (pos + rightAngle * _size / 2.0f - gradient * _size/2.0f).GetData() );     texPos.RotateAroundY(-0.5f * M_PI);
        glTexCoord2f( texOff.x+texPos.x, texOff.z+texPos.z );       glVertex2dv( (pos - rightAngle * _size / 2.0f - gradient * _size/2.0f).GetData() );     texPos.RotateAroundY(-0.5f * M_PI);
        glTexCoord2f( texOff.x+texPos.x, texOff.z+texPos.z );       glVertex2dv( (pos - rightAngle * _size / 2.0f + gradient * _size/2.0f).GetData() );     texPos.RotateAroundY(-0.5f * M_PI);
    glEnd();

    glDepthMask     ( true );
    glDisable       ( GL_TEXTURE_2D );
    glEnable        ( GL_CULL_FACE );
}


void MarkerSystem::RenderColourCircle( float screenX, float screenY, float screenDX, float screenDY, float size, RGBAColour colour, float percent )
{
    Vector2 pos( screenX, screenY );
    Vector2 gradient( screenDX, screenDY );
    if( gradient.MagSquared() > 0 )
    {
        gradient.Normalise();
        pos += gradient * 50;
    }
    gradient.Set(0,1);
    Vector2 rightAngle = gradient;
    float tempX = rightAngle.x;
    rightAngle.x = rightAngle.y;
    rightAngle.y = tempX * -1;

    glColor4ubv( colour.GetData() );

    glEnable        ( GL_BLEND );
    glDisable       ( GL_CULL_FACE );
    glDepthMask     ( false );
    //glBlendFunc		( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );

    glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTextureWithAlpha( "icons/marker_colour.bmp" ) );

    float totalHeight = percent * size;

    glBegin( GL_QUADS );
        glTexCoord2f( 0, 1 );               glVertex2dv( (pos - rightAngle * size / 2.0f + gradient * size/2.0f).GetData() );
        glTexCoord2f( 0, 1-percent );       glVertex2dv( (pos - rightAngle * size / 2.0f + gradient * size/2.0f - gradient * totalHeight).GetData() );
        glTexCoord2f( 1, 1-percent );       glVertex2dv( (pos + rightAngle * size / 2.0f + gradient * size/2.0f - gradient * totalHeight).GetData() );
        glTexCoord2f( 1, 1 );               glVertex2dv( (pos + rightAngle * size / 2.0f + gradient * size/2.0f).GetData() );
    glEnd();

    glDepthMask     ( true );
    glDisable       ( GL_TEXTURE_2D );
    glEnable        ( GL_CULL_FACE );
}


void MarkerSystem::RegisterMarker_Crate( WorldObjectId id )
{
    LocationMarker *marker = new LocationMarker();
    marker->m_type = LocationMarker::TypeAttachedToCrate;
    marker->m_objId = id;
    m_markers.PutData( marker );
}

void MarkerSystem::RegisterMarker_Research( WorldObjectId id )
{
    LocationMarker *marker = new LocationMarker();
    marker->m_type = LocationMarker::TypeAttachedToResearch;
    marker->m_objId = id;
    m_markers.PutData( marker );
}


void MarkerSystem::RegisterMarker_Statue( WorldObjectId id )
{
    LocationMarker *marker = new LocationMarker();
    marker->m_type = LocationMarker::TypeAttachedToStatue;
    marker->m_objId = id;
    m_markers.PutData( marker );
}


void MarkerSystem::RegisterMarker_Task( WorldObjectId id, bool fromCrate )
{
    LocationMarker *marker = new LocationMarker();
    marker->m_type = LocationMarker::TypeAttachedToTask;
    marker->m_teamId = id.GetTeamId();
    marker->m_objId = id;;
    marker->m_fromCrate = true;
    m_markers.PutData( marker );
}


void MarkerSystem::RegisterMarker_Fixed( int teamId, Vector3 const &pos, char *icon, bool fromCrate )
{
    LocationMarker *marker = new LocationMarker();
    marker->m_type = LocationMarker::TypeFixed;
    marker->m_teamId = teamId;
    marker->m_pos = pos;
    marker->m_pos += g_upVector * 30;
    marker->m_fromCrate = fromCrate;
    strcpy( marker->m_icon, icon );
    m_markers.PutData( marker );
}


void MarkerSystem::RegisterMarker_Orders  ( WorldObjectId id )
{
    LocationMarker *marker = new LocationMarker();
    marker->m_type = LocationMarker::TypeAttachedToOrders;
    marker->m_teamId = id.GetTeamId();
    marker->m_objId = id;
    marker->m_fromCrate = false;
    m_markers.PutData( marker );

    Entity *entity = g_app->m_location->GetEntity( id );
    if( entity && entity->m_type == Entity::TypeArmour )
    {
        marker->m_pos = ((Armour *)entity)->m_wayPoint;
    }
}


void MarkerSystem::RegisterMarker_Officer( WorldObjectId id )
{
    LocationMarker *marker = new LocationMarker();
    marker->m_type = LocationMarker::TypeAttachedToOfficer;
    marker->m_teamId = id.GetTeamId();
    marker->m_objId = id ;
    m_markers.PutData( marker );
}


void MarkerSystem::SortMarkers()
{
    //
    // Sort by distance to camera
    // Put the nearest at the end of the list
    // Note : this only does a single pass per frame


    for( int i = 0; i < m_markers.Size()-1; ++i )
    {
        LocationMarker *thisMarker = m_markers[i];
        LocationMarker *nextMarker = m_markers[i+1];

        float thisDist = ( thisMarker->m_pos - g_app->m_camera->GetPos() ).MagSquared();
        float nextDist = ( nextMarker->m_pos - g_app->m_camera->GetPos() ).MagSquared();

        // Bring closer if this marker is for our team

        if( thisMarker->m_teamId == g_app->m_globalWorld->m_myTeamId ) thisDist -= 50.0f;
        if( nextMarker->m_teamId == g_app->m_globalWorld->m_myTeamId ) nextDist -= 50.0f;

        // Bring to the top if the marker is very new

        float thisAge = GetHighResTime() - thisMarker->m_birthTime;
        float nextAge = GetHighResTime() - nextMarker->m_birthTime;

        if( thisAge < 3.0f ) thisDist = thisAge;
        if( nextAge < 3.0f ) nextDist = nextAge;

        // Bring to the top if the marker is highlighted

        if( thisMarker->m_objId == m_markerUnderMouse && thisMarker->m_type != LocationMarker::TypeAttachedToOfficer ) thisDist = 0.0f;
        if( nextMarker->m_objId == m_markerUnderMouse && nextMarker->m_type != LocationMarker::TypeAttachedToOfficer ) nextDist = 0.0f;
        
        // Move further away if we're attached to orders (so we always go behind the unit itself)

        if( thisMarker->m_type == LocationMarker::TypeAttachedToOrders ) thisDist += 50;
        if( nextMarker->m_type == LocationMarker::TypeAttachedToOrders ) nextDist += 50;

        if( thisDist < nextDist )
        {
            m_markers.RemoveData(i+1);
            m_markers.PutDataAtIndex( nextMarker, i );
        }
    }
}


void MarkerSystem::Advance()
{    
    if( !g_app->m_location ) return;

    SortMarkers();

    for( int i = 0; i < m_markers.Size(); ++i )
    {
        LocationMarker *marker = m_markers[i];

        if( !marker->m_active )
        {
            m_markers.RemoveData(i);
            delete marker;
            --i;
        }
        else
        {
            Vector3 newPos = marker->m_pos;

            switch( marker->m_type )
			{
                case LocationMarker::TypeAttachedToCrate:
                {
                    Building *building = g_app->m_location->GetBuilding( marker->m_objId.GetUniqueId() );
					if( !building || building->m_type != Building::TypeCrate )
                    {
                        marker->m_active = false;
                    }
                    else
                    {
                        newPos = building->m_pos + g_upVector * 20;
                    }
                    break;
                }

				case LocationMarker::TypeAttachedToResearch:
                {
                    Building *building = g_app->m_location->GetBuilding( marker->m_objId.GetUniqueId() );
					if( !building || building->m_type != Building::TypeResearchItem )
                    {
                        marker->m_active = false;
                    }
                    else
                    {
                        newPos = building->m_pos + g_upVector * 20;
                    }
                    break;
                }

                case LocationMarker::TypeAttachedToStatue:
                {
                    Building *building = g_app->m_location->GetBuilding( marker->m_objId.GetUniqueId() );
                    if( !building || building->m_type != Building::TypeStatue )
                    {
                        marker->m_active = false;
                    }
                    else
                    {
                        newPos = building->m_pos;
                        newPos += building->m_up * building->m_radius * 4.0f;
                    }

                    break;
                }

                case LocationMarker::TypeAttachedToTask:
                {
                    if( marker->m_teamId != -1 )
                    {
                        Team *team = g_app->m_location->m_teams[ marker->m_teamId ];
                        if( team && team->m_taskManager)
                        {
                            Task *task = team->m_taskManager->GetTask( marker->m_objId );
                            if( task )
                            {
                                newPos = task->GetPosition();
                                newPos += g_upVector * 20;
                            }
                            else
                            {
                                marker->m_active = false;
                            }
                        }
                    }
                    break;
                }

                case LocationMarker::TypeAttachedToOfficer:
                {
                    Entity *entity = g_app->m_location->GetEntity( marker->m_objId );
                    if( entity )
                    {
                        newPos = entity->m_pos + g_upVector * 20;
                    }
                    else
                    {
                        marker->m_active = false;
                    }
                    break;
                }

                case LocationMarker::TypeAttachedToOrders:
                {
                    marker->m_hasBeenSeen = true; 

                    Entity *entity = g_app->m_location->GetEntity( marker->m_objId );
                    if( entity )
                    {
                        if( entity->m_type == Entity::TypeArmour )
                        {
                            Armour *armour = (Armour *)entity;
                            Vector3 targetPos = armour->m_wayPoint;
                            float distance = ( targetPos - marker->m_pos ).MagSquared();
                            if( distance > 0.01f ) marker->m_active = false;

                            distance = ( armour->m_pos - armour->m_wayPoint ).MagSquared();
                            if( distance < 15.0f * 15.0f ) marker->m_active = false;
                        }
                    }
                    else
                    {
                        marker->m_active = false;
                    }
                    break;
                }

                case LocationMarker::TypeFixed:
                    if( marker->m_hasBeenSeen )
                    {
                        double age = marker->m_onscreenTimer;
                        if( age >= 8.0f )
                        {
                            marker->m_active = false;
                        }
                    }
                    break;
            }


            //
            // Smooth out the movement of the marker slightly

            if( marker->m_pos == g_zeroVector ) 
            {
                marker->m_pos = newPos;
            }
            else
            {
                float factor1 = g_advanceTime * 5.0f;
                float factor2 = 1.0f - factor1;
                marker->m_pos = ( marker->m_pos * factor2 ) + ( newPos * factor1 );
            }
        }
    }
}


void MarkerSystem::Render()
{    
    if( !g_app->m_location ) return;
    if( g_app->m_editing ) return;
    if( g_app->m_hideInterface ) return;

    START_PROFILE("RenderMarkerSystem");

    glDisable   ( GL_CULL_FACE );
    glBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glEnable    ( GL_BLEND );

    m_isOrderMarkerUnderMouse = false;

    g_app->m_renderer->SetupMatricesFor3D();

    for( int i = 0; i < m_markers.Size(); ++i )
    {
        LocationMarker *marker = m_markers[i];

        //
        // Calculate where the marker is on screen

        float alpha = 1.0f;
        float screenX, screenY;
        bool onScreen = CalculateScreenPosition( marker->m_pos, screenX, screenY );
        float camDist = (marker->m_pos - g_app->m_camera->m_pos).Mag();

        Vector2 lineNormal(0,-1);

        if( !onScreen )
        {
            screenY = g_app->m_renderer->ScreenH() - screenY;
            lineNormal.Set( g_app->m_renderer->ScreenW()/2.0f - screenX,
                            g_app->m_renderer->ScreenH()/2.0f - screenY );
            lineNormal.Normalise();                                
        }

        if( onScreen )
        {
            if( marker->m_onscreenTimer < 0.0f ) marker->m_onscreenTimer = 0.0f;
            marker->m_onscreenTimer += g_advanceTime;

            //
            // Move the marker up a bit

            screenY -= 5 * g_app->m_renderer->ScreenH()/camDist;

            //
            // Lower the alpha based on how long we've been on screen

            double timeOnScreen = marker->m_onscreenTimer;

            alpha = 1.0f;

            if( timeOnScreen >= 1.0f )
            {
                marker->m_hasBeenSeen = true;  
            }

            if( marker->m_hasBeenSeen &&
                marker->m_onscreenTimer > 6.0f )
            {
                float fadeOut = (marker->m_onscreenTimer-6.0f) / 9.0f;
                if( fadeOut > 1.0f ) fadeOut = 1.0f;
                alpha *= (1.0f - fadeOut);
            }


            //
            // Raise the alpha if we're a long way away

            float minAlpha = 0.0f;

            if( camDist > 350 ) 
            {
                minAlpha = (camDist - 350) / 500.0f;                       
            }


            //
            // Raise the alpha if we're near the centre of the screen
                
            float toScreenCentre = sqrtf( pow(screenX - g_app->m_renderer->ScreenW()/2.0f, 2 ) +
                                          pow(screenY - g_app->m_renderer->ScreenH()/2.0f, 2 ) );

            toScreenCentre /= ((float)g_app->m_renderer->ScreenH()*0.5f);
            if( toScreenCentre < 1.0f )
            {
                minAlpha = max( minAlpha, 1.0f - toScreenCentre );
            }


            if( alpha < minAlpha ) alpha = minAlpha;
            if( alpha > 1.0f ) alpha = 1.0f;
        }
        else
        {
            if( marker->m_hasBeenSeen )
            {
                marker->m_onscreenTimer += g_advanceTime;
            }
            
            if( !marker->ShowOffscreen() ) alpha = 0.0f;           
        }


        //
        // If we're not persistant and the user has seen us on screen,
        // fade out after 3 seconds

        if( !marker->IsPersistant() && 
            marker->m_hasBeenSeen &&
            marker->m_onscreenTimer > 6.0f )
        {
            float fadeOut = (marker->m_onscreenTimer-6.0f) / 2.0f;
            if( fadeOut > 1.0f ) fadeOut = 1.0f;
            alpha *= (1.0f - fadeOut);
        }


        if( marker->AlwaysAtFullAlpha() )
        {
            if( onScreen || marker->ShowOffscreen() )
            {
                alpha = max( alpha, 0.75f );
            }
        }


        //
        // Render the shadow drop and the compass effect

        g_app->m_renderer->SetupMatricesFor2D();

        float iconSize = g_app->m_renderer->ScreenH() / 20.0f;     
        float shadowSize = iconSize + 10;
        float compassSize = iconSize + g_app->m_renderer->ScreenH() / 30.0f;


        //
        // Make it flash if it demands attention

        if( marker->ShouldFlashAlert() )
        {
            double age = GetHighResTime() - marker->m_birthTime;
            if( age <= 3.0f )
            {
                if( fmodf( age, 0.4f ) < 0.2f )
                {
                    alpha = 0.2f;        
                }
                else
                {
                    alpha = 1.0f;
                    iconSize *= 1.3f;
                    shadowSize *= 1.3f;
                    compassSize *= 1.3f;
                }
            }
        }


        //
        // Make us bigger if we're really near the camera

        if( camDist < 500 )
        {
            float scaleFactor = 1 + (1.0f - camDist/500.0f);
            iconSize *= scaleFactor;
            shadowSize *= scaleFactor;
            compassSize *= scaleFactor;
        }


        //
        // Handle mouse highlight/selection

        if( marker->IsSelectable() )
        {           
            bool teamSelectedThisMarker = g_app->m_location->GetMyTeam() &&
                                          g_app->m_location->GetMyTeam()->GetMyEntity() &&
                                          g_app->m_location->GetMyTeam()->GetMyEntity()->m_id == marker->m_objId;

            if( m_markerUnderMouse == marker->m_objId || 
                g_app->m_gameCursor->m_objUnderMouse == marker->m_objId ||
                teamSelectedThisMarker )
            {
                alpha = 1.0f;
                iconSize *= 1.2f;
                shadowSize *= 1.2f;
                compassSize *= 1.2f;
            }

            if( marker->m_type != LocationMarker::TypeAttachedToOfficer &&
                IsMOuseInside( screenX, screenY, lineNormal.x, lineNormal.y, iconSize ) )
            {            
                m_newMarkerUnderMouse = marker->m_objId;            

                if( marker->m_type == LocationMarker::TypeAttachedToOrders )
                {
                    m_isOrderMarkerUnderMouse = true;
                }
            }
        }


        //
        // Render icon based on marker type
               
        switch( marker->m_type )
        {
            case LocationMarker::TypeAttachedToCrate:
            {
                Crate *crate = (Crate *)g_app->m_location->GetBuilding(marker->m_objId.GetUniqueId());
                if( crate && crate->m_type == Building::TypeCrate )
                {
                    RGBAColour colour(155,155,155,255);               
                    float percent = 1.0f;

                    if( crate->m_id.GetTeamId() != 255 )
                    {
                        colour = g_app->m_location->m_teams[crate->m_id.GetTeamId()]->m_colour;
                        percent = crate->GetPercentCaptured();
                        alpha = 1.0f;
                    }

                    colour.a *= alpha;

                    crate->m_percentCapturedSmooth = ( crate->m_percentCapturedSmooth * 0.95f ) + crate->GetPercentCaptured() * 0.05f;
                    float rotation = crate->m_percentCapturedSmooth * M_PI * -10.0f;

                    RenderShadow        ( screenX, screenY, lineNormal.x, lineNormal.y, shadowSize, 0.9f * alpha );
                    RenderCompass       ( screenX, screenY, lineNormal.x, lineNormal.y, compassSize, 1.0f * alpha );
                    RenderColourCircle  ( screenX, screenY, lineNormal.x, lineNormal.y, iconSize, colour, percent );
                    RenderIcon          ( screenX, screenY, lineNormal.x, lineNormal.y, iconSize, "icons/icon_crate.bmp", RGBAColour(255,255,255,255*alpha), rotation );                    
                }
                break;
            }

			case LocationMarker::TypeAttachedToResearch:
            {
				ResearchItem *research = (ResearchItem *)g_app->m_location->GetBuilding(marker->m_objId.GetUniqueId());
                if( research && research->m_type == Building::TypeResearchItem )
                {
                    RGBAColour colour(155,155,155,255);               
                    float percent = 1.0f;

                    if( research->m_id.GetTeamId() != 255 )
                    {
                        //colour = g_app->m_location->m_teams[research->m_id.GetTeamId()]->m_colour;
						percent = (100.0f - research->GetReprogrammed())/100;//crate->GetPercentCaptured();
                        alpha = 1.0f;
                    }

                    colour.a *= alpha;

                    research->m_percentResearchedSmooth = ( research->m_percentResearchedSmooth * 0.95f ) + percent * 0.05f;
                    float rotation = research->m_percentResearchedSmooth * M_PI * -10.0f;

                    RenderShadow        ( screenX, screenY, lineNormal.x, lineNormal.y, shadowSize, 0.9f * alpha );
                    RenderCompass       ( screenX, screenY, lineNormal.x, lineNormal.y, compassSize, 1.0f * alpha );
                    RenderColourCircle  ( screenX, screenY, lineNormal.x, lineNormal.y, iconSize, colour, percent );
                    RenderIcon          ( screenX, screenY, lineNormal.x, lineNormal.y, iconSize, "icons/icon_crate.bmp", RGBAColour(255,255,255,255*alpha), rotation );                    
                }
                break;
            }

            case LocationMarker::TypeAttachedToStatue:
            {
                Statue *statue = (Statue *)g_app->m_location->GetBuilding(marker->m_objId.GetUniqueId());
                if( statue && statue->m_type == Building::TypeStatue )
                {
                    RGBAColour colour(155,155,155,255);               
                    float percent = 1.0f;

                    if( statue->m_id.GetTeamId() != 255 )
                    {
                        colour = g_app->m_location->m_teams[statue->m_id.GetTeamId()]->m_colour;
                        percent = (statue->m_numLifters - statue->m_minLifters) /
                                  float(statue->m_maxLifters - statue->m_minLifters);
                        alpha = 1.0f;
                    }

                    colour.a *= alpha;
                    colour.a *= 0.5f;

                    RenderShadow        ( screenX, screenY, lineNormal.x, lineNormal.y, shadowSize, 0.9f * alpha );
                    RenderCompass       ( screenX, screenY, lineNormal.x, lineNormal.y, compassSize, 1.0f * alpha );
                    RenderColourCircle  ( screenX, screenY, lineNormal.x, lineNormal.y, iconSize, colour, percent );
                    RenderIcon          ( screenX, screenY, lineNormal.x, lineNormal.y, iconSize, "icons/icon_statue.bmp", RGBAColour(255,255,255,255*alpha) );
                }
                break;
            }

            case LocationMarker::TypeAttachedToTask:
            {
                if( marker->m_teamId != -1 )
                {
                    Team *team = g_app->m_location->m_teams[ marker->m_teamId ];
                    Task *task = team->m_taskManager->GetTask( marker->m_objId );
                    if( task )
                    {                        
                        RGBAColour colour = team->m_colour;
                        colour.a *= alpha;

                        char bmpFilename[256];
                        sprintf( bmpFilename, "icons/icon_%s.bmp", Task::GetTaskName(task->m_type) );

                        if( marker->m_fromCrate )
                        {
                            float timePassed = GetHighResTime() - marker->m_birthTime;
                            if( timePassed < 5 ) marker->m_rotation += pow( 5 - timePassed, 2 );
                        }

                        RenderShadow        ( screenX, screenY, lineNormal.x, lineNormal.y, shadowSize, 0.9f * alpha );
                        RenderCompass       ( screenX, screenY, lineNormal.x, lineNormal.y, compassSize, 1.0f * alpha );
                        RenderColourCircle  ( screenX, screenY, lineNormal.x, lineNormal.y, iconSize, colour );
                        //RenderIcon          ( screenX, screenY, lineNormal.x, lineNormal.y, iconSize, "icons/icon_crate.bmp", RGBAColour(255,255,255,50*alpha), marker->m_rotation * -0.03f );
                        RenderIcon          ( screenX, screenY, lineNormal.x, lineNormal.y, iconSize, bmpFilename, RGBAColour(255,255,255,255*alpha) );                        

                        if( g_app->m_location->IsFriend( task->m_teamId, g_app->m_globalWorld->m_myTeamId ) )
                        {
                            float minX, minY, maxX, maxY;
                            CalculateScreenBox( screenX, screenY, lineNormal.x, lineNormal.y, iconSize, minX, minY, maxX, maxY );

                            TaskManagerInterfaceMultiwinia *tmm = (TaskManagerInterfaceMultiwinia *)g_app->m_taskManagerInterface;
                            tmm->RenderExtraTaskInfo( task, Vector2( (maxX+minX)/2.0f, (maxY+minY)/2.0f ), (maxX-minX), alpha );


                            //
                            // If we're an Armour unit and the user is trying to move Darwinians inside
                            // Change our icon to show this

                            /*if( m_markerUnderMouse == task->m_objId )
                            {
                                Armour *armour = (Armour *)g_app->m_location->GetEntitySafe( task->m_objId, Entity::TypeArmour );
                                if( armour && 
                                    team->m_numDarwiniansSelected > 0 &&
                                    fmodf( GetHighResTime(), 0.5f ) < 0.25f )
                                {
                                    RenderShadow        ( screenX, screenY, lineNormal.x, lineNormal.y, shadowSize, 0.5f * alpha );
                                    RenderIcon          ( screenX, screenY, lineNormal.x, lineNormal.y, iconSize, "icons/icon_enter.bmp", RGBAColour(255,255,255,255*alpha) );                        
                                }
                            }*/
                        }
                    }
                    else
                    {
                        marker->m_active = false;
                    }
                }
                break;
            }


            case LocationMarker::TypeAttachedToOrders:
            {
                if( marker->m_teamId != -1 )
                {
                    Team *team = g_app->m_location->m_teams[ marker->m_teamId ];
                    Task *task = team->m_taskManager->GetTask( marker->m_objId );
                    if( task )
                    {                        
                        Armour *armour = (Armour *) g_app->m_location->GetEntitySafe( task->m_objId, Entity::TypeArmour );

                        RGBAColour colour = team->m_colour;
                        colour.a *= alpha;

                        char bmpFilename[256];
                        sprintf( bmpFilename, "icons/icon_%s.bmp", Task::GetTaskName(task->m_type) );

                        iconSize *= 0.7f;
                        shadowSize *= 0.7f;
                        compassSize *= 0.7f;

                        g_app->m_renderer->SetupMatricesFor3D();
                        float highlightSize = 10 + sinf(GetHighResTime()*5) * 1;
                        g_app->m_gameCursor->RenderUnitHighlightEffect( marker->m_pos, highlightSize, team->m_colour );                        

                        Vector3 rayStart, rayDir;
                        g_app->m_camera->GetClickRay( g_target->X(), g_target->Y(), &rayStart, &rayDir );
                        if( marker->IsSelectable() &&
                            RaySphereIntersection( rayStart, rayDir, marker->m_pos, 8 ) )
                        {
                            // Extra mouse-over check - sphere on ground
                            m_newMarkerUnderMouse = marker->m_objId;
                            m_isOrderMarkerUnderMouse = true;
                        }

                        g_app->m_renderer->SetupMatricesFor2D();

                        RenderShadow        ( screenX, screenY, lineNormal.x, lineNormal.y, shadowSize, 0.9f * alpha );
                        RenderCompass       ( screenX, screenY, lineNormal.x, lineNormal.y, compassSize, 1.0f * alpha );
                        RenderColourCircle  ( screenX, screenY, lineNormal.x, lineNormal.y, iconSize, colour );
                        RenderIcon          ( screenX, screenY, lineNormal.x, lineNormal.y, iconSize, bmpFilename, RGBAColour(255,255,255,255*alpha) );                        

                        if( g_app->m_location->IsFriend( task->m_teamId, g_app->m_globalWorld->m_myTeamId ) )
                        {
                            float minX, minY, maxX, maxY;
                            CalculateScreenBox( screenX, screenY, lineNormal.x, lineNormal.y, iconSize, minX, minY, maxX, maxY );

                            TaskManagerInterfaceMultiwinia *tmm = (TaskManagerInterfaceMultiwinia *)g_app->m_taskManagerInterface;
                            tmm->RenderExtraTaskInfo( task, Vector2( (maxX+minX)/2.0f, (maxY+minY)/2.0f ), (maxX-minX), alpha, true );
                        }
                    }
                    else
                    {
                        marker->m_active = false;
                    }
                }
                break;
            }

            case LocationMarker::TypeFixed:
            {
				if( g_app->IsSinglePlayer() || marker->m_teamId != -1 )
                {
					RGBAColour colour(155,155,155,255);

					if( !g_app->IsSinglePlayer() )
					{
						Team *team = g_app->m_location->m_teams[ marker->m_teamId ];

						colour = team->m_colour;
					}
                    colour.a *= alpha;

                    if( marker->m_fromCrate )
                    {
                        float timePassed = GetHighResTime() - marker->m_birthTime;
                        if( timePassed < 2.5f ) marker->m_rotation += pow( 2.5f - timePassed, 2 );
                    }

                    RenderShadow        ( screenX, screenY, lineNormal.x, lineNormal.y, shadowSize, 0.9f * alpha );
                    RenderCompass       ( screenX, screenY, lineNormal.x, lineNormal.y, compassSize, 1.0f * alpha );
                    RenderColourCircle  ( screenX, screenY, lineNormal.x, lineNormal.y, iconSize, colour );
                    //RenderIcon          ( screenX, screenY, lineNormal.x, lineNormal.y, iconSize, "icons/icon_crate.bmp", RGBAColour(255,255,255,50*alpha), marker->m_rotation * -0.03f );
                    RenderIcon          ( screenX, screenY, lineNormal.x, lineNormal.y, iconSize, marker->m_icon, RGBAColour(255,255,255,255*alpha) );                    
                }
                break;
            }

            case LocationMarker::TypeAttachedToOfficer:
            {
                Officer *officer = (Officer *)g_app->m_location->GetEntitySafe( marker->m_objId, Entity::TypeOfficer );

                if( officer && !officer->m_dead && officer->m_enabled )
                {
                    //
                    // We do a slightly different mouse-over detection based on the flag

                    g_app->m_renderer->SetupMatricesFor3D();

                    Vector3 rayStart, rayDir;
                    g_app->m_camera->GetClickRay( g_target->X(), g_target->Y(), &rayStart, &rayDir );
                    if( officer->m_id.GetTeamId() == g_app->m_globalWorld->m_myTeamId &&
                        officer->m_flag.RayHit( rayStart, rayDir ) )
                    {
                        m_newMarkerUnderMouse = marker->m_objId;            
                    }

                    officer->m_flag.m_isHighlighted = (m_markerUnderMouse == officer->m_id ||
                                                       g_app->m_gameCursor->m_objUnderMouse == officer->m_id );                    
                    officer->RenderFlag(0.0f);
                    g_app->m_renderer->SetupMatricesFor2D();                    
                }

                break;
            }
        }

        g_app->m_renderer->SetupMatricesFor3D();
    }


    m_markerUnderMouse = m_newMarkerUnderMouse;
    m_newMarkerUnderMouse.SetInvalid();

    END_PROFILE("RenderMarkerSystem");
}

void MarkerSystem::ClearAllMarkers()
{
    m_markers.EmptyAndDelete();
}



// ============================================================================


LocationMarker::LocationMarker()
:   m_type(TypeUnknown),
    m_active(true),
    m_onscreenTimer(-1.0f),
    m_hasBeenSeen(false),
    m_teamId(-1),
    m_rotation(0.0f),
    m_fromCrate(true)
{
    m_birthTime = GetHighResTime();
}


bool LocationMarker::IsPersistant()
{
    return( m_type == TypeAttachedToTask ||
            m_type == TypeAttachedToCrate ||
            m_type == TypeAttachedToStatue ||
            m_type == TypeAttachedToOfficer ||
            m_type == TypeAttachedToOrders );
}


bool LocationMarker::ShowOffscreen()
{
    if( m_type == TypeAttachedToStatue ||
        m_type == TypeAttachedToCrate ) return true;

    if( m_type == TypeAttachedToTask &&
        m_teamId == g_app->m_globalWorld->m_myTeamId ) return true;

    if( !m_hasBeenSeen ) return true;

    return false;
}


bool LocationMarker::IsSelectable()
{
    if( m_teamId == g_app->m_globalWorld->m_myTeamId )
    {
        if( m_type == TypeAttachedToTask )
        {
            //Entity *attached = g_app->m_location->GetEntity( m_objId );
            //if( attached )
            //{
            //    Team *team = g_app->m_location->GetMyTeam();
            //    AppAssert(team);
            //    Entity *entity = team->GetMyEntity();

            //    if( !entity ) return true;
            //    if( attached->m_type != Entity::TypeArmour ) return true;
            //    if( entity == attached ) return true;
            //}

            //Unit *unit = g_app->m_location->GetUnit( m_objId );
            //if( unit ) return true;

            return true;
        }
        
        if( m_type == TypeAttachedToOfficer )
        {         
            return true;
        }

        if( m_type == TypeAttachedToOrders )
        {
            // Order markers are only selectable if we have nothing currently selected,
            // or we have the unit in question selected

            Entity *attached = g_app->m_location->GetEntity( m_objId );
            if( attached )
            {
                Team *team = g_app->m_location->GetMyTeam();
                AppAssert(team);

                Entity *entity = team->GetMyEntity();
                if( !entity && team->m_numDarwiniansSelected == 0 ) return true;
                if( entity == attached ) return true;
            }
        }
    }

    if( m_type == TypeAttachedToCrate )
    {
        return true;
    }

    return false;
}


bool LocationMarker::AlwaysAtFullAlpha()
{
    if( m_teamId == g_app->m_globalWorld->m_myTeamId )
    {
        if( m_type == TypeAttachedToTask ||
            m_type == TypeAttachedToOrders )
        {
            return true;
        }            
    }

    return false;
}


bool LocationMarker::ShouldFlashAlert()
{
    //
    // Basic rule - we demand attention if this marker is attached to something
    // that has happened without the player directly making it happen.
    // eg : received new armour unit
    //      enemy placed crate powerup


    if( g_app->m_multiwinia->GameInGracePeriod() ) return false;

    if( m_type == TypeAttachedToTask &&
        m_teamId == g_app->m_globalWorld->m_myTeamId ) return true;

    if( m_type == TypeFixed &&
        m_teamId != g_app->m_globalWorld->m_myTeamId ) return true;

    return false;
}
