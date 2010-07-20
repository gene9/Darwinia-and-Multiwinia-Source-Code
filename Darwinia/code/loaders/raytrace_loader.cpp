#include "lib/universal_include.h"

#include <float.h>

#include "lib/window_manager.h"
#include "lib/input/input.h"
#include "lib/targetcursor.h"
#include "lib/math_utils.h"
#include "lib/hi_res_time.h"
#include "lib/resource.h"
#include "lib/text_renderer.h"
#include "lib/matrix33.h"
#include "lib/language_table.h"
#ifdef _OPENMP
#include <omp.h>
#endif

#include "loaders/raytrace_loader.h"

#include "app.h"
#include "main.h"
#include "renderer.h"

#include "sound/soundsystem.h"


inline int RayTraceLoader::GetFloorColour( Vector3 const &pos )
{
    int x = pos.x;
    int z = pos.z + m_time * 100.0f;
    bool result = (x ^ z) & 32;
    return result;
}


inline bool RayIntersectPlane(  Vector3 const &rayStart, Vector3 const &rayDir,
                                Vector3 const &planePos, Vector3 const &planeNormal,
                                Vector3 *pos )
{
    float d = -planePos.y;
    float t = -( rayStart * planeNormal + d ) / ( rayDir * planeNormal );
    if( t < 0.0f ) return false;

    *pos = rayStart + rayDir * t;

    return true;
}


inline bool RayIntersectSphere( Vector3 const &rayStart, Vector3 const &rayDir,
                                Vector3 const &spherePos, float sphereRadius,
                                Vector3 *pos, Vector3 *normal )
{
    Vector3 l = spherePos - rayStart;
    float tca = l * rayDir;
    if( tca < 0.0f ) return false;

    float r2 = sphereRadius * sphereRadius;

    float d2 = (l * l) - (tca * tca);
    if( d2 > r2 ) return false;

    float thc = sqrtf( r2 - d2 );
    float t = tca - thc;

    *pos = rayStart + rayDir * t;
    *normal = *pos - spherePos;
    *normal /= sphereRadius;

    return true;
}


struct Intersection
{
    Vector3 m_pos;
    Vector3 m_norm;
    float m_distance;
};


inline Vector3 CalculateReflectionVector( Vector3 const &_incomingDir, Vector3 const &_normal )
{
    float dotProd = _normal * (_incomingDir * -1);
    Vector3 reflectionVector = 2.0f * dotProd * _normal + _incomingDir;
    return reflectionVector;
}


inline void CalculateSphereRefraction( Vector3 const &_incomingDir, Vector3 const &_hitPoint, Vector3 const &_hitNormal,
                                       Vector3 const &_spherePos, float _sphereRadius,
                                       Vector3 &_outgoingDir, Vector3 &_outgoingPoint )
{
#define REFRACTION_COEFFICIENT 1.4f

    Vector3 X = _hitNormal;
    Vector3 Y = _hitNormal * ( _incomingDir * _hitNormal ) - _incomingDir;
    Y.Normalise();

    float theta = acosf( -_incomingDir * _hitNormal );
    float thetaPrime = asinf( sinf(theta) / REFRACTION_COEFFICIENT );

    //Vector3 internalAngle = (cosf(thetaPrime) * X) + (sinf(thetaPrime) * Y);
    //_outgoingPoint = _hitPoint + 2.0f * _sphereRadius * cosf(thetaPrime) * internalAngle;

//    Vector3 internalAngle( -cosf( thetaPrime), -sinf( thetaPrime ), 0.0f );
//    Vector3 centrePoint( -_sphereRadius, 0, 0 );
//    Vector3 exitPoint = 2.0f * _sphereRadius * cosf(thetaPrime) * internalAngle;
//    Vector3 exitNormal = (exitPoint - centrePoint).Normalise();
//    Vector3 exitVector = exitNormal;
//    exitVector.RotateAroundZ( -theta );

    Vector3 incomingRay( -cosf(theta), -sinf(theta), 0.0f );
    Vector3 innerRay = incomingRay;
    innerRay.RotateAroundZ( thetaPrime - theta );
    float innerDistance = 2.0f * _sphereRadius * cosf(thetaPrime);
    Vector3 exitPoint = innerRay * innerDistance;
    Vector3 exitVector = incomingRay;
    exitVector.RotateAroundZ( thetaPrime - theta );

    _outgoingDir = exitVector.x * X + exitVector.y * Y;
    _outgoingPoint = exitPoint.x * X + exitPoint.y * Y + _hitPoint;
}


int RayTraceLoader::CastRay( int _x, int _y,
                               Vector3 const &_startRay, Vector3 const &_rayDir,
                               int _numStepsRemaining, float _intensity )
{
    if( _numStepsRemaining < 0 ) return 0;

    //
    // Find the nearest intersecting object

    float nearest = FLT_MAX;
    Vector3 intersectPoint, intersectNormal;
    RGBAColour colour;
    bool hitSomething = false;
    Vector3 spherePos;
    float radius;

    for( int i = 0; i < m_numObjects; ++i )
	{
        Vector3 thisIntersectPoint, thisIntersectNormal;
        bool hit = RayIntersectSphere( _startRay, _rayDir, m_objects[i].m_pos, m_objects[i].m_radius, &thisIntersectPoint, &thisIntersectNormal );
        if( hit )
        {
            float distance = ( thisIntersectPoint - _startRay ).MagSquared();
            if( distance < nearest )
            {
                nearest = distance;
                intersectPoint = thisIntersectPoint;
                intersectNormal = thisIntersectNormal;
                colour = m_objects[i].m_colour;
                spherePos = m_objects[i].m_pos;
                radius = m_objects[i].m_radius;
                hitSomething = true;
            }
        }
    }

    if( hitSomething )
    {
        //
        // Render that part of the sphere

//        Vector3 outgoingDir, outgoingPoint;
//        CalculateSphereRefraction( _rayDir, intersectPoint, intersectNormal, spherePos, radius, outgoingDir, outgoingPoint );
//        RenderRay( _x, _y, _dx, _dy, outgoingPoint, outgoingDir, _numStepsRemaining-1, _intensity*0.9f );

        float intensity = intersectNormal * m_light * -1.0f;
        intensity = max( intensity, 0.0f );
        intensity = min( intensity, 1.0f );
        intensity *= _intensity;
        colour *= intensity;
        colour.a = 255 * m_reflectiveness;
        m_buffer[_x][_y].AddWithClamp( colour );

        Vector3 reflectionVector = CalculateReflectionVector( _rayDir, intersectNormal );
        int numHits = CastRay( _x, _y, intersectPoint, reflectionVector, _numStepsRemaining-1, _intensity );

        return numHits+1;
    }
    else
    {
        bool hitFloor = RayIntersectPlane( _startRay, _rayDir, m_floor, g_upVector, &intersectPoint );
        if( hitFloor )
        {
            //Vector3 reflectionVector = CalculateReflectionVector( _rayDir, g_upVector );
            //RenderRay( _x, _y, _dx, _dy, intersectPoint, reflectionVector, _numStepsRemaining-1, _intensity*0.5f );

            int hitFloorWhite = GetFloorColour( intersectPoint );
            if( hitFloorWhite )
            {
                float intensity = 100000.0f / ( intersectPoint ).MagSquared();
                intensity = min( intensity, 1.0f );
                intensity *= _intensity;
                RGBAColour floorColour( 200, 200, 200, 200 );
                floorColour *= intensity;
                m_buffer[_x][_y].AddWithClamp( floorColour );
            }
        }

        return 0;
    }
}


RayTraceLoader::RayTraceLoader()
:   Loader()
{
    glDisable           (GL_DEPTH_TEST );
    glDisable           (GL_CULL_FACE );
#ifndef USE_DIRECT3D
    glEnable            (GL_BLEND ); // our d3d present discards old backbuffer
#endif
    glShadeModel        (GL_FLAT);

    //
    // Set up our objects

    m_numObjects = 6;

    m_objects[0].m_radius = 10.0f;
    m_objects[0].m_colour.Set( 200, 130, 100, 255 );

    for( int i = 1; i < m_numObjects; ++i )
    {
        m_objects[i].m_radius = 5.0f;
        if( i % 2 == 0 )    m_objects[i].m_colour.Set( 100, 200, 130, 255 );
        else                m_objects[i].m_colour.Set( 200, 200, 130, 255 );
    }

    m_floor.Set(0, -40, 0);

    m_light.Set(1,0,1);
    m_light.Normalise();

    m_time = GetHighResTime();
    m_startTime = m_time;

    m_resolution = 2;
    m_reflectiveness = 1.0f;
    m_pixelWave = 0.0f;
    m_motionBlur = 0.1f;
    m_pixelBlur = 2.0f;
    m_brightness = 1.2f;

    m_cameraPos.Set( 0.0f, 0.0f, -100.0f );
    m_cameraFront.Set( 0.0f, 0.0f, 1.0f );
    m_cameraUp.Set( 0.0f, 1.0f, 0.0f );
    m_cameraMode = 1;
}


void RayTraceLoader::AdvanceObjects(float _advanceTime)
{
    static Vector3 angle1( 0, 0, 1 );

    angle1.RotateAroundY( _advanceTime*0.75f );        // 0.75
    angle1.RotateAroundX( _advanceTime*0.75f );        // 0.75

    float zPos = 700.0f - sqrtf( m_time - m_startTime ) * 150.0f;
    zPos = max( zPos, 0.0f );

//    if( m_time > m_startTime + 45.0f )
//    {
//        m_pixelWave += 0.01f;
//        m_pixelWave = min( m_pixelWave, 2.0f );
//    }

    if( m_time > m_startTime + 60.0f )
    {
        zPos = 0.0f - 2.0f*(m_time-(m_startTime+60.0f));
    }

    float yPos = -7.0f + sinf(m_time/6.0f) * 10.0f;
    m_objects[0].m_pos.Set( 0.0f,
                            yPos,
                            zPos );

    Vector3 angle( 0, 1, 0 );
    angle.RotateAround( angle1 * (m_time+20.0f) * 0.1f );

    for( int i = 1; i < m_numObjects; ++i )
    {
        m_objects[i].m_pos = m_objects[0].m_pos + angle * (10.0f+i*5.0f);
        angle.RotateAround( angle1 * 1.0f * M_PI * 1.0f/(m_numObjects-1) );
        angle.RotateAroundZ( (m_time+20.0f) * 0.05f );     // 0.05
    }


    if( m_cameraMode == 2 )
    {
        m_cameraPos.RotateAroundY( _advanceTime * 0.3f );
        m_cameraPos.y = 10.0f + sinf(m_time * 0.8f) * 10.0f;
        m_cameraPos.SetLength( 100.0f + sinf(m_time*0.5f) * 40.0f );
        m_cameraFront = -m_cameraPos;
        m_cameraFront.Normalise();
        Vector3 camRight = g_upVector ^ m_cameraFront;
        m_cameraUp = m_cameraFront ^ camRight;
    }
}


void RayTraceLoader::AdvanceControls( float _advanceTime )
{
    if( g_inputManager->controlEvent( ControlRTLoaderReflectivenessIncrease ) )
    {
        m_reflectiveness += _advanceTime;
        m_reflectiveness = min( m_reflectiveness, 1.0f );
    }

    if( g_inputManager->controlEvent( ControlRTLoaderReflectivenessDecrease ) )
    {
        m_reflectiveness -= _advanceTime;
        m_reflectiveness = max( m_reflectiveness, 0.0f );
    }

    if( g_inputManager->controlEvent( ControlRTLoaderPixelWaveIncrease ) )
    {
        m_pixelWave += _advanceTime;
    }

    if( g_inputManager->controlEvent( ControlRTLoaderPixelWaveDecrease ) )
    {
        m_pixelWave -= _advanceTime;
        m_pixelWave = max( m_pixelWave, 0.0f );
    }

    if( g_inputManager->controlEvent( ControlRTLoaderMotionBlurIncrease ) )
    {
        m_motionBlur += _advanceTime * 0.1f;
        m_motionBlur = min( m_motionBlur, 1.0f );
    }

    if( g_inputManager->controlEvent( ControlRTLoaderMotionBlurDecrease ) )
    {
        m_motionBlur -= _advanceTime * 0.1f;
        m_motionBlur = max( m_motionBlur, 0.0f );
    }

    if( g_inputManager->controlEvent( ControlRTLoaderPixelBlurIncrease ) )
    {
        m_pixelBlur++;
    }

    if( g_inputManager->controlEvent( ControlRTLoaderPixelBlurDecrease ) )
    {
        m_pixelBlur--;
        m_pixelBlur = max( m_pixelBlur, 1.0f );
    }

    if( g_inputManager->controlEvent( ControlRTLoaderBrightnessIncrease ) )
    {
        m_brightness += _advanceTime * 0.1f;
    }

    if( g_inputManager->controlEvent( ControlRTLoaderBrightnessDecrease ) )
    {
        m_brightness -= _advanceTime * 0.1f;
    }

    if( g_inputManager->controlEvent( ControlRTLoaderResolutionDecrease ) )
    {
        m_resolution--;
        m_resolution = max( m_resolution, 1 );
    }

    if( g_inputManager->controlEvent( ControlRTLoaderResolutionIncrease ) )
    {
        m_resolution++;
        m_resolution = min( m_resolution, RAYTRACELOADER_MAXRES );
    }

    if( g_inputManager->controlEvent( ControlRTLoaderCameraMode1 ) ) m_cameraMode = 0;
    if( g_inputManager->controlEvent( ControlRTLoaderCameraMode2 ) ) m_cameraMode = 1;
    if( g_inputManager->controlEvent( ControlRTLoaderCameraMode3 ) ) m_cameraMode = 2;

    if( m_cameraMode == 1 )
    {
        Vector3 camRight = m_cameraUp ^ m_cameraFront;
        if( g_inputManager->controlEvent( ControlCameraForwards ) ) m_cameraPos += m_cameraFront * _advanceTime * 20;
        if( g_inputManager->controlEvent( ControlCameraBackwards ) ) m_cameraPos -= m_cameraFront * _advanceTime * 20;
        if( g_inputManager->controlEvent( ControlCameraLeft ) ) m_cameraPos -= camRight * _advanceTime * 20;
        if( g_inputManager->controlEvent( ControlCameraRight ) ) m_cameraPos += camRight * _advanceTime * 20;
        if( g_inputManager->controlEvent( ControlCameraUp ) ) m_cameraPos += m_cameraUp * _advanceTime * 20;
        if( g_inputManager->controlEvent( ControlCameraDown ) ) m_cameraPos -= m_cameraUp * _advanceTime * 20;

        if( g_inputManager->controlEvent( ControlRTLoaderRMB ) )
        {
            Matrix33 mat(1);
		    mat.RotateAroundY((float)g_target->dX() * _advanceTime * 0.1f );
		    m_cameraUp = m_cameraUp * mat;
		    m_cameraFront = m_cameraFront * mat;

            Vector3 right = m_cameraUp ^ m_cameraFront;
		    mat.SetToIdentity();
		    mat.FastRotateAround(right, (float)g_target->dY() * _advanceTime * -0.1f );
		    m_cameraUp = m_cameraUp * mat;
		    m_cameraFront = m_cameraFront * mat;
        }
    }

    if( g_inputManager->controlEvent( ControlRTLoaderLMB ) )
    {
        Vector3 right = m_light ^ g_upVector;
        Vector3 up = right ^ m_light;
        m_light.RotateAround( up * (float)g_target->dX() * 0.01f );
        m_light.Normalise();
    }
}


void RayTraceLoader::RenderMessage()
{
    if( m_time > m_startTime + 30.0f )
    {
        float alpha = m_time - ( m_startTime + 30.0f );
        alpha *= 0.1f;
        if( m_time > m_startTime + 60.0f )
        {
            alpha = m_time - ( m_startTime + 60.0f );
            alpha *= 0.1f;
            alpha = 1.0f - alpha;
        }

        alpha = min( alpha, 1.0f );
        alpha = max( alpha, 0.0f );

        float screenX = 160;
        screenX += sinf(m_time*2.0f) * 0.5f;

        for( int i = 0; i < 2; ++i )
        {
            float yPos = 20;
            yPos += sinf(m_time*3.0f) * 0.5f;

            if( i == 0 )
            {
                g_gameFont.SetRenderShadow(true);
                glColor4f( alpha, alpha, alpha, 0.0f );
            }
            else
            {
                g_gameFont.SetRenderShadow(false);
                glColor4f( 0.8f, 0.8f, 1.0f, alpha );
                yPos++;
            }

            float texH = 9;
            g_gameFont.DrawText2DCentre( screenX, yPos, texH,              "================================" );
            g_gameFont.DrawText2DCentre( screenX, yPos+=texH,       texH*4,"DARWINIA" );
            g_gameFont.DrawText2DCentre( screenX, yPos+=texH*3.5,   texH,  "================================" );
            g_gameFont.DrawText2DCentre( screenX, yPos+=texH*2,     texH,  LANGUAGEPHRASE("bootloader_raytrace_1") );
            g_gameFont.DrawText2DCentre( screenX, yPos+=texH,       texH,  LANGUAGEPHRASE("bootloader_raytrace_2") );
            g_gameFont.DrawText2DCentre( screenX, yPos+=texH,       texH,  LANGUAGEPHRASE("bootloader_raytrace_3") );
            g_gameFont.DrawText2DCentre( screenX, yPos+=texH,       texH,  LANGUAGEPHRASE("bootloader_raytrace_4") );
            g_gameFont.DrawText2DCentre( screenX, yPos+=texH*5,     texH,  LANGUAGEPHRASE("bootloader_raytrace_5") );
            g_gameFont.DrawText2DCentre( screenX, yPos+=texH,       texH,  LANGUAGEPHRASE("bootloader_raytrace_6") );
            g_gameFont.DrawText2DCentre( screenX, yPos+=texH,       texH,  LANGUAGEPHRASE("bootloader_raytrace_7") );
            g_gameFont.DrawText2DCentre( screenX, yPos+=texH,       texH,  LANGUAGEPHRASE("bootloader_raytrace_8") );
            g_gameFont.DrawText2DCentre( screenX, yPos+=texH*4,     texH,  LANGUAGEPHRASE("bootloader_raytrace_9") );
        }
    }
}


void RayTraceLoader::RenderHelp()
{
    int screenW = m_resolution * 160;
    int screenH = m_resolution * 120;

    float screenX = screenW * 0.2f;
    screenX += sinf(m_time*2.0f) * 0.5f;

    for( int i = 0; i < 2; ++i )
    {
        float yPos = screenH * 0.2f;
        yPos += sinf(m_time*3.0f) * 0.5f;

        if( i == 0 )
        {
            g_gameFont.SetRenderShadow(true);
            glColor4f( 1.0f, 1.0f, 1.0f, 0.0f );
        }
        else
        {
            g_gameFont.SetRenderShadow(false);
            glColor4f( 0.8f, 0.8f, 1.0f, 1.0f );
            yPos++;
        }

        float texH = screenW/32;

        g_gameFont.DrawText2D( screenX, yPos+=texH, texH*2,     LANGUAGEPHRASE("bootloader_raytrace_10") );
        g_gameFont.DrawText2D( screenX, yPos+=texH*3, texH,     "X - C  %15s %dx%d", LANGUAGEPHRASE("bootloader_raytrace_11"), m_resolution*160, m_resolution*120 );
        g_gameFont.DrawText2D( screenX, yPos+=texH, texH,       "O - P  %15s %2.2f", LANGUAGEPHRASE("bootloader_raytrace_12"), m_reflectiveness );
        g_gameFont.DrawText2D( screenX, yPos+=texH, texH,       "K - L  %15s %2.2f", LANGUAGEPHRASE("bootloader_raytrace_13"), m_pixelWave );
        g_gameFont.DrawText2D( screenX, yPos+=texH, texH,       "N - M  %15s %2.2f", LANGUAGEPHRASE("bootloader_raytrace_14") );
        g_gameFont.DrawText2D( screenX, yPos+=texH, texH,       "U - I  %15s %2.2f", LANGUAGEPHRASE("bootloader_raytrace_15"), m_pixelBlur );
        g_gameFont.DrawText2D( screenX, yPos+=texH, texH,       "V - B  %15s %2.2f", LANGUAGEPHRASE("bootloader_raytrace_16"), m_brightness );
        g_gameFont.DrawText2D( screenX, yPos+=texH*2, texH,     "1      %s", LANGUAGEPHRASE("bootloader_raytrace_17") );
        g_gameFont.DrawText2D( screenX, yPos+=texH, texH,       "2      %s", LANGUAGEPHRASE("bootloader_raytrace_18") );
        g_gameFont.DrawText2D( screenX, yPos+=texH, texH,       "3      %s", LANGUAGEPHRASE("bootloader_raytrace_19") );
        g_gameFont.DrawText2D( screenX, yPos+=texH, texH,       "LMB    %s", LANGUAGEPHRASE("bootloader_raytrace_20") );
        g_gameFont.DrawText2D( screenX, yPos+=texH, texH,       "RMB    %s", LANGUAGEPHRASE("bootloader_raytrace_21") );
        g_gameFont.DrawText2D( screenX, yPos+=texH, texH,       "WASD   %s", LANGUAGEPHRASE("bootloader_raytrace_22") );
        g_gameFont.DrawText2D( screenX, yPos+=texH, texH,       "QE     %s", LANGUAGEPHRASE("bootloader_raytrace_23") );


    }
}


void RayTraceLoader::RenderBackground()
{
    glShadeModel( GL_SMOOTH );

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    float blur = 1.0f - m_motionBlur;

    int screenW = m_resolution * 160;
    int screenH = m_resolution * 120;

    glBegin( GL_QUAD_STRIP );
        glColor4f( 0.0f, 0.0f, 0.1f, blur );
        glVertex2i( 0, 0 );
        glVertex2i( screenW, 0 );

        glColor4f( 0.0f, 0.0f, 0.0f, blur );
        glVertex2i( 0, screenW/2.0f );
        glVertex2i( screenW, screenH/2.0f );

        glColor4f( 0.0f, 0.1f, 0.1f, blur );
        glVertex2i( 0, screenH );
        glVertex2i( screenW, screenH );
    glEnd();

    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    glShadeModel( GL_FLAT );
}


void RayTraceLoader::SetupMatrices( Vector3 &_left, Vector3 &_up, Vector3 &_horizDiff, Vector3 &_vertDiff )
{
    int screenW = m_resolution * 160;
    int screenH = m_resolution * 120;

    //
    // Set up the render matrices

	glMatrixMode        (GL_MODELVIEW);
	glLoadIdentity      ();
	glMatrixMode        (GL_PROJECTION);
	glLoadIdentity      ();
    gluOrtho2D(0, screenW, screenH, 0);
    glMatrixMode        (GL_MODELVIEW);


    //
    // Calculate our rendering bounds

    float fov = 90.0f;
    float ratio = (float) g_app->m_renderer->ScreenH() / (float) g_app->m_renderer->ScreenW();
    float vfov = fov * ratio;

    Vector3 camRight = m_cameraUp ^ m_cameraFront;
    Vector3 leftMost = m_cameraFront;
    Vector3 rightMost = m_cameraFront;
    Vector3 upMost = m_cameraFront;
    Vector3 downMost = m_cameraFront;

    leftMost.FastRotateAround( m_cameraUp, -2.0f * M_PI * (fov/2.0f)/360.0f );
    rightMost.FastRotateAround( m_cameraUp, 2.0f * M_PI * (fov/2.0f)/360.0f );
    upMost.FastRotateAround( camRight, -2.0f * M_PI * (vfov/2.0f)/360.0f );
    downMost.FastRotateAround( camRight, 2.0f * M_PI * (vfov/2.0f)/360.0f );

    Vector3 horizDif = rightMost - leftMost;
    Vector3 vertDif = downMost - upMost;

    _left = leftMost;
    _up = upMost;
    _horizDiff = horizDif;
    _vertDiff = vertDif;
}


void RayTraceLoader::CastAllRays()
{
    int screenW = m_resolution * 160;
    int screenH = m_resolution * 120;
    Vector3 leftMost, upMost, horizDif, vertDif;
    SetupMatrices( leftMost, upMost, horizDif, vertDif );

    int maxScreenW = RAYTRACELOADER_MAXRES * 160;
    int maxScreenH = RAYTRACELOADER_MAXRES * 120;
    memset( m_buffer, 0, maxScreenW*maxScreenH*sizeof(RGBAColour) );

	#pragma omp parallel for
    for( int x = 0; x < screenW; ++x )
    {
		Vector3 horiz = leftMost + horizDif * (x / (float) screenW );
        for( int y = 0; y < screenH; ++y )
        {
            Vector3 vert = upMost + vertDif * (y / (float) screenH );
            Vector3 clickRay = (horiz + vert).Normalise();

            int numHits = CastRay( x, y, m_cameraPos, clickRay,
                                   RAYTRACELOADER_RENDERDEPTH, m_brightness/m_pixelBlur );
        }
    }

}


void RayTraceLoader::RenderRays()
{
    int screenW = m_resolution * 160;
    int screenH = m_resolution * 120;

    glBlendFunc(GL_SRC_ALPHA, GL_ONE );
    glShadeModel( GL_SMOOTH );

    for( int y = 0; y < screenH-1; ++y )
    {
        glBegin( GL_QUAD_STRIP );
        for( int x = 0; x < screenW; ++x )
        {
            float dx = 0.0f;
            float dy = 0.0f;
            if( m_pixelWave > 0.0f )
            {
                dx = sinf(m_time * 5.0f + y * 0.1f) * m_pixelWave;
                dy = sinf(m_time * 5.0f + x * 0.1f) * m_pixelWave;
            }

            RGBAColour col1 = m_buffer[x][y];
            RGBAColour col2 = m_buffer[x][y+1];
            col1.a = 255;
            col2.a = 255;
            glColor4ubv( col1.GetData() );
            glVertex2f( x+dx, y+dy );
            glColor4ubv( col2.GetData() );
            glVertex2f( x+dx, y+m_pixelBlur+dy );
        }
        glEnd();
    }
}


void RayTraceLoader::Run()
{
    g_app->m_soundSystem->TriggerOtherEvent( NULL, "LoaderRaytrace", SoundSourceBlueprint::TypeMusic );

    //
    // Go into our render loop

	float endOfSecond = GetHighResTime() + 1.0f;
	int fps = 0;
	int lastFps = 0;
    while( !g_inputManager->controlEvent( ControlSkipMessage ) )
    {
        if( g_app->m_requestQuit ) break;
        float oldTime = m_time;
        m_time = GetHighResTime();
        float timeThisFrame = m_time - oldTime;
        AdvanceObjects(timeThisFrame);
        AdvanceControls( timeThisFrame );

		if (m_time > endOfSecond)
		{
			endOfSecond += 1.0f;
			lastFps = fps;
			fps = 0;
		}
		else
		{
			++fps;
		}

        CastAllRays();
#ifdef USE_DIRECT3D
		glClear(GL_COLOR_BUFFER_BIT);
#endif
        RenderBackground();
        RenderRays();

        glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
        //g_gameFont.DrawText2D(5, 10, 5, "%d", lastFps);

        if( g_inputManager->controlEvent( ControlLoaderHelp ) ) RenderHelp();
		else RenderMessage();

        g_windowManager->Flip();
		g_inputManager->Advance();
        g_inputManager->PollForEvents();
        AdvanceSound();
        Sleep(1);
    }

    g_app->m_soundSystem->StopAllSounds( WorldObjectId(), "Music LoaderRaytrace" );
}
