#ifndef _included_chess_h
#define _included_chess_h

#include "worldobject/building.h"
#include "worldobject/carryablebuilding.h"



class ChessBase : public Building
{    
public:
    double m_width;
    double m_depth;
    double m_sparePoints;
    double m_timer;
    
public:
    ChessBase();

    void Initialise( Building *_template );

    void SpawnPiece( double _scale );

    bool Advance();
    void Render( double _precictionTime );

    void Read   ( TextReader *_in, bool _dynamic );     
    void Write  ( TextWriter *_out );    
};



class ChessPiece : public CarryableBuilding
{
public:
    int     m_chessBaseId;
    double   m_damage;

public:
    ChessPiece();

    void Initialise( Building *_template );

    bool Advance();

    void ExplodeShape( double _fraction );

    void SetWaypoint( Vector3 const &_waypoint );
    void HandleCollision( double _force );
    void HandleSuccess();                                       // we arrived at an enemy's chess base
};

#endif