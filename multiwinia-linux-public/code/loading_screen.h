#ifndef _included_loadingscreen_h
#define _included_loadingscreen_h

#include "lib/work_queue.h"
	
class TextureToConvert;
class BitmapRGBA;

class LoadingScreen
{
public:
    LoadingScreen();
    
    void Render();
	void QueueJob( Job *_t );
	void RunJobs();
	bool IsRendering() const;

public:
	bool		m_rendering;
    int         m_texId;
    bool        m_initialLoad;
	bool		m_platformLogoShown;

	double      m_startTime;

    WorkQueue   *m_workQueue;

private:

	void RenderFrame();
	void RenderSpinningDarwinian();
	void RenderIVLogo( float _alpha );
	void RenderPlatformLogo( const char *path, int w, int h, bool scale, float _alpha );

	bool JobsToDo();
	Job *TryDequeueJob();


private:

	NetMutex		m_lock;
	LList<Job *>	m_jobs;

	double			m_startFadeOutTime;
	bool			m_fadeOutLogo;

};

extern LoadingScreen *g_loadingScreen;

#endif
