#ifndef _included_raytraceloader_h
#define _included_raytraceloader_h

#include "lib/universal_include.h"

#ifdef USE_LOADERS

#include "lib/vector3.h"
#include "lib/rgb_colour.h"

#include "loaders/loader.h"

#define RAYTRACELOADER_MAXOBJECTS   10
#define RAYTRACELOADER_RENDERDEPTH  2
#define RAYTRACELOADER_MAXRES       5

struct RayTraceObject
{
public:
    Vector3 m_pos;
    float m_radius;
    RGBAColour m_colour;
};



class RayTraceLoader : public Loader
{
protected:   
    void        SetupMatrices       ( Vector3 &_left, Vector3 &_up, Vector3 &_horizDiff, Vector3 &_vertDiff );

    void        CastAllRays         ();
    int         CastRay             ( int _x, int _y, 
                                      Vector3 const &_startRay, Vector3 const &_rayDir, 
                                      int _numStepsRemaining, float _intensity );
    
    void        RenderRays          ();
    void        RenderBackground    ();
    void        RenderMessage       ();    
    void        RenderHelp          ();

    int         GetFloorColour      ( Vector3 const &pos );             // Returns 0 or 1

    void        AdvanceObjects      ( float _advanceTime );
    void        AdvanceControls     ( float _advanceTime );

protected:
    RGBAColour      m_buffer[RAYTRACELOADER_MAXRES*160][RAYTRACELOADER_MAXRES*120];
    RayTraceObject  m_objects[RAYTRACELOADER_MAXOBJECTS];
    Vector3         m_light;
    Vector3         m_floor;
    int             m_numObjects;   
    
    int             m_resolution;
    float           m_reflectiveness;
    float           m_pixelWave;
    float           m_motionBlur;
    float           m_pixelBlur;
    float           m_brightness;

    Vector3         m_cameraPos;
    Vector3         m_cameraFront;
    Vector3         m_cameraUp;
    int             m_cameraMode;               // 0 = fixed.  1 = controllable.  2 = rotating
    
    float           m_startTime;
    float           m_time;
    
public:
    RayTraceLoader();

    void Run();
};

#endif // USE_LOADERS

#endif
