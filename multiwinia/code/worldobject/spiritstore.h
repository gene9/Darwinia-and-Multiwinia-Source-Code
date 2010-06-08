#ifndef _included_spiritstore_h
#define _included_spiritstore_h

#include "lib/tosser/fast_darray.h"
#include "lib/vector3.h"

#include "worldobject/spirit.h"


class SpiritStore 
{
public:
    Vector3     m_pos;
    double       m_sizeX;
    double       m_sizeY;
    double       m_sizeZ;
    
protected:
    FastDArray  <Spirit> m_spirits;

public:
    SpiritStore     ();                  

    void Initialise ( int _initialCapacity, int _maxCapacity, Vector3 _pos, 
                      double _sizeX, double _sizeY, double _sizeZ );            // Capacity isn't enforced, just provide a "best guess"

    void Advance    ();
    void Render     ( double _predictionTime );

    int  NumSpirits     ();
    void AddSpirit      ( Spirit *_spirit );
    void RemoveSpirits  ( int _quantity );
};


#endif
