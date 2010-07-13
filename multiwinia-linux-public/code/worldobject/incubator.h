
#ifndef _included_incubator_h
#define _included_incubator_h

#include "lib/tosser/fast_darray.h"

#include "worldobject/building.h"
#include "worldobject/spirit.h"

class ShapeMarker;

#define INCUBATOR_PROCESSTIME                  5.0
#define INCUBATOR_PROCESSTIME_MULTIPLAYER      0.2

struct IncubatorIncoming
{
    Vector3 m_pos;
    int     m_entrance;
    double   m_alpha;
};


class Incubator: public Building
{
protected:
    FastDArray      <Spirit> m_spirits;
    ShapeMarker     *m_spiritCentre;
    ShapeMarker     *m_exit;
    ShapeMarker     *m_dock;
    ShapeMarker     *m_spiritEntrance[3];

    int             m_troopType;
    double           m_timer;
    
    LList           <IncubatorIncoming *> m_incoming;

public:
    int             m_numStartingSpirits;

public:
    Incubator();
    ~Incubator();

    void Initialise ( Building *_template );
    
    bool Advance    ();   
    void SpawnEntity();
    void AddSpirit  ( Spirit *_spirit );

    void Render         ( double _predictionTime );
    void RenderAlphas   ( double _predictionTime );

    int  NumSpiritsInside();

    void Read   ( TextReader *_in, bool _dynamic );     
    void Write  ( TextWriter *_out );							

    void GetDockPoint( Vector3 &_pos, Vector3 &_front );

    void ListSoundEvents( LList<char *> *_list );
};


#endif
