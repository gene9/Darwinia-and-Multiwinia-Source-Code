#include "lib/universal_include.h"

#include <stdlib.h>

#include "lib/bitmap.h"
#include "lib/hi_res_time.h"
#include "lib/resource.h"
#include "lib/rgb_colour.h"
#include "lib/window_manager.h"
#include "lib/random.h"
#include "lib/input/input.h"

#include "sound/soundsystem.h"

#include "app.h"

#include "loaders/speccy_loader.h"


void SpeccyLoader::StartFrame()
{
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0, 320, 256, 0);
	glMatrixMode(GL_MODELVIEW);

	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
}


void SpeccyLoader::DrawLine(int y)
{
	glBegin(GL_QUADS);
		glVertex2i(0, y);
		glVertex2i(320, y);
		glVertex2i(320, y+1);
		glVertex2i(0, y+1);
	glEnd();
}


void SpeccyLoader::DrawText()
{
	int textureId = g_app->m_resource->GetTexture("textures/program_darwinia.bmp",false);
	glColor3ub(0,0,0);
	glEnable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, textureId);
	glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	glBegin(GL_QUADS);
		glTexCoord2i(0, 1);		glVertex2i(33, 40);
		glTexCoord2i(1, 1);		glVertex2i(256 + 33, 40);
		glTexCoord2i(1, 0);		glVertex2i(256 + 33, 40 + 256);
		glTexCoord2i(0, 0);		glVertex2i(33, 40 + 256);
	glEnd();
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
}


void SpeccyLoader::Header(float _endTime, bool _drawText)
{
	m_mode = ModeHeader;

	while (!g_inputManager->controlEvent( ControlSkipMessage ) && GetHighResTime() < _endTime)
	{
        if( g_app->m_requestQuit ) break;
		StartFrame();

		RGBAColour cols[2];
		cols[0].Set(255, 0, 0);
		cols[1].Set(0, 255, 255);

		int currentCol = 0;
		int nextSwitch = darwiniaRandom() % 4;

		for (int y = 0; y < 256; ++y)
		{
			glColor3ubv(cols[currentCol].GetData());
			if (y == nextSwitch)
			{
				int x = darwiniaRandom() % 1000;
				DrawLine(y);

				currentCol = 1 - currentCol;
				glColor3ubv(cols[currentCol].GetData());
				nextSwitch += 7;

				glBegin(GL_QUADS);
					glVertex2i(x, y);
					glVertex2i(320, y);
					glVertex2i(320, y+1);
					glVertex2i(x, y+1);
				glEnd();
			}
			else
			{
				DrawLine(y);
			}
		}

		glColor3ub(200,200,200);
		glBegin(GL_QUADS);
			glVertex2i(32, 32);
			glVertex2i(288, 32);
			glVertex2i(288, 224);
			glVertex2i(32, 224);
		glEnd();

		if (_drawText)
		{
			DrawText();
		}

		FlipBuffers();
		AdvanceSound();
	}
}


void SpeccyLoader::Data(float _endTime, bool _drawText)
{
	m_mode = ModeData;

	while (!g_inputManager->controlEvent( ControlSkipMessage ) && GetHighResTime() < _endTime)
	{
        if( g_app->m_requestQuit ) break;
		StartFrame();

		RGBAColour cols[2];
		cols[0].Set(255, 255, 0);
		cols[1].Set(0, 0, 255);

		int currentCol = 0;
		int nextSwitch = darwiniaRandom() % 4;

		for (int y = 0; y < 256; ++y)
		{
			glColor3ubv(cols[currentCol].GetData());
			if (y == nextSwitch)
			{
				int x = darwiniaRandom() % 1000;
				DrawLine(y);

				currentCol = 1 - currentCol;
				glColor3ubv(cols[currentCol].GetData());
				nextSwitch += 2 + darwiniaRandom() % 4;

				glBegin(GL_QUADS);
					glVertex2i(x, y);
					glVertex2i(320, y);
					glVertex2i(320, y+1);
					glVertex2i(x, y+1);
				glEnd();
			}
			else
			{
				DrawLine(y);
			}
		}

		glColor3ub(200,200,200);
		glBegin(GL_QUADS);
			glVertex2i(32, 32);
			glVertex2i(288, 32);
			glVertex2i(288, 224);
			glVertex2i(32, 224);
		glEnd();

		if (_drawText)
		{
			DrawText();
		}

		FlipBuffers();
		AdvanceSound();
	}
}


void SpeccyLoader::Silence(float _endTime, bool _drawText)
{
	m_mode = ModeSilence;
	int currentCol = 0;

	while (!g_inputManager->controlEvent( ControlSkipMessage ) && GetHighResTime() < _endTime)
	{
        if( g_app->m_requestQuit ) break;
		StartFrame();

		RGBAColour cols[2];
		cols[0].Set(255, 0, 0);
		cols[1].Set(0, 255, 255);

		if (darwiniaRandom() % 200 == 0)
		{
			currentCol = 1 - currentCol;
		}

		glColor3ubv(cols[currentCol].GetData());
		glBegin(GL_QUADS);
			glVertex2i(0, 0);
			glVertex2i(320, 0);
			glVertex2i(320, 256);
			glVertex2i(0, 256);
		glEnd();

		glColor3ub(200,200,200);
		glBegin(GL_QUADS);
			glVertex2i(32, 32);
			glVertex2i(288, 32);
			glVertex2i(288, 224);
			glVertex2i(32, 224);
		glEnd();

		if (_drawText)
		{
			DrawText();
		}

		FlipBuffers();
		AdvanceSound();
	}
}


void SpeccyLoader::Screen()
{
	m_mode = ModeScreen;

	double timeToAdvance = GetHighResTime() + 0.192;

	while ( !g_inputManager->controlEvent( ControlSkipMessage ) )
	{
        if( g_app->m_requestQuit ) break;
		StartFrame();


		//
		// Border effect

		RGBAColour cols[2];
		cols[0].Set(255, 255, 0);
		cols[1].Set(0, 0, 255);

		int currentCol = 0;
		int nextSwitch = darwiniaRandom() % 4;

		for (int y = 0; y < 256; ++y)
		{
			glColor3ubv(cols[currentCol].GetData());
			if (y == nextSwitch)
			{
				int x = darwiniaRandom() % 1000;
				DrawLine(y);

				currentCol = 1 - currentCol;
				glColor3ubv(cols[currentCol].GetData());
				nextSwitch += 2 + darwiniaRandom() % 4;

				glBegin(GL_QUADS);
					glVertex2i(x, y);
					glVertex2i(320, y);
					glVertex2i(320, y+1);
					glVertex2i(x, y+1);
				glEnd();
			}
			else
			{
				DrawLine(y);
			}
		}

		glColor3ub(200,200,200);
		glBegin(GL_QUADS);
			glVertex2i(32, 32);
			glVertex2i(288, 32);
			glVertex2i(288, 224);
			glVertex2i(32, 224);
		glEnd();


		DrawText();


		//
		// Loading screen image

		glColor3ub(200,200,200);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, g_app->m_resource->GetTexture("textures/speccy_screen.bmp", false, false));
		glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
		glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
		glBegin(GL_QUADS);
			for (int y = 0; y < m_nextLine; ++y)
			{
				int blah = ((y & 0x38) >> 3) + ((y & 0x07) << 3) + (y & 0xC0);
				float fy = (256.0f - blah) / 256.0f;
				float inc = 1.0f / 256.0f;
				glTexCoord2f(0, fy+inc);		glVertex2i(32, 32 + blah);
				glTexCoord2f(1, fy+inc);		glVertex2i(288, 32 + blah);
				glTexCoord2f(1, fy);			glVertex2i(288, 32 + blah + 1);
				glTexCoord2f(0, fy);			glVertex2i(32, 32 + blah + 1);
			}
		glEnd();
		glDisable(GL_TEXTURE_2D);
		if (GetHighResTime() > timeToAdvance && m_nextLine < 192)
		{
			m_nextLine++;
			timeToAdvance += 0.194f;
		}

		if (m_nextLine == 192)
		{
			return;
		}

		FlipBuffers();
		AdvanceSound();
	}
}


SpeccyLoader::SpeccyLoader()
:	Loader(),
	m_mode(-1),
	m_startJ(0),
	m_nextLine(0)
{
}


SpeccyLoader::~SpeccyLoader()
{
}


void SpeccyLoader::Run()
{
    g_app->m_soundSystem->TriggerOtherEvent( NULL, "LoaderSpeccy", SoundSourceBlueprint::TypeMusic );

	Silence(GetHighResTime() + 1.5f,	false);

	Header(GetHighResTime() + 4.8f,		false);
	Data(GetHighResTime() + 0.07f,		false);
	Silence(GetHighResTime() + 1.1f,	true);
	Header(GetHighResTime() + 2.0f,		true);
	Data(GetHighResTime() + 1.85f,		true);

	Silence(GetHighResTime() + 1.01f,	true);
	Header(GetHighResTime() + 5.05f,	true);
	Data(GetHighResTime() + 0.07f,		true);
	Silence(GetHighResTime() + 0.99f,	true);
	Header(GetHighResTime() + 1.9f,		true);
	Screen();

    g_app->m_soundSystem->StopAllSounds( WorldObjectId(), "Music LoaderSpeccy" );
}
