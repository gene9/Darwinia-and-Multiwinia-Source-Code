#include "lib/universal_include.h"
#include "lib/input/input.h"
#include "lib/text_renderer.h"
#include "lib/hi_res_time.h"
#include "lib/resource.h"
#include "lib/window_manager.h"
#include "lib/language_table.h"

#include "loaders/credits_loader.h"

#include "app.h"
#include "renderer.h"

#include "sound/soundsystem.h"
#include "sound/sound_library_2d.h"



CreditsLoader *g_creditsLoader = NULL;


CreditsLoader::CreditsLoader()
:   Loader()
{
    g_creditsLoader = this;
}


void CreditsLoader::GenerateCharsCentre( float _x, float _y, float _size, char *_string )
{
    float texWidth = g_gameFont.GetTextWidth( strlen(_string), _size );

    GenerateChars( _x - texWidth/2.0f, _y, _size, _string );
}


int CreditsLoader::AddString( float _x, float _y, float _size, char *_string )
{
    //
    // Look for existing item

    CreditsItem *item = NULL;

    int itemId = -1;
    for( int j = 0; j < m_items.Size(); ++j )
    {
        if( m_items.ValidIndex(j) )
        {
            CreditsItem *thisItem = m_items[j];
            if( strcmp(thisItem->m_string, _string) == 0 &&
                /*thisItem->m_size == _size &&*/
                !thisItem->m_used )
            {
                item = thisItem;
                itemId = j;
                item->m_used = true;
                break;
            }
        }
    }

    //
    // Create new item if required

    if( !item )
    {        
        item = new CreditsItem();
        item->SetString( _string );
        item->m_size = _size;
        item->m_used = true;
        item->m_pos = GenerateOffscreenPosition( 1000.0f + (darwiniaRandom() % 200) );        
        itemId = m_items.PutData( item );
    }

    float texW = g_gameFont.GetTextWidth( strlen(_string), _size );
    item->m_targetPos.Set( _x + texW/2.0f, _y );

    return itemId;
}


void CreditsLoader::GenerateWords( float _x, float _y, float _size, char *_string )
{
    char stringCopy[256];
    strcpy( stringCopy, _string );
    
    char *copyPos = stringCopy;
    float x = _x;

    while( true )
    {
        char *space = strchr( copyPos, ' ' );
        if( space ) *space = '\x0';

        int itemId = AddString( x, _y, _size, copyPos );
        CreditsItem *item = m_items[itemId];

        CreditsDarwinian *darwinian = new CreditsDarwinian();
        
        if( IsOffscreen(item->m_pos) )
        {
            darwinian->m_pos = item->m_pos;
        }
        else
        {
            darwinian->m_pos = GenerateOffscreenPosition( 200.0f + (darwiniaRandom() % 100) );
        }
        darwinian->m_itemId = itemId;
        darwinian->m_state = CreditsDarwinian::StateWalkingToItem;
        m_darwinians.PutData( darwinian );

        if( !space ) break;
        
        x += g_gameFont.GetTextWidth( strlen(copyPos)+1, _size );
        copyPos = (space+1);
    }
}


void CreditsLoader::GenerateWordsCentre( float _x, float _y, float _size, char *_string )
{
    float texWidth = g_gameFont.GetTextWidth( strlen(_string), _size );
    GenerateWords( _x - texWidth/2.0f, _y, _size, _string );
}


void CreditsLoader::GenerateChars( float _x, float _y, float _size, char *_string )
{
    float x = _x;

    for( int i = 0; i < strlen(_string); ++i )    
    {
        char thisChar = _string[i];
        if( thisChar != ' ' )
        {
            char buffer[64];
            sprintf( buffer, "%c", thisChar);

            int itemId = AddString( x, _y, _size, buffer );
            CreditsItem *item = m_items[itemId];
            CreditsDarwinian *darwinian = new CreditsDarwinian();
            
            if( IsOffscreen(item->m_pos) )
            {
                darwinian->m_pos = item->m_pos;
            }
            else
            {
                darwinian->m_pos = GenerateOffscreenPosition( 200.0f + (darwiniaRandom() % 100) );
            }
            darwinian->m_itemId = itemId;
            darwinian->m_state = CreditsDarwinian::StateWalkingToItem;
            m_darwinians.PutData( darwinian );
        }

        x += g_gameFont.GetTextWidth( 1, _size );
    }
}


void CreditsLoader::RemoveUnusedStrings()
{
    for( int i = 0; i < m_items.Size(); ++i )
    {
        if( m_items.ValidIndex(i) )
        {
            CreditsItem *item = m_items[i];
            if( !item->m_used )
            {
                item->m_targetPos = GenerateOffscreenPosition( 150 );

                CreditsDarwinian *darwinian = new CreditsDarwinian();
                darwinian->m_pos = GenerateOffscreenPosition( 40 + (darwiniaRandom() % 50) );
                darwinian->m_itemId = i;
                darwinian->m_state = CreditsDarwinian::StateWalkingToItem;
                m_darwinians.PutData( darwinian );
            }
        }
    }
}


Vector2 CreditsLoader::GenerateOffscreenPosition( float _distanceOut )
{
    Vector2 result;
    
    switch( darwiniaRandom() % 4 )
    {
        case 0 :    result.Set( -1 * _distanceOut, darwiniaRandom() % 600 );         break;
        case 1 :    result.Set( darwiniaRandom() % 800, -1 * _distanceOut );         break;
        case 2 :    result.Set( 800 + _distanceOut, darwiniaRandom() % 600 );        break;
        case 3 :    result.Set( darwiniaRandom() % 800, 600 + _distanceOut );        break;
    }

    return result;
}


bool CreditsLoader::IsOffscreen( Vector2 const &_pos )
{
    if( _pos.x < 0.0f ) return true;
    if( _pos.y < 0.0f ) return true;
    if( _pos.x > 800.0f ) return true;
    if( _pos.y > 600.0f ) return true;

    return false;
}


void CreditsLoader::Advance( float _time )
{
    for( int i = 0; i < m_darwinians.Size(); ++i )
    {
        if( m_darwinians.ValidIndex(i) )
        {
            CreditsDarwinian *darwinian = m_darwinians[i];
            bool amIDone = darwinian->Advance( _time );
            if( amIDone )
            {
                m_darwinians.MarkNotUsed( i );
                delete darwinian;
            }
        }
    }
}


void CreditsLoader::RenderDarwinian( float _x, float _y )
{
    float _size = 30.0f;

    glColor3ub(100,255,100);

    glEnable( GL_TEXTURE_2D );
    glBindTexture( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "sprites/darwinian.bmp" ) );

    glBegin( GL_QUADS );
        glTexCoord2i(0,1);      glVertex2f( _x - _size/2.0f, _y - _size/2.0f );
        glTexCoord2i(1,1);      glVertex2f( _x + _size/2.0f, _y - _size/2.0f );
        glTexCoord2i(1,0);      glVertex2f( _x + _size/2.0f, _y + _size/2.0f );
        glTexCoord2i(0,0);      glVertex2f( _x - _size/2.0f, _y + _size/2.0f );
    glEnd();

    glDisable( GL_TEXTURE_2D );
}


void CreditsLoader::Render()
{
    for( int i = 0; i < m_darwinians.Size(); ++i )
    {
        if( m_darwinians.ValidIndex(i) )
        {
            CreditsDarwinian *darwinian = m_darwinians[i];
            RenderDarwinian( darwinian->m_pos.x, darwinian->m_pos.y );
        }
    }
    
    for( int i = 0; i < m_items.Size(); ++i )
    {
        if( m_items.ValidIndex(i) )
        {
            CreditsItem *item = m_items[i];

            glColor4f( 1.0f, 1.0f, 1.0f, 0.05f );
            //g_gameFont.DrawText2D( item->m_targetPos.x, item->m_targetPos.y, item->m_size, item->m_string );            

            glColor4f( 1.0f, 1.0f, 1.0f, 0.0f );
            g_gameFont.SetRenderOutline(true);
            g_gameFont.DrawText2DCentre( item->m_pos.x, item->m_pos.y, item->m_size, item->m_string );            
            
            glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
            g_gameFont.SetRenderOutline(false);
            g_gameFont.DrawText2DCentre( item->m_pos.x, item->m_pos.y, item->m_size, item->m_string );            
        }
    }    
}


bool CreditsLoader::CreateNextScene()
{
    DeleteUnusedStrings();
    
    switch( m_sceneIndex )
    {
        case 0:
            GenerateWordsCentre( 400, 150, 30, LANGUAGEPHRASE("bootloader_credits_1") );
            GenerateCharsCentre( 400, 250, 40, LANGUAGEPHRASE("bootloader_credits_2") );
            GenerateCharsCentre( 400, 300, 40, LANGUAGEPHRASE("bootloader_credits_3") );
            break;

        case 1:
            GenerateWordsCentre( 400, 150, 30, LANGUAGEPHRASE("bootloader_credits_4") );
            GenerateCharsCentre( 400, 200, 50, LANGUAGEPHRASE("bootloader_credits_5") );
            GenerateCharsCentre( 400, 300, 40, LANGUAGEPHRASE("bootloader_credits_6") );
            GenerateCharsCentre( 400, 350, 40, LANGUAGEPHRASE("bootloader_credits_7") );
            GenerateCharsCentre( 400, 400, 40, LANGUAGEPHRASE("bootloader_credits_8") );
            GenerateCharsCentre( 400, 450, 40, LANGUAGEPHRASE("bootloader_credits_9") );
            break;

        case 2:
            GenerateWordsCentre( 400, 150, 30, LANGUAGEPHRASE("bootloader_credits_41") );
            GenerateCharsCentre( 400, 200, 40, LANGUAGEPHRASE("bootloader_credits_42") );
            GenerateCharsCentre( 400, 250, 40, LANGUAGEPHRASE("bootloader_credits_43") );
            GenerateCharsCentre( 400, 300, 40, LANGUAGEPHRASE("bootloader_credits_44") );
            break;            

        case 3:
            GenerateWordsCentre( 400, 150, 30, LANGUAGEPHRASE("bootloader_credits_10") );
            GenerateCharsCentre( 400, 250, 40, LANGUAGEPHRASE("bootloader_credits_11") );
            GenerateCharsCentre( 400, 300, 40, LANGUAGEPHRASE("bootloader_credits_12") );
            break;

        case 4:
            GenerateWordsCentre( 400, 150, 30, LANGUAGEPHRASE("bootloader_credits_13") );
            GenerateCharsCentre( 400, 250, 50, LANGUAGEPHRASE("bootloader_credits_14") );
            break;

        case 5:
            GenerateWordsCentre( 400, 150, 30, LANGUAGEPHRASE("bootloader_credits_16") );
            GenerateCharsCentre( 400, 200, 40, LANGUAGEPHRASE("bootloader_credits_17") );
            GenerateCharsCentre( 400, 270, 40, LANGUAGEPHRASE("bootloader_credits_18") );
            GenerateWordsCentre( 400, 400, 30, LANGUAGEPHRASE("bootloader_credits_19") );
            GenerateCharsCentre( 400, 450, 40, LANGUAGEPHRASE("bootloader_credits_20") );
            break;

        case 6:
            GenerateWordsCentre( 400, 150, 30, LANGUAGEPHRASE("bootloader_credits_21") );
            GenerateCharsCentre( 400, 250, 50, LANGUAGEPHRASE("bootloader_credits_22") );
            GenerateWordsCentre( 400, 300, 30, LANGUAGEPHRASE("bootloader_credits_23") );
            GenerateCharsCentre( 400, 400, 50, LANGUAGEPHRASE("bootloader_credits_24") );
            GenerateWordsCentre( 400, 450, 30, LANGUAGEPHRASE("bootloader_credits_25") );
            break;

        case 7:
            GenerateWordsCentre( 400, 150, 30, LANGUAGEPHRASE("bootloader_credits_26") );
            GenerateCharsCentre( 400, 200, 40, LANGUAGEPHRASE("bootloader_credits_27") );
            GenerateCharsCentre( 400, 250, 40, LANGUAGEPHRASE("bootloader_credits_28") );
            GenerateWordsCentre( 400, 400, 30, LANGUAGEPHRASE("bootloader_credits_29") );
            GenerateCharsCentre( 400, 450, 40, LANGUAGEPHRASE("bootloader_credits_30") );
            break;

        case 8:
            GenerateWordsCentre( 400, 150, 30, LANGUAGEPHRASE("bootloader_credits_31") );
            GenerateCharsCentre( 400, 250, 40, LANGUAGEPHRASE("bootloader_credits_32") );
            break;


        case 9:
            GenerateWordsCentre( 400, 150, 30, LANGUAGEPHRASE("bootloader_credits_33") );
            GenerateCharsCentre( 400, 200, 40, LANGUAGEPHRASE("bootloader_credits_34") );
            GenerateWordsCentre( 400, 350, 30, LANGUAGEPHRASE("bootloader_credits_35") );
            GenerateCharsCentre( 400, 400, 40, LANGUAGEPHRASE("bootloader_credits_36") );
            break;
            
        case 10:
            GenerateWordsCentre( 400, 50, 30, LANGUAGEPHRASE("bootloader_credits_37") );
            GenerateBetaTesterList();
            break;

        case 11:
            GenerateCharsCentre( 400, 200, 60, LANGUAGEPHRASE("bootloader_credits_38") );
            GenerateWordsCentre( 400, 300, 20, LANGUAGEPHRASE("bootloader_credits_39") );
            GenerateWordsCentre( 400, 330, 20, LANGUAGEPHRASE("bootloader_credits_40") );
            break;
            
        case 12:
            break;

        case 13: 
            return true;

    }
    
    RemoveUnusedStrings();
    ++m_sceneIndex;
    return false;
}


void CreditsLoader::GenerateEGamesList()
{
    TextReader *reader = g_app->m_resource->GetTextReader( "egames.txt" );

    //
    // Load the names from the file

    float yPos = 30;

    while( reader && reader->ReadLine() )
    {
        int size = atoi(reader->GetNextToken());
        char *thisName = reader->GetRestOfLine();
        if( strlen(thisName) > 1 )
        {
            GenerateWordsCentre( 400, yPos, size, thisName );
            yPos += size * 1.3f;
        }
        else
        {
            yPos += 20;
        }
    }

    delete reader;
}


void CreditsLoader::GenerateBetaTesterList()
{
    TextReader *reader = g_app->m_resource->GetTextReader( "betatesters.txt" );

    //
    // Load the names from the file

    LList<char *> names;

    while( reader && reader->ReadLine() )
    {
        char *thisName = reader->GetRestOfLine();
        if( strlen(thisName) > 1 )
        {
            char *nameDup = strdup( thisName );        
            names.PutData( nameDup );
        }
    }

    delete reader;

    //
    // Display the names

    int x = 200;
    int y = 75;

    float screenRatio = (float) g_app->m_renderer->ScreenH() / (float) g_app->m_renderer->ScreenW();
    int screenH = 800 * screenRatio;

    int numColumns = 2;
    int numWordsPerColumn = names.Size() / (float) numColumns;
    numWordsPerColumn --;
    
    float gapSize = ( screenH - (y+40) ) / numWordsPerColumn;
    float textSize = gapSize * 2.0f/3.0f;
    
    for( int i = 0; i < names.Size(); ++i )
    {
        GenerateWordsCentre( x, y+=gapSize, textSize, names[i] );
        
        if( i == numWordsPerColumn )
        {
            x += 400;
            y = 75;
        }
    }
}


void CreditsLoader::DeleteUnusedStrings()
{
    for( int i = 0; i < m_items.Size(); ++i )
    {
        if( m_items.ValidIndex(i) )
        {
            CreditsItem *item = m_items[i];
            if( !item->m_used && IsOffscreen( item->m_pos ) )
            {
                m_items.MarkNotUsed(i);
                delete item;
            }
        }
    }
}


void CreditsLoader::FlipBuffers( float _alpha )
{
	g_inputManager->PollForEvents(); 
	g_inputManager->Advance();
	glFinish();
	g_windowManager->Flip();
	glClear	(GL_DEPTH_BUFFER_BIT);

    float screenRatio = (float) g_app->m_renderer->ScreenH() / (float) g_app->m_renderer->ScreenW();
    int screenH = 800 * screenRatio;

    glColor4f( 0.0f, 0.0f, 0.0f, _alpha );
    glBegin( GL_QUADS );
        glVertex2i(0,0);
        glVertex2i(800,0);
        glVertex2i(800,screenH);
        glVertex2i(0,screenH);
    glEnd();
    
    //g_app->m_soundSystem->Advance();
    //g_soundLibrary2d->TopupBuffer();    

	Sleep(1);
}

 
void CreditsLoader::Run()
{
    g_app->m_soundSystem->TriggerOtherEvent( NULL, "Credits", SoundSourceBlueprint::TypeMusic );

    m_sceneIndex = 0;
    int screenH = SetupFor2D( 800 );

    float time = GetHighResTime();
    float startTime = time;

    while( !g_inputManager->controlEvent( ControlSkipMessage ) )
    {
        if( g_app->m_requestQuit ) break;
        if( m_darwinians.NumUsed() == 0 )
        {
            bool amIDone = CreateNextScene();
            if( amIDone ) break;
        }

        float newTime = GetHighResTime();
        Advance( newTime - time );
        time = newTime;
        Render();

        float alpha = (time - startTime) / 7.0f;
        alpha = min( alpha, 1.0f );
        FlipBuffers( alpha );
        AdvanceSound();
    }

    g_app->m_soundSystem->StopAllSounds( WorldObjectId(), "Music Cutscene3" );
}


bool CreditsDarwinian::AdvanceToTargetPos( float _time )
{
    Vector2 xVector = (m_targetPos - m_pos);
    xVector.y *= 0.3f;
    float distanceX = xVector.Mag();
    if( distanceX > 10.0f )
    {
        xVector.SetLength( _time * 150.0f );
        m_pos += xVector;
        return false;
    }
    else
    {
        xVector.SetLength( _time * 150.0f * distanceX / 10.0f );
        m_pos += xVector;
        if( distanceX > 2.0f ) return false;
    }

    Vector2 yVector = (m_targetPos - m_pos);
    yVector.x *= 0.3f;
    float distanceY = yVector.Mag();
    if( distanceY > 10.0f )
    {
        yVector.SetLength( _time * 150.0f );
        m_pos += yVector;
        return false;
    }
    else
    {
        yVector.SetLength( _time * 150.0f * distanceY / 10.0f );
        m_pos += yVector;
        if( distanceY > 2.0f ) return false;
    }

    return true;
}


bool CreditsDarwinian::Advance( float _time )
{           
    switch( m_state )
    {
        case StateWalkingToItem:
        {
            if( !g_creditsLoader->m_items.ValidIndex( m_itemId ) )
            {
                m_state = StateLeaving;
                m_targetPos = g_creditsLoader->GenerateOffscreenPosition( 30 );
                break;
            }
            CreditsItem *item = g_creditsLoader->m_items[ m_itemId ];
            m_targetPos = item->m_pos + Vector2(0,item->m_size) * 0.5f;

            bool arrived = AdvanceToTargetPos(_time);
            if( arrived )
            {
                m_state = StateCarryingItem;
            }
            break;
        }
        case StateCarryingItem:
        {
            if( !g_creditsLoader->m_items.ValidIndex( m_itemId ) )
            {
                m_state = StateLeaving;
                m_targetPos = g_creditsLoader->GenerateOffscreenPosition( 30 );
                break;
            }
            CreditsItem *item = g_creditsLoader->m_items[ m_itemId ];
            m_targetPos = item->m_targetPos + Vector2(0,item->m_size)*0.5f;
            bool arrived = AdvanceToTargetPos( _time );
            item->m_pos = m_pos - Vector2(0,item->m_size) * 0.5f;

            if( arrived )
            {
                item->m_used = false;
                if( g_creditsLoader->IsOffscreen( m_pos ) ) return true;
                m_state = StateLeaving;
                m_targetPos = g_creditsLoader->GenerateOffscreenPosition( 100 );
            }
            break;
        }

        case StateLeaving:
        {
            bool arrived = AdvanceToTargetPos( _time );
            return arrived;
            break;
        }
    }

    return false;
}
