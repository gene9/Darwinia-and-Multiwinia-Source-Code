#ifndef _included_laserfence_h
#define _included_laserfence_h

#include <stdio.h>

#include "worldobject/building.h"


class Shape;
class ShapeFragment;
class TextReader;

#define LASERFENCE_RAISESPEED       0.3


class LaserFence : public Building
{
protected:    
    double           m_status;                       // 0=down, 1=up    
    int             m_nextLaserFenceId;
    double           m_sparkTimer;
    
    bool            m_radiusSet;
    
    ShapeMarker     *m_marker1;
    ShapeMarker     *m_marker2;

	bool			m_nextToggled;		// set to true when the fence has enabled/disabled the next fence in the line, to prevent constant enable calling
    bool            m_solarMode;        // receiving power from solar panels, so switch off if the supply stops
	bool			m_solarLinked;		// a laser fence that is connected to us is solar powered, which means if it shuts down, we will too

    double           m_surgeReceived;

public:
    enum
    {
        ModeDisabled,
        ModeEnabling,
        ModeEnabled,
        ModeDisabling,
		ModeNeverOn,
        ModeAlwaysOn
    };
    int             m_mode;    
    double           m_scale;
        
public:
    LaserFence();

    void Initialise( Building *_template );
    void SetDetail ( int _detail );

    bool Advance        ();
    void Render         ( double predictionTime );
    void RenderAlphas   ( double predictionTime );
    void RenderLights   ();

    bool PerformDepthSort   ( Vector3 &_centrePos );
    bool IsInView           ();

    void Read   ( TextReader *_in, bool _dynamic );
    void Write  ( TextWriter *_out );
    
    void Enable     ();
    void Disable    ();
    void Toggle     ();
	bool IsEnabled  ();

    void Spark      ();
    void Electrocute( Vector3 const &_pos );

	void SetSolarLinked();

    void ProvidePower();

    int  GetBuildingLink();                 
    void SetBuildingLink( int _buildingId );

    double GetFenceFullHeight    ();

    bool DoesSphereHit          (Vector3 const &_pos, double _radius);
    bool DoesRayHit             (Vector3 const &_rayStart, Vector3 const &_rayDir, 
                                 double _rayLen=1e10, Vector3 *_pos=NULL, Vector3 *_norm=NULL);
    bool DoesShapeHit           (Shape *_shape, Matrix34 _transform);

    void ListSoundEvents        (LList<char *> *_list );

    Vector3 GetTopPosition();
};


#endif
