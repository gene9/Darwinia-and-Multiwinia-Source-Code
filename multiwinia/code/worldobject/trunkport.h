
#ifndef _included_trunkport_h
#define _included_trunkport_h

#include "worldobject/building.h"

#define TRUNKPORT_HEIGHTMAP_MAXSIZE 16

class TrunkPort : public Building
{
public:
    int m_targetLocationId;
    
    ShapeMarker *m_destination1;
    ShapeMarker *m_destination2;
    
    int     m_heightMapSize;
    Vector3 *m_heightMap;
    double   m_openTimer;
    int     m_populationLock;

public:
    TrunkPort();
	~TrunkPort();

    void Initialise         ( Building *_template );
    void SetDetail          ( int _detail );
    bool Advance            ();
    void Render             ( double predictionTime );
    void RenderAlphas       ( double predictionTime );       
    
    bool PerformDepthSort   ( Vector3 &_centrePos );     

    void ReprogramComplete();
    bool PopulationLocked();

    void ListSoundEvents    ( LList<char *> *_list );

    void Read   ( TextReader *_in, bool _dynamic );     
    void Write  ( TextWriter *_out );							
};



#endif
