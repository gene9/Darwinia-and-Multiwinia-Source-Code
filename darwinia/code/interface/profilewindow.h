#ifndef _included_profilewindow_h
#define _included_profilewindow_h

#ifdef PROFILER_ENABLED


#include "interface/darwinia_window.h"


class ProfiledElement;


//*****************************************************************************
// Class ProfileWindow
//*****************************************************************************

class ProfileWindow : public DarwiniaWindow
{
protected:
	int		m_yPos;

	void RenderElementProfile(ProfiledElement *_pe, unsigned int _indent);


public:
	bool	m_totalPerSecond;

    ProfileWindow( char *name );
    ~ProfileWindow();

    void Render( bool hasFocus );
	void Create();
	void Remove();
};


#endif // PROFILER_ENABLED

#endif
