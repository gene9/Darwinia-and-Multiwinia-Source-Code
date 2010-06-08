#ifndef SPECCY_LOADER_H
#define SPECCY_LOADER_H

#include "loaders/loader.h"


class SpeccyLoader : public Loader
{
protected:
	void StartFrame();
	void EndFrame();
	void DrawLine(int y);
	void DrawText();

	void Silence(float _endTime, bool _drawText);
	void Header(float _endTime, bool _drawText);
	void Data(float _endTime, bool _drawText);
	void Screen();

public:
	enum
	{
		ModeSilence,
		ModeHeader,
		ModeData,
		ModeScreen
	};

	bool	m_monoBmp[256 * 192];
	int		m_currentPixel;	// Used by the sound callback
	int		m_mode;
	int		m_startJ;
	int		m_nextLine;		// Used by renderer

public:    
	SpeccyLoader();
    ~SpeccyLoader();

	void Run();
};


#endif
