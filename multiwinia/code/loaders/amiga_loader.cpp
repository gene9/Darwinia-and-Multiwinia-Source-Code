#include "lib/universal_include.h"

#ifdef USE_LOADERS

#include "lib/hi_res_time.h"
#include "lib/input/input.h"
#include "lib/math_utils.h"
#include "lib/resource.h"
#include "lib/text_renderer.h"
#include "lib/window_manager.h"
#include "lib/language_table.h"
#include "lib/math/random_number.h"

#include "loaders/amiga_loader.h"

#include "sound/soundsystem.h"

#include "3d_sierpinski_gasket.h"
#include "app.h"
#include "renderer.h"


class Star 
{
public:
	Vector2	m_pos;
	float	m_speed;
	unsigned char m_brightness;
};


AmigaLoader::AmigaLoader()
:	Loader(),
	m_scrollPhase(0.0f),
	m_scrollOffsetX(0.0),
	m_timeSinceStart(0.0f),
	m_fracTriPhase(20.0f),
	m_fracTriSpinRate(0.0f)
{
	m_sierpinski = new Sierpinski3D(10000);

	m_numStars = 4000;
	m_stars = new Star[m_numStars];
	for (int i = 0; i < m_numStars; ++i)
	{
		m_stars[i].m_pos.x = frand(1024);
		m_stars[i].m_pos.y = frand(768);
		m_stars[i].m_speed = -(frand(200) + 20);
		m_stars[i].m_brightness = frand(128) + 127;
	}
}


AmigaLoader::~AmigaLoader()
{
	delete m_sierpinski; m_sierpinski = NULL;
}


void AmigaLoader::SetupFor2d()
{
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0, 320, 256, 0);
	glMatrixMode(GL_MODELVIEW);
}


void AmigaLoader::SetupFor3d()
{
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(80.0f,	// FOV in degrees
				   1.33f,	// Aspect ratio
				   1, 1000);// Near and far planes
	glMatrixMode(GL_MODELVIEW);
}


void AmigaLoader::RenderStars(float _frameTime)
{
	glDisable(GL_BLEND);
	glPointSize(2.0f);
	glBegin(GL_POINTS);
	for (int i = 0; i < m_numStars; ++i)
	{
		Star &star = m_stars[i];
		star.m_pos.x += _frameTime * star.m_speed;
		while (star.m_pos.x < 0) star.m_pos.x += 1024;
		glColor3ub(star.m_brightness, star.m_brightness, star.m_brightness);
		glVertex2f(star.m_pos.x, star.m_pos.y);
	}
	glEnd();
}


void AmigaLoader::RenderScrollText(float _frameTime)
{
    UnicodeString theText = LANGUAGEPHRASE("bootloader_amiga");

	int len = theText.Length();
	float const textSpeed = 450.0f;
	double const textHeight = 55.0;
	double const textPitch = 32.0;
	int const charsPerScreen = 1024.0f / textPitch + 2;

	g_editorFont.BeginText2D();
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0, 1024, 768, 0);

	wchar_t buf[] = L"a";
	double fractionIndex = m_scrollOffsetX / textPitch;
	int rawIndex = floor(fractionIndex);
	float x = ((float)rawIndex - (float)fractionIndex) * (float)textPitch;

	for (int i = 0; i < charsPerScreen; ++i)
	{
		int index = rawIndex + i;
		index %= len;
		buf[0] = theText.m_unicodestring[index];
		float ourPhase = x * 0.01f + m_scrollPhase;
		float y = 40.0f + iv_sin(ourPhase) * 20.0f;
		int colour = iv_sin(ourPhase * 4.0f) * 100.0f + 101.0f;
		RGBAColour col;
		col.Set(32, 64, 64);
		glColor3ubv(col.GetData());
		g_editorFont.DrawText2DSimple(x, y, textHeight + 8, UnicodeString(buf));
		col.Set(255 - colour, 255 - colour, colour + 20);
		glColor3ubv(col.GetData());
		g_editorFont.DrawText2DSimple(x, y, textHeight, UnicodeString(buf));
		x += textPitch;
	}
		
	g_editorFont.EndText2D();

	m_scrollOffsetX += _frameTime * textSpeed;
	m_scrollPhase += _frameTime * 10.0f;
}


void AmigaLoader::RenderLogo(float _frameTime)
{
	Vector2 pos1, pos2, posOffset;

	float logoPhase = m_timeSinceStart * 4.0f;
	posOffset.x = iv_sin(logoPhase * 0.6f) * 40.0f;
	posOffset.y = iv_sin(logoPhase * 0.4f + 2.0f) * 40.0f;

	pos1.x = 80.0f + iv_sin(logoPhase) * 30.0f		+ posOffset.x;
	pos1.y = 120.0f + iv_sin(logoPhase + 1) * 40.0f	+ posOffset.y;

	pos2.x = 80.0f + iv_sin(logoPhase+1.5) * 40.0f	+ posOffset.x;
	pos2.y = 120.0f + iv_sin(logoPhase+3.5) * 50.0f	+ posOffset.y;

	glColor3ub(0,0,0);
	glDisable(GL_CULL_FACE);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glBindTexture(GL_TEXTURE_2D, g_app->m_resource->GetTexture("textures/dmacrew.bmp", true));
	glBegin(GL_QUADS);
		glTexCoord2f(0.0,  1.0);		glVertex2f(pos1.x,			pos1.y);
		glTexCoord2f(0.61, 1.0);		glVertex2f(pos1.x + 130,	pos1.y);
		glTexCoord2f(0.61, 0.765);		glVertex2f(pos1.x + 130,	pos1.y + 50);
		glTexCoord2f(0.0,  0.765);		glVertex2f(pos1.x,			pos1.y + 50);

		glTexCoord2f(0.0,  0.76);		glVertex2f(pos2.x,			pos2.y);
		glTexCoord2f(0.83, 0.76);		glVertex2f(pos2.x + 170,	pos2.y);
		glTexCoord2f(0.83, 0.59);		glVertex2f(pos2.x + 170,	pos2.y + 37);
		glTexCoord2f(0.0,  0.59);		glVertex2f(pos2.x,			pos2.y + 37);
	glEnd();
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_CULL_FACE);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
}


void AmigaLoader::RenderFracTri(float _frameTime)
{
	float const startTime = 10.0f;
	if (m_timeSinceStart < startTime) return;

	float scale = iv_sin(m_timeSinceStart * 4.0f) * 1.0f + 1.25f;

	SetupFor3d();
	glTranslatef(0, 0, -100);	
	glRotatef(90.0f, -1, 0, 0);
	glRotatef(m_fracTriPhase * 50.0f, m_fracTriPhase/50.0f, 0, 1);
	if (m_timeSinceStart < startTime + 1.0f)
	{
		scale = (m_timeSinceStart - startTime) * 2.0f;
	}
	else if (m_timeSinceStart < startTime + 2.0f)
	{
		float factor1 = m_timeSinceStart - (startTime + 1.0f);
		float factor2 = 1.0f - factor1;
		scale = scale * factor1 + 2.0f * factor2;
	}

	m_sierpinski->Render(scale);

	m_fracTriPhase += _frameTime * m_fracTriSpinRate;
	if (m_fracTriPhase > 0.0f)
	{
		m_fracTriSpinRate -= 2.1f * _frameTime;
	}
	else 
	{
		m_fracTriSpinRate += 2.1f * _frameTime;
	}
}


void AmigaLoader::Run()
{
    g_app->m_soundSystem->TriggerOtherEvent( NULL, "LoaderAmiga", SoundSourceBlueprint::TypeMusic );

	float fadeStartTime = GetHighResTime() + 72.0f;
	float endTime = GetHighResTime() + 89.0f;
	float lastTime = GetHighResTime();

	while ( !g_inputManager->controlEvent( ControlSkipMessage ) )
	{
		float timeNow = GetHighResTime();
		float frameTime = timeNow - lastTime;
		m_timeSinceStart += frameTime;
		lastTime = timeNow;

        if( g_app->m_requestQuit ) break;
		if (timeNow > endTime) break;

		SetupFor2d();

		RenderStars(frameTime);
		RenderFracTri(frameTime);
		SetupFor2d();
		RenderLogo(frameTime);
		RenderScrollText(frameTime);

		float fadeAmount = (timeNow - fadeStartTime) / 15.0f;
		if (fadeAmount > 1.0f) fadeAmount = 1.0f;
		if (fadeAmount > 0.0f)
		{
			glEnable(GL_BLEND);
			glColor4f(0,0,0, fadeAmount);
			glBegin(GL_QUADS);
				glVertex2f(0,256);
				glVertex2f(320,256);
				glVertex2f(320,0);
				glVertex2f(0,0);
			glEnd();
			glDisable(GL_BLEND);
		}

		FlipBuffers();

		AdvanceSound();
	}

    g_app->m_soundSystem->StopAllSounds( WorldObjectId(), "Music LoaderAmiga" );
}

#endif // USE_LOADERS
