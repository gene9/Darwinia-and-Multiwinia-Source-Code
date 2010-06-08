#ifndef _included_spiritstore_h
#define _included_spiritstore_h

#include "lib/fast_darray.h"
#include "lib/vector3.h"

#include "worldobject/spirit.h"


class SpiritStore 
{
public:
    Vector3     m_pos;
    float       m_sizeX;
    float       m_sizeY;
    float       m_sizeZ;
    
protected:
    FastDArray  <Spirit> m_spirits;

public:
    SpiritStore     ();                  

    void Initialise ( int _initialCapacity, int _maxCapacity, Vector3 _pos, 
                      float _sizeX, float _sizeY, float _sizeZ );            // Capacity isn't enforced, just provide a "best guess"

    void Advance    ();
    void Render     ( float _predictionTime );

    int  NumSpirits     ();
    void AddSpirit      ( Spirit *_spirit );
    void RemoveSpirits  ( int _quantity );
};


#endif
