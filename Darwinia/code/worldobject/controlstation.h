
#ifndef _included_controlstation_h
#define _included_controlstation_h

#include "worldobject/building.h"


class TextWriter;

class ControlStation : public Building
{
protected:
	int		m_controlBuildingId;
	bool	m_scored;
    ShapeMarker     *m_lightPos;

public:
	ControlStation();
	bool Advance();

	void Initialise( Building *_template );

	int  GetBuildingLink();                 
    void SetBuildingLink( int _buildingId );

	void RecalculateOwnership();

	void RenderPorts();
    void RenderAlphas( double _predictionTime );
    bool PerformDepthSort( Vector3 &_centrePos );

    bool IsInView();

	void Read   ( TextReader *_in, bool _dynamic );
    void Write  ( FileWriter *_out );
};

#endif
