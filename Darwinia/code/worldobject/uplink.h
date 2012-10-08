
#ifndef _included_uplink_h
#define _included_uplink_h

#include "worldobject/building.h"

class UplinkDataPacket
{
public:
    float   m_currentProgress;
};

class Uplink : public Building
{

protected:
	    LList<UplinkDataPacket *> m_packets;


public:

	int m_direction;

public:
    Uplink();

    void Initialise     ( Building *_template );

	void Render( float _predictionTime );
	void RenderAlphas ( float _predictionTime );
	void RenderPacket ( Vector3 const &_pos );

    void Read( TextReader *_in, bool _dynamic );
    void Write( FileWriter *_out );

	bool Advance();

protected:
	Vector3	GetDataNode ();
};


#endif