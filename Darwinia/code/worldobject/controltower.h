
#ifndef _included_controltower_h
#define _included_controltower_h

#include "worldobject/building.h"


class FileWriter;


class ControlTower : public Building
{
public:
    float       m_ownership;                            // 100 = strongly owned, 5 = nearly lost, 0 = neutral

protected:
    static Shape    *s_dishShape;
    ShapeMarker     *m_lightPos;
    ShapeMarker     *m_reprogrammer[3];
    ShapeMarker     *m_console[3];
    ShapeMarker     *m_dishPos;

    Matrix34        m_dishMatrix;

    bool        m_beingReprogrammed[3];                 // One bool for each slot
    int         m_controlBuildingId;                    // Whom I affect

    float       m_checkTargetTimer;

public:
    ControlTower();

    void Initialise( Building *_template );

    bool Advance        ();

    bool IsInView       ();
    void Render         ( float _predictionTime );
    void RenderAlphas   ( float _predictionTime );

    int  GetAvailablePosition   ( Vector3 &_pos, Vector3 &_front );         // Finds place for reprogrammer
    void GetConsolePosition     ( int _position, Vector3 &_pos );

    void BeginReprogram         ( int _position );
    bool Reprogram              ( int _teamId );                            // Returns true if job completed
    void EndReprogram           ( int _position );

    void ListSoundEvents        ( LList<char *> *_list );

    void Read   ( TextReader *_in, bool _dynamic );
    void Write  ( FileWriter *_out );

    int  GetBuildingLink();
    void SetBuildingLink( int _buildingId );

};


#endif
