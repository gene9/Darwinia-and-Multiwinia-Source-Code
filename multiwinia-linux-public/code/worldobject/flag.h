
#ifndef _included_flag_h
#define _included_flag_h

#include "lib/vector3.h"

#define FLAG_RESOLUTION 6


class Flag
{
protected:
    Vector3 m_flag[FLAG_RESOLUTION][FLAG_RESOLUTION];

    int     m_texId;
    Vector3 m_pos;
    Vector3 m_target;
    Vector3 m_front;
    Vector3 m_up;
    bool    m_viewFlipped;
    double   m_size;
	
    Vector3 m_centre;
    float   m_radius;

public:
	bool	m_isSelected;
    bool    m_isHighlighted;

    Flag();

    void Initialise         ();
    
    void SetTexture         ( int _textureId );
    void SetPosition        ( Vector3 const &_pos );
    void SetTarget          ( Vector3 const &_pos );
    void SetOrientation     ( Vector3 const &_front, Vector3 const &_up );
    void SetSize            ( double _size );
    void SetViewFlipped     ( bool _flipped );

    void Render             ( int _teamId = -1, char *_fileName = NULL );
    void RenderText         ( int _posX, int _posY, char *_caption );    

    bool RayHit             ( Vector3 const &rayStart, Vector3 const &rayDir );
};


#endif