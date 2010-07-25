#include "lib/universal_include.h"

#include <memory>

#include <eclipse.h>

#include "lib/hi_res_time.h"
#include "lib/input/input.h"
#include "lib/targetcursor.h"
#include "lib/language_table.h"
#include "lib/llist.h"
#include "lib/mouse_cursor.h"
#include "lib/preferences.h"
#include "lib/resource.h"
#include "lib/text_renderer.h"
#include "lib/debug_render.h"
#include "lib/profiler.h"
#include "lib/math_utils.h"
#include "lib/shape.h"
#include "lib/string_utils.h"
#include "lib/filesys_utils.h"

#include "app.h"
#include "camera.h"
#include "gamecursor.h"
#include "gesture_demo.h"
#include "location.h"
#include "main.h"
#include "renderer.h"
#include "sepulveda.h"
#include "taskmanager.h"
#include "taskmanager_interface.h"
#include "tutorial.h"
#include "script.h"
#include "control_help.h"

#include "sound/soundsystem.h"



void Sepulveda::LoadAvatar ( char *_imageName )
{
	int i = 1;

    char filename[256];
    sprintf( filename, "sepulveda/%s1.bmp",_imageName );
	
	char dataFileName[256];
	char modFileName[256];

    sprintf( dataFileName, "data/%s", filename );
    sprintf( modFileName, "%smods/%s/%s", g_app->GetProfileDirectory(), g_app->m_resource->GetModName(), filename );

	while ( DoesFileExist(dataFileName) || DoesFileExist(modFileName) )
    {
        g_app->m_resource->GetTexture( filename, true, false );

		i++;
		sprintf( filename, "sepulveda/%s%d.bmp",_imageName, i );
		sprintf( dataFileName, "data/%s", filename );
		sprintf( modFileName, "%smods/%s/%s", g_app->GetProfileDirectory(), g_app->m_resource->GetModName(), filename );
    }
	m_picsFound = i-1;
	//DarwiniaReleaseAssert(m_picsFound == 0, "Unable to load avatar: %s", _imageName );
}

Sepulveda::Sepulveda()
:   m_timeSync(0.0),
	m_gestureDemo(NULL),
	m_mouseCursor(NULL),
    m_fade(0.0f),
    m_previousNumChars(0),
    m_cutsceneMode(false),
    m_scrollbarOffset(0)
{
	m_mouseCursor = new MouseCursor("icons/mouse_main.bmp");

    //
    // Pre-cache sepulveda images to avoid stuttering on load

	m_currentAvatar = new char[512];
	sprintf(m_currentAvatar,"sepulveda");

	LoadAvatar("sepulveda");
	//int i = 0;
    //char filename[256];
    //sprintf( filename, "sepulveda/sepulveda%d.bmp", i+1 );
	//while ( 
    //for( int i = 0; i < 5; ++i )
    //{
    //    g_app->m_resource->GetTexture( filename, true, false );
    //}

    m_caption[0] = '\x0';

	// Put some of the operating system specific definitions into the
	// language hash

#ifdef TARGET_OS_MACOSX
	m_langDefines.PutData("part_leftclick", "Click");
	m_langDefines.PutData("part_rightclick", "Right click");
	m_langDefines.PutData("part_leftmousebutton", "Mouse button");
	m_langDefines.PutData("part_leftclicking", "Clicking");
#else
	m_langDefines.PutData("part_leftclick", "Left click");
	m_langDefines.PutData("part_rightclick", "Right click");
	m_langDefines.PutData("part_leftmousebutton", "Left mouse button");
	m_langDefines.PutData("part_leftclicking", "Left clicking");
#endif


    //
    // Test english.txt

    if( g_prefsManager->GetInt( "TextLanguageTest", 0 ) == 1 )
    {
        FILE *output = fopen( "language_errors.txt", "at" );

        DArray<LangPhrase *> *phrases = g_app->m_langTable->GetPhraseList();

        for( int i = 0; i < phrases->Size(); ++i )
        {
            if( phrases->ValidIndex(i) )
            {
                LangPhrase *phrase = phrases->GetData(i);
    			char buf[SEPULVEDA_MAX_PHRASE_LENGTH];
                bool result = BuildCaption( phrase->m_string, buf );
                if( !result )
                {
                    fprintf( output, "ERROR : Invalid keycode with string : '%s'\n", phrase->m_key );
                }

//#ifndef NDEBUG
                if( strstr( phrase->m_string, "gesture" ) ||
                    strstr( phrase->m_string, "Gesture" ) ||
                    strstr( phrase->m_string, "[TAB]" ) )
                {
                    char iconsId[256];
                    sprintf( iconsId, "%s_icons", phrase->m_key );
                    if( !ISLANGUAGEPHRASE(iconsId) )
                    {
                        fprintf( output, "WARNING : StringID found that may require ICONS version : '%s'\n", phrase->m_key );
                    }
                }
//#endif
            }
        }

        fclose( output );

        delete phrases;
    }
}


//////////////////////////////////////////////////////////////////////////////
// And now for something completely different - String handling functions.  //
//////////////////////////////////////////////////////////////////////////////

//bool buildCaption(char const *_baseString, char *_dest); // see sepulveda_strings.cpp

bool Sepulveda::BuildCaption(char const *_baseString, char *_dest) {
//	return buildCaption( _baseString, _dest );
	strncpy( _dest, _baseString, SEPULVEDA_MAX_PHRASE_LENGTH - 1 );
	_dest[SEPULVEDA_MAX_PHRASE_LENGTH-1] = '\x0';
	return true;
}

//////////////////////////////////////////////////////////////////////////////
// End of the string handling functions.                                    //
//////////////////////////////////////////////////////////////////////////////


void Sepulveda::Advance()
{
    //
    // Has enough time passed to get rid of the existing message?

    bool captionGone = false;

    if( m_caption[0] != '\0' )
    {
        float textTime = CalculateTextTime();
        float pauseTime = CalculatePauseTime();

        if( GetHighResTime() >= m_timeSync + textTime + pauseTime ||
            g_inputManager->controlEvent( ControlSkipMessage ) )
        {
            m_caption[0] = '\0';
		    m_gestureDemo = NULL;
            m_timeSync = GetHighResTime();
            captionGone = true;
        }
        else
        {
            m_history[0]->m_timeSync = GetHighResTime();
        }
    }


    //
    // Are there more messages waiting in the queue?

    if( m_caption[0] == '\0' && m_msgQueue.Size() > 0 )
    {
        char *stringId = m_msgQueue[0];
        m_msgQueue.RemoveData(0);
		std::auto_ptr<SepulvedaCaption> sepulvedaCaption = std::auto_ptr<SepulvedaCaption>( new SepulvedaCaption() );
        sepulvedaCaption->m_stringId = NewStr(stringId);
        sepulvedaCaption->m_timeSync = GetHighResTime();

        DarwiniaDebugAssert( ISLANGUAGEPHRASE_ANY( stringId ) );

        BuildCaption( LANGUAGEPHRASE( stringId ), m_caption );
		if ( m_caption[0] != '\0' ) {
			m_history.PutDataAtStart( sepulvedaCaption.release() );
        m_timeSync = GetHighResTime();

	        if( !captionGone ) {
            // Sepulveda just started talking
            g_app->m_soundSystem->TriggerOtherEvent( NULL, "Appear", SoundSourceBlueprint::TypeSepulveda );
        }
    }
    }

    if( captionGone && m_caption[0] == '\0' )
    {
        // Sepulveda just finished talking
        g_app->m_soundSystem->TriggerOtherEvent( NULL, "Disappear", SoundSourceBlueprint::TypeSepulveda );
    }


    //
    // Advance our highlights

    for( int i = 0; i < m_highlights.Size(); ++i )
    {
        SepulvedaHighlight *highlight = m_highlights[i];
        bool amIDone = highlight->Advance();
        if( amIDone )
        {
            delete highlight;
            m_highlights.RemoveData(i);
            --i;
        }
    }


    //
    // Pressing TAB should re-highlight the text box

    bool chatLog = g_app->m_sepulveda->ChatLogVisible();
    if( chatLog )
    {
        float timeNow = GetHighResTime();
        for( int i = 0; i < m_history.Size(); ++i )
        {
            m_history[i]->m_timeSync = max( m_history[i]->m_timeSync, timeNow - 3.0f );
        }
    }


    //
    // If we are in a cutscene mode
    // Make sure all the message history is faded out

    if( m_cutsceneMode )
    {
        for( int i = 0; i < m_history.Size(); ++i )
        {
            SepulvedaCaption *caption = m_history[i];
            caption->m_timeSync -= 30.0f;
        }
    }
}


bool Sepulveda::PlayerSkipsMessage()
{
    if( m_caption[0] != '\0' )
    {
        m_caption[0] = '\0';
		m_gestureDemo = NULL;	// No need to delete the gesture demo because it is owned by the resource system
        return true;
    }

    return false;
}


void Sepulveda::Say( char *_stringId )
{
    //
    // If we are using a 1 button mouse, look out for an alternative stringID
    // Same if we are using the icon based control mechanism

    char *actualStringId = NULL;

    int numMouseButtons = g_prefsManager->GetInt( "ControlMouseButtons" );
    if( numMouseButtons == 1 )
    {
        char phrase1mb[256];
        sprintf( phrase1mb, "%s_1mb", _stringId );
        if( ISLANGUAGEPHRASE( phrase1mb ) )
        {
            actualStringId = NewStr( phrase1mb );
        }
    }

    int controlMethod = g_prefsManager->GetInt( "ControlMethod" );
    if( controlMethod == 1 )
    {
        char phrase[256];
        sprintf( phrase, "%s_icons", _stringId );
        if( ISLANGUAGEPHRASE( phrase ) )
        {
            actualStringId = NewStr( phrase );
        }
    }

    if( !actualStringId )
    {
        actualStringId = NewStr( _stringId );
    }

	// Is thisi string empty?
	if ( actualStringId[0] == '\0' )
	{
		delete [] actualStringId;
		return;
	}

    // Has Sepulveda already said this string?
    if( m_history.Size() > 0 )
    {
        char *mostRecentStringId = m_history[0]->m_stringId;
        if( strcmp( mostRecentStringId, actualStringId ) == 0 && IsTalking() )
        {
            delete [] actualStringId;
            return;
        }
    }

	// Is this string already in the message queue?
	for (int i = 0; i < m_msgQueue.Size(); ++i)
	{
		char *stringId = m_msgQueue[i];
		if (strcmp( stringId, actualStringId ) == 0)
		{
            delete [] actualStringId;
			return;
		}
	}

    m_msgQueue.PutDataAtEnd( actualStringId );
}


void Sepulveda::HighlightPosition( Vector3 const &_pos, float _radius, char *_highlightName )
{
    //
    // Does the highlight already exist?
    bool found = false;

    for( int i = 0; i < m_highlights.Size(); ++i )
    {
        SepulvedaHighlight *highlight = m_highlights[i];
        if( highlight->m_pos == _pos && NearlyEquals( highlight->m_radius, _radius ) )
        {
            found = true;
            break;
        }
    }

    if( !found )
    {
        SepulvedaHighlight *highlight = new SepulvedaHighlight( _highlightName );
        highlight->Start( _pos, _radius );
        m_highlights.PutData( highlight );
    }
}


void Sepulveda::HighlightBuilding( int _buildingId, char *_highlightName )
{
    if( !g_app->m_location ) return;

    Building *building = g_app->m_location->GetBuilding( _buildingId );
    if( building )
    {
        SepulvedaHighlight *highlight = new SepulvedaHighlight( _highlightName );
        highlight->Start( building->m_centrePos, building->m_radius );
        m_highlights.PutData( highlight );
    }
}


bool Sepulveda::IsHighlighted( Vector3 const &_pos, float _radius, char *_highlightName )
{
    for( int i = 0; i < m_highlights.Size(); ++i )
    {
        SepulvedaHighlight *highlight = m_highlights[i];
        if( !_highlightName || strcmp( _highlightName, highlight->m_name ) == 0 )
        {
            if( highlight->m_pos == _pos && highlight->m_radius == _radius )
            {
                return true;
            }
        }
    }

    return false;
}


void Sepulveda::ClearHighlights( char *_highlightName )
{
    for( int i = 0; i < m_highlights.Size(); ++i )
    {
        SepulvedaHighlight *highlight = m_highlights[i];
        if( !_highlightName || strcmp( _highlightName, highlight->m_name ) == 0 )
        {
            highlight->Stop();
        }
    }
}


void Sepulveda::ShutUp()
{
    if( m_caption[0] != '\0' )
    {
        m_caption[0] = '\0';
    }

    ClearHighlights( NULL );
    m_gestureDemo = NULL;

    m_msgQueue.EmptyAndDelete();
}


void Sepulveda::DemoGesture(char const *_gestureDemoName, float _startDelay)
{
    if( g_prefsManager->GetInt( "ControlMethod" ) == 1 )
    {
        // We're using the icons system, so this is clearly a mistake
        return;
    }

	m_gestureDemo = g_app->m_resource->GetGestureDemo(_gestureDemoName);
	m_gestureStartTime = g_gameTime + _startDelay;
}


bool Sepulveda::IsTalking()
{
    return( m_caption[0] != '\0' || m_msgQueue.Size() > 0 );
}


bool Sepulveda::IsVisible()
{
    if( IsTalking() ) return true;

    if( m_history.Size() == 0 ) return false;

    float startingAlpha = 0.8f;
    startingAlpha *= GetCaptionAlpha(m_history[0]->m_timeSync);

    if( startingAlpha > 0.1f ) return true;

    return false;
}


float Sepulveda::CalculateTextTime()
{
    if( m_caption[0] == '\0' )
    {
        return 0.0f;
    }
    else
    {
        int totalLen = strlen(m_caption);
        int narratorSpeed = g_prefsManager->GetInt( "TextSpeed", 15 );
        float timeToWait = 0.6f * (float) totalLen / (float) narratorSpeed;

        return timeToWait;
    }
}


float Sepulveda::CalculatePauseTime()
{
    if( m_caption[0] == '\0' )
    {
        return 0.0f;
    }
    else
    {
        int totalLen = strlen(m_caption);
        int narratorSpeed = g_prefsManager->GetInt( "TextSpeed", 15 );
        float timeToWait = (float) totalLen / (float) narratorSpeed;

        if( !m_cutsceneMode ) timeToWait *= 0.5f;

        return timeToWait;
    }
}


void Sepulveda::RenderGestureDemo()
{
	float timeSinceStart = g_gameTime - m_gestureStartTime;

	if (timeSinceStart < 0.0f)
	{
		return;
	}

	float totalDuration = m_gestureDemo->GetTotalDuration();
	if (timeSinceStart > totalDuration + 2.0f)
	{
		m_gestureStartTime = g_gameTime + 2.0f;
		return;
	}
	else if (timeSinceStart > totalDuration)
	{
		timeSinceStart = totalDuration;
	}


	g_gameFont.BeginText2D();

	GestureMouseCoords coords = m_gestureDemo->GetCoords(0.0f);
	float prevX = coords.x;
	float prevY = coords.y;
	glColor3ub(255,128,128);
	glEnable(GL_BLEND);
	glLineWidth(40.0f);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBegin(GL_LINES);
		for (float i = 0.0f; i < timeSinceStart; i += 0.04f)
		{
			float ageOfThisLine = g_gameTime - m_gestureStartTime - i;
			if (ageOfThisLine > 2.0f) ageOfThisLine = 2.0f;
			unsigned char col = 127.0f * (2.0f - ageOfThisLine);
            col = max( col, 128 );
			glColor4ub(col>>1, col>>1, col, col);
			coords = m_gestureDemo->GetCoords(i);
			glVertex2f(prevX, prevY);
			glVertex2f(coords.x, coords.y);
			prevX = coords.x;
			prevY = coords.y;
		}
	glEnd();
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_BLEND);

	m_mouseCursor->Render(coords.x, coords.y);

	g_gameFont.EndText2D();
}


void Sepulveda::RenderHighlights()
{
    START_PROFILE( g_app->m_profiler, "Highlights" );

    if( m_highlights.Size() > 0 )
    {
        glEnable        ( GL_BLEND );
        glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );
        glEnable        ( GL_TEXTURE_2D );
        glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/godray.bmp" ) );
        //glDisable       ( GL_DEPTH_TEST );
        glDepthMask     ( false );

	    float nearPlaneStart = g_app->m_renderer->GetNearPlane();
	    g_app->m_camera->SetupProjectionMatrix(nearPlaneStart * 1.5f,
							 			       g_app->m_renderer->GetFarPlane());

        for( int i = 0; i < m_highlights.Size(); ++i )
        {
            SepulvedaHighlight *highlight = m_highlights[i];
            highlight->Render();
        }

	    g_app->m_camera->SetupProjectionMatrix(nearPlaneStart,
								 		       g_app->m_renderer->GetFarPlane());

        glDepthMask     ( true );
        //glEnable        ( GL_DEPTH_TEST );
        glDisable       ( GL_TEXTURE_2D );
        glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        glDisable       ( GL_BLEND );
    }

    END_PROFILE( g_app->m_profiler, "Highlights" );
}


float Sepulveda::GetCaptionAlpha( float _timeSync )
{
    float timePassed = GetHighResTime() - _timeSync;
    float alphaScale = 1.0f - timePassed / 20.0f;
    alphaScale = max( alphaScale, 0.0f );
    alphaScale = min( alphaScale, 1.0f );
    return alphaScale;
}


void Sepulveda::SetCutsceneMode( bool _cutsceneMode )
{
    m_cutsceneMode = _cutsceneMode;
}


bool Sepulveda::IsInCutsceneMode()
{
    return m_cutsceneMode;
}


void Sepulveda::RenderTextBoxCutsceneMode()
{
    //
    // Setup render matrices

    g_gameFont.BeginText2D();

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

    float screenRatio = (float) g_app->m_renderer->ScreenW() / (float) g_app->m_renderer->ScreenH();
    float m_screenH = 600.0f;
    float m_screenW = m_screenH * screenRatio;

    gluOrtho2D(0, m_screenW, m_screenH, 0);
	glMatrixMode(GL_MODELVIEW);

    float textH = 18.0f;
    float textBoxX = m_screenW/2-300;
    textBoxX += 20.0f;
    float textBoxW = 600;
    float textBoxY = 450;
    float textBoxH = 140;

    float sepulvedaH = 100;
    float sepulvedaW = sepulvedaH * 0.8f;
    float sepulvedaX = textBoxX - sepulvedaW - 10;
    float sepulvedaY = 430;     //590 - sepulvedaH;

    if( m_caption[0] != '\0' )
    {
        //m_fade = m_fade * 0.95f + 0.05f;
        //m_fade = min( 1.0f, m_fade );
        m_fade = 1.0f;
    }
    else
    {
        //m_fade = m_fade * 0.95f;
        m_fade = 0.0f;
    }


    //
    // Render background box + sepulveda

    float startingAlpha = m_fade * 0.75f;


    //
    // Render sepulveda

    RenderFace( sepulvedaX, sepulvedaY, sepulvedaW, sepulvedaH, m_fade );


    //
    // Current text appearing on screen

    float yPos = 440.0f;
    float alpha = 1.0f;
    float textX = textBoxX+15;

    glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );

    if( m_caption[0] != '\0' )
    {
        float textTime = CalculateTextTime();
        float finishTime = m_timeSync + textTime;
        float fractionDone = ( GetHighResTime() - m_timeSync ) / ( finishTime - m_timeSync );
        if( fractionDone < 0.0f ) fractionDone = 0.0f;
        if( fractionDone > 1.0f ) fractionDone = 1.0f;

        int totalChars = strlen(m_caption);
        int finishingChar = totalChars * fractionDone;
        int thisChar = 0;

        if( finishingChar != m_previousNumChars )
        {
            g_app->m_soundSystem->TriggerOtherEvent( NULL, "TextAppear", SoundSourceBlueprint::TypeSepulveda );
            m_previousNumChars = finishingChar;
        }

        glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );

        LList<char *> *wrapped = WordWrapText( m_caption, textBoxW-15, g_editorFont.GetTextWidth(1,textH), true );

        for( int i = 0; i < wrapped->Size(); i++ )
        {
            char *thisLine = wrapped->GetData(i);

            if( thisChar + strlen(thisLine) < finishingChar )
            {
                g_gameFont.SetRenderOutline(true);
                glColor4f( 1.0f, 1.0f, 1.0f, 0.0f );
                g_gameFont.DrawText2D( textX, yPos, textH, thisLine );
                glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
                g_gameFont.SetRenderOutline(false);
                g_gameFont.DrawText2D( textX, yPos, textH, thisLine );
                yPos += textH;
            }
            else if( thisChar < finishingChar )
            {
                char thisLineCopy[512];
                strcpy( thisLineCopy, thisLine );
                int finishingCharIndex = ( finishingChar - thisChar );
                thisLineCopy[ finishingCharIndex ] = '\x0';
                g_gameFont.SetRenderOutline(true);
                glColor4f( 1.0f, 1.0f, 1.0f, 0.0f );
                g_gameFont.DrawText2D( textX, yPos, textH, thisLineCopy );
                glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
                g_gameFont.SetRenderOutline(false);
                g_gameFont.DrawText2D( textX, yPos, textH, thisLineCopy );
                yPos += textH;
            }

            thisChar += strlen(thisLine);
        }
        delete [] wrapped->GetData(0);
        delete wrapped;
    }


    glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
    if( g_app->m_script->m_permitEscape )
    {
        g_gameFont.DrawText2DRight( textBoxX+textBoxW, 590.0f, 7.0f, LANGUAGEPHRASE("sepulveda_pressreturn_skipall") );
    }
    else if( m_msgQueue.Size() > 0 )
    {
        char theString[256];
        sprintf( theString, "%s (%d)", LANGUAGEPHRASE("sepulveda_pressreturn"), m_msgQueue.Size() );
        g_gameFont.DrawText2DRight( textBoxX+textBoxW, 590.0f, 7.0f, theString );
    }
    else if( m_caption[0] != '\0' )
    {
        g_gameFont.DrawText2DRight( textBoxX+textBoxW, 590.0f, 7.0f, LANGUAGEPHRASE("sepulveda_pressreturn") );
    }

    //
    // Reset render matrices

    g_app->m_renderer->SetupMatricesFor3D();
    g_gameFont.EndText2D();

}


void Sepulveda::RenderTextBoxTaskManagerMode()
{
    //
    // Setup render matrices

    g_gameFont.BeginText2D();

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

    float screenRatio = (float) g_app->m_renderer->ScreenW() / (float) g_app->m_renderer->ScreenH();
    float m_screenH = 600.0f;
    float m_screenW = m_screenH * screenRatio;

    gluOrtho2D(0, m_screenW, m_screenH, 0);
	glMatrixMode(GL_MODELVIEW);

    if( m_caption[0] != '\0' )
    {
        m_fade = m_fade * 0.95f + 0.05f;
        m_fade = min( 1.0f, m_fade );
    }
    else
    {
        m_fade = m_fade * 0.95f;
    }

    float textBoxX = m_screenW/2-300;

    float sepulvedaH = 40;
    float sepulvedaW = sepulvedaH * 0.8f;
    float sepulvedaX = textBoxX - sepulvedaW;
    float sepulvedaY = m_screenH - sepulvedaH - 26;

    float textH = 12.0f;
    float textBoxW = 600;
    float textBoxY = sepulvedaY;
    float textBoxH = 40;

    //
    // Render background box

    float startingAlpha = m_fade * 0.75f;
    glColor4f( 0.1f, 0.1f, 0.5f, startingAlpha );

    glBegin( GL_QUADS );
        glVertex2i( textBoxX, textBoxY );
        glVertex2i( textBoxX+textBoxW, textBoxY);
        glVertex2i( textBoxX+textBoxW, textBoxY+textBoxH );
        glVertex2i( textBoxX, textBoxY+textBoxH );
    glEnd();


    //
    // Render sepulveda

    RenderFace( sepulvedaX, sepulvedaY, sepulvedaW, sepulvedaH, m_fade );


    //
    // Render border

    glColor4ub( 199, 214, 220, 255*m_fade );
    glLineWidth( 1.0f );
    glBegin( GL_LINE_LOOP );
        glVertex2i( sepulvedaX, textBoxY );
        glVertex2i( textBoxX+textBoxW, textBoxY);
        glVertex2i( textBoxX+textBoxW, textBoxY+textBoxH );
        glVertex2i( sepulvedaX, textBoxY+textBoxH );
    glEnd();
    glLineWidth( 1.0f );


    //
    // Current text appearing on screen

    float yPos = m_screenH - 56;
    float alpha = 1.0f;
    float textX = textBoxX+15;

    if( m_caption[0] != '\0' )
    {
        float textTime = CalculateTextTime();
        float finishTime = m_timeSync + textTime;
        float fractionDone = ( GetHighResTime() - m_timeSync ) / ( finishTime - m_timeSync );
        if( fractionDone < 0.0f ) fractionDone = 0.0f;
        if( fractionDone > 1.0f ) fractionDone = 1.0f;

        int totalChars = strlen( m_caption );
        int finishingChar = totalChars * fractionDone;
        int thisChar = 0;

        if( finishingChar != m_previousNumChars )
        {
            g_app->m_soundSystem->TriggerOtherEvent( NULL, "TextAppear", SoundSourceBlueprint::TypeSepulveda );
            m_previousNumChars = finishingChar;
        }

        LList<char *> *wrapped = WordWrapText( m_caption, textBoxW-15, g_editorFont.GetTextWidth(1,textH), true );

        for( int i = 0; i < wrapped->Size(); i++ )
        {
            char *thisLine = wrapped->GetData(i);

            if( thisChar + strlen(thisLine) < finishingChar )
            {
                g_gameFont.SetRenderOutline(true);
                glColor4f( m_fade, m_fade, m_fade, 0.0f );
                g_gameFont.DrawText2D( textX, yPos, textH, thisLine );
                glColor4f( 1.0f, 1.0f, 1.0f, m_fade );
                g_gameFont.SetRenderOutline(false);
                g_gameFont.DrawText2D( textX, yPos, textH, thisLine );
                yPos += textH;
            }
            else if( thisChar < finishingChar )
            {
                char thisLineCopy[512];
                strcpy( thisLineCopy, thisLine );
                int finishingCharIndex = ( finishingChar - thisChar );
                thisLineCopy[ finishingCharIndex ] = '\x0';
                g_gameFont.SetRenderOutline(true);
                glColor4f( m_fade, m_fade, m_fade, 0.0f );
                g_gameFont.DrawText2D( textX, yPos, textH, thisLineCopy );
                glColor4f( 1.0f, 1.0f, 1.0f, m_fade );
                g_gameFont.SetRenderOutline(false);
                g_gameFont.DrawText2D( textX, yPos, textH, thisLineCopy );
                yPos += textH;
            }

            thisChar += strlen(thisLine);
        }
        delete [] wrapped->GetData(0);
        delete wrapped;
    }

    //
    // Reset render matrices

    g_app->m_renderer->SetupMatricesFor3D();
    g_gameFont.EndText2D();
}


void Sepulveda::RenderFace( float _x, float _y, float _w, float _h, float _alpha )
{
	if ( m_picsFound == 0 ) { LoadAvatar("sepulveda"); }

    if( _alpha <= 0.0f ) return;

    char filename[256];
    static int previousPicIndex = 1;
    static int picIndex = 1;
    static float picTimer = 0.0f;
    static float zoomFactor = 0.0f;
    static float currentZoomFactor = 0.0f;
    static float offsetX = 0.0f;
    static float currentOffsetX = 0.0f;
    float frameTime = 0.4f;

    float timeNow = GetHighResTime();

    //
    // Time to chose a new pic?

    if( timeNow > picTimer + frameTime && _alpha > 0.9f )
    {
        previousPicIndex = picIndex;

		/*
        if( frand(6.0f) < 1.0f )        zoomFactor = frand(0.1f);
        if( frand(6.0f) < 1.0f )        offsetX = sfrand(0.1f);

        if      ( frand(10.0f) < 1.0f ) picIndex = 5;           // mouth open
        else if ( frand(10.0f) < 1.0f ) picIndex = 4;           // eyes shut
        else                            picIndex = 1 + ( darwiniaRandom() % 3 );
		*/
		picIndex = 1 + ( darwiniaRandom() % m_picsFound );
        picTimer = timeNow;
    }

    glEnable        ( GL_TEXTURE_2D );
    glDepthMask     ( false );


    float texX = 0;
    float texY = 0;
    float texW = 1;
    float texH = 1;

    currentZoomFactor = currentZoomFactor * ( 1.0f-(g_advanceTime*4) ) + (zoomFactor*g_advanceTime*4);
    currentOffsetX = currentOffsetX * ( 1.0f-(g_advanceTime*4) ) + (offsetX*g_advanceTime*4);
    currentOffsetX = min( currentOffsetX, currentZoomFactor );
    currentOffsetX = max( currentOffsetX, -currentZoomFactor );

    texX += currentZoomFactor;
    texX += currentOffsetX;
    texY += currentZoomFactor;
    texW -= currentZoomFactor * 2;
    texH -= currentZoomFactor * 2;

    //
    // Render current pic

    sprintf( filename, "sepulveda/%s%d.bmp", m_currentAvatar, picIndex );
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( filename, true, false ) );

    glColor4f( 1.0f, 1.0f, 1.0f, _alpha );
    glBegin( GL_QUADS );
        glTexCoord2f( texX, texY+texH );        glVertex2f( _x, _y );
        glTexCoord2f( texX+texW, texY+texH );   glVertex2f( _x+_w, _y);
        glTexCoord2f( texX+texW, texY );        glVertex2f( _x+_w, _y+_h);
        glTexCoord2f( texX, texY );             glVertex2f( _x, _y+_h);
    glEnd();


    //
    // Render previous pic if we are fading

    if( timeNow < picTimer + frameTime && _alpha > 0.75f )
    {
        float previousAlpha = _alpha;
        previousAlpha *= ( 1.0f - ( timeNow - picTimer ) / frameTime );
        sprintf( filename, "sepulveda/%s%d.bmp", m_currentAvatar, previousPicIndex );
        glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( filename ) );

        previousAlpha *= 0.75f;

        glColor4f( 1.0f, 1.0f, 1.0f, previousAlpha );
        glBegin( GL_QUADS );
            glTexCoord2f( texX, texY+texH );        glVertex2f( _x, _y );
            glTexCoord2f( texX+texW, texY+texH );   glVertex2f( _x+_w, _y);
            glTexCoord2f( texX+texW, texY );        glVertex2f( _x+_w, _y+_h);
            glTexCoord2f( texX, texY );             glVertex2f( _x, _y+_h);
        glEnd();
    }

    glDepthMask     ( true );
    glDisable       ( GL_TEXTURE_2D );


    //
    // Render white border

    glColor4f( 1.0f, 1.0f, 1.0f, _alpha );
    glLineWidth( 1.0f );
    glBegin( GL_LINE_LOOP );
        glVertex2f( _x, _y );
        glVertex2f( _x+_w, _y);
        glVertex2f( _x+_w, _y+_h);
        glVertex2f( _x, _y+_h);
    glEnd();
}


void Sepulveda::RenderTextBox()
{
    //
    // Setup render matrices

    g_gameFont.BeginText2D();

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

    float screenRatio = (float) g_app->m_renderer->ScreenW() / (float) g_app->m_renderer->ScreenH();
    float m_screenH = 600.0f;
    float m_screenW = m_screenH * screenRatio;

    gluOrtho2D(0, m_screenW, m_screenH, 0);
	glMatrixMode(GL_MODELVIEW);

    float textH = 12.0f;
    float textBoxW = 600;
    float textBoxX = m_screenW/2-textBoxW/2;
    textBoxX += 20;


    //
    // Render background box + sepulveda

    static float s_smoothHighestY = m_screenH;
    static float s_highestY = m_screenH;
    if( s_highestY < s_smoothHighestY )
    {
        s_smoothHighestY = s_highestY;
    }
    else
    {
        s_smoothHighestY = s_smoothHighestY * 0.95f + s_highestY * 0.05f;
    }

    float startingAlpha = 0.8f;
    if( m_caption[0] == '\0' && m_history.Size() ) startingAlpha *= GetCaptionAlpha(m_history[0]->m_timeSync);

    if( s_smoothHighestY < 579.0f )
    {
        glShadeModel( GL_SMOOTH );
        glBegin( GL_QUADS );
            glColor4f( 0.1f, 0.1f, 0.5f, startingAlpha );
            glVertex2i( textBoxX, 590 );
            glVertex2i( textBoxX+textBoxW, 590 );
            glColor4f( 0.1f, 0.1f, 0.5f, 0.0f );
            glVertex2i( textBoxX+textBoxW, s_smoothHighestY - 10 );
            glVertex2i( textBoxX, s_smoothHighestY - 10 );
        glEnd();
        glShadeModel( GL_FLAT );
    }

    s_highestY = m_screenH;


    //
    // Render sepulveda

    float sepulvedaH = 100;
    float sepulvedaW = sepulvedaH * 0.8f;
    float sepulvedaX = textBoxX - sepulvedaW - 10;
    float sepulvedaY = 590 - sepulvedaH;

    if( m_caption[0] != '\0' )
    {
        m_fade = m_fade * 0.95f + 0.05f;
        m_fade = min( 1.0f, m_fade );
    }
    else
    {
        m_fade = m_fade * 0.95f;
    }

    RenderFace( sepulvedaX, sepulvedaY, sepulvedaW, sepulvedaH, m_fade );


    //
    // Current text appearing on screen

    float yPos = 580.0f;
    float alpha = 1.0f;
    float textX = textBoxX+15;

    glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );

    yPos += m_scrollbarOffset;

    if( m_caption[0] != '\0' )
    {
        float textTime = CalculateTextTime();
        float finishTime = m_timeSync + textTime;
        float fractionDone = ( GetHighResTime() - m_timeSync ) / ( finishTime - m_timeSync );
        if( fractionDone < 0.0f ) fractionDone = 0.0f;
        if( fractionDone > 1.0f ) fractionDone = 1.0f;

        int totalChars = strlen( m_caption );
        int finishingChar = totalChars * fractionDone;
        int thisChar = totalChars;

        if( finishingChar != m_previousNumChars )
        {
            g_app->m_soundSystem->TriggerOtherEvent( NULL, "TextAppear", SoundSourceBlueprint::TypeSepulveda );
            m_previousNumChars = finishingChar;
        }

        LList<char *> *wrapped = WordWrapText( m_caption, textBoxW-15, g_editorFont.GetTextWidth(1,textH), true );

        for( int i = wrapped->Size()-1; i >= 0; --i )
        {
            char *thisLine = wrapped->GetData(i);
            thisChar -= strlen(thisLine);

            if( yPos > 450.0f && yPos < 570.0f+textH )
            {
                if( i == 0 )
                {
                    glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
                    g_gameFont.DrawText2D( textX-10, yPos, textH, ">" );
                }

                if( thisChar + strlen(thisLine) < finishingChar )
                {
                    g_gameFont.SetRenderOutline(true);
                    glColor4f( 1.0f, 1.0f, 1.0f, 0.0f );
                    g_gameFont.DrawText2D( textX, yPos, textH, thisLine );
                    g_gameFont.SetRenderOutline(false);
                    glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
                    g_gameFont.DrawText2D( textX, yPos, textH, thisLine );
                    yPos -= textH;
                }
                else if( thisChar < finishingChar )
                {
                    char thisLineCopy[512];
                    strcpy( thisLineCopy, thisLine );
                    int finishingCharIndex = ( finishingChar - thisChar );
                    thisLineCopy[ finishingCharIndex ] = '\x0';

                    g_gameFont.SetRenderOutline(true);
                    glColor4f( 1.0f, 1.0f, 1.0f, 0.0f );
                    g_gameFont.DrawText2D( textX, yPos, textH, thisLineCopy );
                    g_gameFont.SetRenderOutline(false);
                    glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
                    g_gameFont.DrawText2D( textX, yPos, textH, thisLineCopy );
                    yPos -= textH;
                }
            }
        }

        delete [] wrapped->GetData(0);
        delete wrapped;

        yPos -= textH/2.0f;
        alpha -= 0.1f;
    }


    //
    // Text history

    int firstItemIndex = 1;
	if ( m_caption[0] == '\0' ) firstItemIndex = 0;
    yPos += m_scrollbarOffset;
    for( int i = firstItemIndex; i < g_app->m_sepulveda->m_history.Size(); ++i )
    {
        SepulvedaCaption *caption = g_app->m_sepulveda->m_history[i];
        float alphaScale = GetCaptionAlpha( caption->m_timeSync );

        if( alpha*alphaScale>0.0f)
        {
			char buf[SEPULVEDA_MAX_PHRASE_LENGTH];
            BuildCaption(LANGUAGEPHRASE( caption->m_stringId ), buf);
			LList<char *> *wrapped = WordWrapText( buf, textBoxW-15, g_editorFont.GetTextWidth(1,textH) );
            bool rendered = false;

            for( int j = wrapped->Size()-1; j >= 0; --j )
            {
                char *thisLine = wrapped->GetData(j);
                if( yPos > 450.0f && yPos < 570.0f+textH )
                {
                    if( j == 0 )
                    {
                        glColor4f( 1.0f, 1.0f, 1.0f, alpha*alphaScale );
                        g_gameFont.DrawText2D( textX-10, yPos, textH, ">" );
                    }

                    float outlineAlpha = alpha * alphaScale;
                    outlineAlpha = max( outlineAlpha, 0.0f );

                    g_gameFont.SetRenderOutline(true);
                    glColor4f( alpha*alphaScale*outlineAlpha, alpha*alphaScale*outlineAlpha, alpha*alphaScale*outlineAlpha, 0.0f );
                    g_gameFont.DrawText2D( textX, yPos, textH, thisLine );
                    g_gameFont.SetRenderOutline(false);
                    glColor4f( 1.0f, 1.0f, 1.0f, alpha*alphaScale );
                    g_gameFont.DrawText2D( textX, yPos, textH, thisLine );

                    rendered = true;
                }
                yPos -= textH;
            }

            if( rendered ) alpha -= 0.2f;
            yPos -= textH/2.0f;

            delete [] wrapped->GetData(0);
            delete wrapped;
        }
    }

    s_highestY = yPos;
    s_highestY = max( s_highestY, 440 );

    glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
    if( m_msgQueue.Size() > 0 )
    {
        char theString[256];
        sprintf( theString, "%s (%d)", LANGUAGEPHRASE("sepulveda_pressreturn"), m_msgQueue.Size() );
        g_gameFont.DrawText2DRight( textBoxX+textBoxW, 590.0f, 7.0f, theString );
    }
    else if( m_caption[0] != '\0' )
    {
        g_gameFont.DrawText2DRight( textBoxX+textBoxW, 590.0f, 7.0f, LANGUAGEPHRASE("sepulveda_pressreturn"), m_msgQueue.Size() );
    }

    //
    // Reset render matrices

    g_app->m_renderer->SetupMatricesFor3D();
    g_gameFont.EndText2D();

}


void Sepulveda::Render()
{
    if( g_app->m_editing ) return;

    START_PROFILE( g_app->m_profiler, "Render Sepulveda" );

	if( m_gestureDemo )
	{
		RenderGestureDemo();
	}


    RenderHighlights();

    if( !g_app->m_taskManagerInterface->m_visible )
    {
        bool cutsceneModeDesired = g_app->m_camera->IsInMode( Camera::ModeBuildingFocus ) ||
                                   g_app->m_camera->IsInMode( Camera::ModeMoveToTarget ) ||
                                   g_app->m_camera->IsInMode( Camera::ModeDoNothing ) ||
                                   g_app->m_camera->IsInMode( Camera::ModeSphereWorldScripted ) ||
                                   g_app->m_camera->IsInMode( Camera::ModeSphereWorldOutro ) ||
                                   g_app->m_camera->IsInMode( Camera::ModeSphereWorldFocus );

                                   /*||g_app->m_tutorial*/


//#ifdef DEMO2
//        if( cutsceneModeDesired &&
//            g_app->m_tutorial &&
//            g_app->m_tutorial->IsRunning() )
//        {
//            cutsceneModeDesired = false;
//        }
//#endif

        if( cutsceneModeDesired )
        {
            RenderTextBoxCutsceneMode();
            m_cutsceneMode = true;
        }
        else
        {
            if( m_cutsceneMode && m_fade > 0.01f )
            {
                RenderTextBoxCutsceneMode();
            }
            else
            {
                m_cutsceneMode = false;
                RenderTextBox();

                bool chatLog = g_app->m_sepulveda->ChatLogVisible();
                if( chatLog )
                {
                    RenderScrollBar();
                }
                else
                {
                    m_scrollbarOffset = 0.0f;
                }
            }
        }
    }
    else
    {
        RenderTextBoxTaskManagerMode();
    }


    END_PROFILE( g_app->m_profiler, "Render Sepulveda" );
}


void Sepulveda::RenderScrollBar()
{
    float screenRatio = (float) g_app->m_renderer->ScreenW() / (float) g_app->m_renderer->ScreenH();
    float m_screenH = 600.0f;
    float m_screenW = m_screenH * screenRatio;

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
    gluOrtho2D(0, m_screenW, m_screenH, 0);
	glMatrixMode(GL_MODELVIEW);

    float textH = 12.0f;
    float textBoxW = 600;
    float textBoxX = m_screenW/2-textBoxW/2;
    float textBoxH = 100;
    textBoxX += 20.0f;

    float scrollX = textBoxX + textBoxW;
    float scrollY = 580.0f - textBoxH;
    float scrollW = 20.0f;
    float scrollH = 110;

    float mouseX = g_target->X();
    float mouseY = g_target->Y();
    float fractionX = mouseX / (float) g_app->m_renderer->ScreenW();
    float fractionY = mouseY / (float) g_app->m_renderer->ScreenH();
    mouseX = m_screenW * fractionX;
    mouseY = m_screenH * fractionY;

    glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
    glDisable( GL_CULL_FACE );
    glEnable( GL_BLEND );

    //
    // Scroll

    glEnable( GL_TEXTURE_2D );
    glBindTexture( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "icons/scrollbar.bmp" ) );

    glColor4f( 0.7f, 0.7f, 1.0f, 0.85f );
    glBegin( GL_QUADS );
        glTexCoord2i(0,0);      glVertex2f( scrollX, scrollY );
        glTexCoord2i(1,0);      glVertex2f( scrollX+scrollW, scrollY );
        glTexCoord2i(1,1);      glVertex2f( scrollX+scrollW, scrollY+scrollH );
        glTexCoord2i(0,1);      glVertex2f( scrollX, scrollY+scrollH );
    glEnd();

    glDisable( GL_TEXTURE_2D );

    glColor4f( 1.0f, 1.0f, 1.0f, 0.25f );
    glBegin( GL_LINE_LOOP );
        glVertex2f( scrollX, scrollY );
        glVertex2f( scrollX+scrollW, scrollY );
        glVertex2f( scrollX+scrollW, scrollY+scrollH );
        glVertex2f( scrollX, scrollY+scrollH );
    glEnd();


    if( mouseX > scrollX && mouseX < scrollX+scrollW &&
        mouseY > scrollY && mouseY < scrollY+scrollH &&
            g_inputManager->controlEvent( ControlEclipseLMousePressed ) ) // TODO: Is this right?
    {
        float midPoint = scrollY + scrollH * 0.5f;
        float scrollDir = midPoint - mouseY;
        m_scrollbarOffset += g_advanceTime * scrollDir * 3;
        m_scrollbarOffset = max( m_scrollbarOffset, 0 );
    }



    glEnable( GL_CULL_FACE );

    g_app->m_renderer->SetupMatricesFor3D();
}


SepulvedaHighlight::SepulvedaHighlight( char *_name )
:   m_radius(0.0f),
    m_alpha(0.0f),
    m_ended(false),
    m_name(NULL)
{
    m_name = NewStr( _name );
}


SepulvedaHighlight::~SepulvedaHighlight()
{
    delete [] m_name;
}


void SepulvedaHighlight::Start( Vector3 const &_pos, float _radius )
{
    m_pos = _pos;
    m_radius = _radius;
    m_ended = false;
    m_alpha = 0.0f;
}


void SepulvedaHighlight::Stop()
{
    m_ended = true;
}


bool SepulvedaHighlight::Advance ()
{
    if( !m_ended && m_alpha < 1.0f )
    {
        m_alpha += 0.02f;
        if( m_alpha > 1.0f ) m_alpha = 1.0f;
    }
    else if( m_ended && m_alpha > 0.0f )
    {
        m_alpha -= 0.02f;
        if( m_alpha < 0.0f )
        {
            return true;
        }
    }

    return false;
}


void SepulvedaHighlight::Render()
{
    Vector3 ourPos = m_pos + Vector3(0,1500,1000);
    Vector3 theirPos = m_pos - Vector3(0,200,133);

    Vector3 camToTheirPos = g_app->m_camera->GetPos() - theirPos;
    Vector3 lineTheirPos = camToTheirPos ^ ( ourPos - theirPos );
    lineTheirPos.SetLength( m_radius * 0.5f );

    for( int i = 0; i < 3; ++i )
    {
        Vector3 pos = theirPos;
        pos.x += sinf( g_gameTime + i ) * 10;
        pos.z += cosf( g_gameTime + i ) * 10;

        float blue = 0.5f + fabs( sinf(g_gameTime * i) ) * 0.5f;

        glColor4f( 1.0f, 1.0f, blue, m_alpha*0.5f );

        glBegin( GL_QUADS );
            glTexCoord2i(1,0);      glVertex3fv( (ourPos - lineTheirPos).GetData() );
            glTexCoord2i(1,1);      glVertex3fv( (ourPos + lineTheirPos).GetData() );
            glTexCoord2i(0,1);      glVertex3fv( (pos + lineTheirPos).GetData() );
            glTexCoord2i(0,0);      glVertex3fv( (pos - lineTheirPos).GetData() );
        glEnd();
    }
}

bool Sepulveda::ChatLogVisible()
{
	if( EclGetWindows()->Size() > 0 )
	{
		return false;
	}

    if( g_prefsManager->GetInt("ControlMethod") == 1 &&
        g_inputManager->controlEvent( ControlIconsChatLog ) )
    {
        return true;
    }

    if( g_prefsManager->GetInt("ControlMethod") == 0 &&
        g_inputManager->controlEvent( ControlGesturesChatLog ) )
    {
        return true;
    }

    return false;


}
