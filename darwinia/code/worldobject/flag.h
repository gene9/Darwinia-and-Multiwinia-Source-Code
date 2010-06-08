
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
    Vector3 m_front;
    Vector3 m_up;
    float   m_size;

public:
    Flag();

    void Initialise         ();
    
    void SetTexture         ( int _textureId );
    void SetPosition        ( Vector3 const &_pos );
    void SetOrientation     ( Vector3 const &_front, Vector3 const &_up );
    void SetSize            ( float _size );

    void Render();
    void RenderText         ( int _posX, int _posY, char *_caption );    
};


#endif