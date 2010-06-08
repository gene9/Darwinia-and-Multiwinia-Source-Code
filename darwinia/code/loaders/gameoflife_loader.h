
#ifndef _included_gameoflife_h
#define _included_gameoflife_h

#include "loaders/loader.h"


class GameOfLifeLoader : public Loader
{
protected:
    int     m_numCellsX;
    int     m_numCellsY;
    int     *m_cells;
    int     *m_cellsTemp;
    int     *m_age;
    float   m_zoom;
    float   m_speed;
    int     m_numFound;
    float   m_allDead;
    bool    m_glow;
    int     m_totalAge;
    
    enum
    {
        CellStateEmpty=0,
        CellStateAlive,
        CellStateDead
    };

protected:
    void SetupScreen( float _screenW, float _screenH );

    void ClearCells();
    void RandomiseCells();
    void PropagateCells( float _start, float _end );                // Does calculations into cellsTemp
    void CommitCells();                                             // Copies cellsTemp into cells

    void RenderDarwinian( int _cellX, int _cellY, int _age );
    void RenderSpirit( int _cellX, int _cellY, int _age );

    void RenderHelp();

public:
    GameOfLifeLoader( bool _glow );

    void Run();
};


#endif