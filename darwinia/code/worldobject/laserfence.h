#ifndef _included_laserfence_h
#define _included_laserfence_h

#include <stdio.h>

#include "worldobject/building.h"


class Shape;
class ShapeFragment;
class TextReader;

#define LASERFENCE_RAISESPEED       0.3f


class LaserFence : public Building
{
protected:    
    float           m_status;                       // 0=down, 1=up    
    int             m_nextLaserFenceId;
    float           m_sparkTimer;
    
    bool            m_radiusSet;
    
    ShapeMarker     *m_marker1;
    ShapeMarker     *m_marker2;

	bool			m_nextToggled;		// set to true when the fence has enabled/disabled the next fence in the line, to prevent constant enable calling

public:
    enum
    {
        ModeDisabled,
        ModeEnabling,
        ModeEnabled,
        ModeDisabling,
		ModeNeverOn
    };
    int             m_mode;    
    float           m_scale;
        
public:
    LaserFence();

    void Initialise( Building *_template );
    void SetDetail ( int _detail );

    bool Advance        ();
    void Render         ( float predictionTime );
    void RenderAlphas   ( float predictionTime );
    void RenderLights   ();

    bool PerformDepthSort   ( Vector3 &_centrePos );
    bool IsInView           ();

    void Read   ( TextReader *_in, bool _dynamic );
    void Write  ( FileWriter *_out );
    
    void Enable     ();
    void Disable    ();
    void Toggle     ();
	bool IsEnabled  ();

    void Spark      ();
    void Electrocute( Vector3 const &_pos );

    int  GetBuildingLink();                 
    void SetBuildingLink( int _buildingId );

    float GetFenceFullHeight    ();

    bool DoesSphereHit          (Vector3 const &_pos, float _radius);
    bool DoesRayHit             (Vector3 const &_rayStart, Vector3 const &_rayDir, 
                                 float _rayLen=1e10, Vector3 *_pos=NULL, Vector3 *_norm=NULL);
    bool DoesShapeHit           (Shape *_shape, Matrix34 _transform);

    void ListSoundEvents        (LList<char *> *_list );

    Vector3 GetTopPosition();
};


#endif
