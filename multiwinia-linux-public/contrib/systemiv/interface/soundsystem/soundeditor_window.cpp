#include "lib/universal_include.h"
#include "lib/sound/sound_blueprint_manager.h"
#include "lib/sound/sound_instance.h"
#include "lib/sound/soundsystem.h"
#include "lib/sound/sound_library_3d.h"
#include "lib/gucci/input.h"
#include "lib/filesys/filesys_utils.h"
#include "lib/render/renderer.h"
#include "lib/debug_utils.h"

#include "interface/components/drop_down_menu.h"
#include "interface/components/filedialog.h"
#include "interface/components/inputfield.h"
#include "interface/components/scrollbar.h"

#include "soundeditor_window.h"
#include "soundparameter_window.h"


class ObjectTypeMenu : public DropDownMenu
{
public:
	ObjectTypeMenu()
	:	DropDownMenu(true)
	{
	}

    void SelectOption( int _option )
    {
        DropDownMenu::SelectOption( _option );

        if( _option != -1 )
        {
            SoundEditorWindow *sew = (SoundEditorWindow *) m_parent;
            sew->SelectEvent( -1 );
            
            EclButton *button = m_parent->GetButton( "New Event" );
            AppDebugAssert( button );
        
            DropDownMenu *menu = (DropDownMenu *) button;
            menu->Empty();

            char *objectType = (char *) GetSelectionName();
            LList<char *> allevents;
            g_soundSystem->m_interface->ListObjectEvents( &allevents, objectType );
            
            for( int i = 0; i < allevents.Size(); ++i )
            {
                menu->AddOption( allevents[i] );
            }
        }
    }
};


class NewSoundEventMenu : public DropDownMenu
{
public:
    void SelectOption( int _value )
    {
        DropDownMenu::SelectOption( _value );
		int optionIndex = FindValue(_value);

        if( optionIndex != -1 )
        {
            SoundEditorWindow *sew = (SoundEditorWindow *) m_parent;
            SoundEventBlueprint *ssb = sew->GetSoundEventBlueprint();
            if( ssb )
            {
				char eventName[128];
				sew->GetSelectionName(eventName);
				strcat(eventName, " ");
				strcat(eventName, m_options[optionIndex]->m_word);

                SoundInstanceBlueprint *event = new SoundInstanceBlueprint();
                strcpy( event->m_eventName, m_options[optionIndex]->m_word );
                event->m_instance = new SoundInstance();
				event->m_instance->SetEventName(eventName, m_options[optionIndex]->m_word);
                ssb->m_events.PutDataAtEnd( event );
                int eventIndex = ssb->m_events.Size() - 1;
                sew->SelectEvent( eventIndex );
            }
            SelectOption( -1 );
        }
    }
};


class SoundEventButton : public InterfaceButton
{
public:
    int m_eventIndex;

public:
	SoundEventButton(): InterfaceButton()
	{
	}

    void Render( int realX, int realY, bool highlighted, bool clicked )
    {
        SoundEditorWindow *sew = (SoundEditorWindow *) m_parent;
        SoundEventBlueprint *ssb = sew->GetSoundEventBlueprint();
        if( ssb && ssb->m_events.ValidIndex(m_eventIndex) )
        {
            SoundInstanceBlueprint *seb = ssb->m_events[m_eventIndex];
            SetCaption( seb->m_eventName, false );

            if( sew->m_eventIndex == m_eventIndex )
            {
                InterfaceButton::Render( realX, realY, true, clicked );
            }
            else
            {
                InterfaceButton::Render( realX, realY, highlighted, clicked );
            }
        }
    }

    void MouseUp()
    {
        SoundEditorWindow *sew = (SoundEditorWindow *) m_parent;        
        sew->SelectEvent( m_eventIndex );
    }
};


class SoundPositionButton : public InterfaceButton
{
public:
    void Render( int realX, int realY, bool highlighted, bool clicked )
    {
        InterfaceButton::Render( realX, realY, false, false );
        
        //
        // Render guidelines

        glColor4f( 1.0f, 1.0f, 1.0f, 0.5f );
        glBegin( GL_LINES );
            glVertex2i( realX + 10, realY + m_h/2 );
            glVertex2i( realX + m_w - 10, realY + m_h/2 );

            glVertex2i( realX + m_w/2, realY + 10 );
            glVertex2i( realX + m_w/2, realY + m_h - 10 );
        glEnd();

        
        //
        // Render sound position

        Vector3<float> soundPos = g_soundSystem->m_editorPos;

        float midX = realX + m_w/2.0f;
        float midY = realY + m_h/2.0f;

        float scale = 5.0f;
        float size = 5.0f;

        glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
        glBegin( GL_LINE_LOOP );
            glVertex2f( midX + soundPos.x/scale - size, midY + soundPos.z/scale - size );
            glVertex2f( midX + soundPos.x/scale + size, midY + soundPos.z/scale - size );
            glVertex2f( midX + soundPos.x/scale + size, midY + soundPos.z/scale + size );
            glVertex2f( midX + soundPos.x/scale - size, midY + soundPos.z/scale + size );
        glEnd();


        //
        // Handle mouse input

        float posX = soundPos.x;
        float posZ = soundPos.z;

		bool mousePressed = g_inputManager->m_lmb;
        if( mousePressed && EclMouseInButton(m_parent,this) )                    
        {
            posX = ( g_inputManager->m_mouseX - midX ) * 5.0f;
            posZ = ( g_inputManager->m_mouseY - midY ) * 5.0f;
        }


        //  BUGFIX
        //  In the sound editor, when make a sample play looped, then stop it and then 
        //  click play or looped again no sound comes out. 
        //  This only occurs if the position is exactly on the camera
        if( posX == 0.0f && posZ == 0.0f ) posZ = 0.01f;

        g_soundSystem->m_editorPos.Set( posX, 0.01f, posZ );
    }
};


class PlayButton : public InterfaceButton
{
public:
    void MouseUp()
    {
        SoundEditorWindow *sew = (SoundEditorWindow *) m_parent;
        sew->StartPlayback(false);
    }
};


class PlayRepeatButton : public InterfaceButton
{
public:
    void MouseUp()
    {
        SoundEditorWindow *sew = (SoundEditorWindow *) m_parent;
        sew->StartPlayback(true);
    }
};


class StopButton : public InterfaceButton
{
public:
    void MouseUp()
    {
        SoundEditorWindow *sew = (SoundEditorWindow *) m_parent;
        sew->StopPlayback();
    }
};


class SaveAllButton : public InterfaceButton
{
public:
    void MouseUp()
    {
        g_soundSystem->m_blueprints.SaveBlueprints();
    }
};


class DeleteEventButton : public InterfaceButton
{
public:
    void MouseUp()
    {
        SoundEditorWindow *sew = (SoundEditorWindow *) m_parent;
        SoundEventBlueprint *ssb = sew->GetSoundEventBlueprint();
        if( ssb->m_events.ValidIndex( sew->m_eventIndex ) )
        {
            sew->StopPlayback();
            SoundInstanceBlueprint *seb = ssb->m_events[ sew->m_eventIndex ];
            ssb->m_events.RemoveData( sew->m_eventIndex );
            delete seb;
            sew->SelectEvent( -1 );
        }
    }
};


class SoundSelector : public FileDialog
{
public:
    SoundSelector( char *_name )
        :   FileDialog( _name )
    {
    }

    void FileSelected( char *_filename )
    {
        char const *extensionRemoved = RemoveExtension( _filename );
//		char const *err = g_soundSystem->m_blueprints.IsSoundSourceOK(extensionRemoved);
//		if (err)
//		{
//			//MessageDialog *win = new MessageDialog("Alert", err);
//			//EclRegisterWindow(win);
//			return;
//		}

        EclWindow *window = EclGetWindow( m_parent );
        SoundEditorWindow *sew = (SoundEditorWindow *) window;
        if( sew )
        {
            SoundInstanceBlueprint *seb = sew->GetSoundInstanceBlueprint();
            seb->m_instance->SetSoundName( extensionRemoved );
        }
    }
};


class SoundSelectorButton : public InterfaceButton
{
public:
    void Render( int realX, int realY, bool highlighted, bool clicked )
    {
        InterfaceButton::Render( realX, realY, highlighted, clicked );

        SoundEditorWindow *sew = (SoundEditorWindow *) m_parent;
        if( sew )
        {
            SoundInstanceBlueprint *seb = sew->GetSoundInstanceBlueprint();
            g_renderer->TextRight( realX + m_w - 10, realY + 3, White, 14, seb->m_instance->m_soundName );
        }
    }

    void MouseUp()
    {
        SoundEditorWindow *sew = (SoundEditorWindow *) m_parent;
        SoundInstanceBlueprint *seb = sew->GetSoundInstanceBlueprint();
        switch( seb->m_instance->m_sourceType )
        {
            case SoundInstance::Sample:
            {
                SoundSelector *selector = new SoundSelector( "SoundName" );
                selector->Init( m_parent->m_name, "data/sounds/", NULL );
                selector->SetSize( 400, 300 );
                EclRegisterWindow( selector, m_parent );
                break;
            }

            case SoundInstance::SampleGroupRandom:
            {
                SampleGroupEditor *editor = new SampleGroupEditor( "SampleGroupEditor" );
                editor->SetSize( 290, 300 );
                editor->m_seb = seb;
                EclRegisterWindow( editor, m_parent );
                editor->SelectSampleGroup( seb->m_instance->m_soundName );                
                break;
            }
        }

    }
};


class DspEffectButton : public InterfaceButton
{
public:
    int m_fxIndex;

public:
    void Render( int realX, int realY, bool highlighted, bool clicked )
    {
        SoundEditorWindow *sew = (SoundEditorWindow *) m_parent;
        SoundInstanceBlueprint *seb = sew->GetSoundInstanceBlueprint();

        if( seb && seb->m_instance->m_dspFX.ValidIndex( m_fxIndex ) )
        {
            DspHandle *effect = seb->m_instance->m_dspFX[ m_fxIndex ];
            DspBlueprint *blueprint = g_soundSystem->m_blueprints.m_dspBlueprints[effect->m_type];
            SetCaption( blueprint->m_name, false );
            InterfaceButton::Render( realX, realY, highlighted, clicked );
        }       
        else if( seb && seb->m_instance->m_dspFX.Size() == m_fxIndex )
        {
            SetCaption( "New DSP Effect", false );
            InterfaceButton::Render( realX, realY, highlighted, clicked );
        }
    }

    void MouseUp()
    {
        SoundEditorWindow *sew = (SoundEditorWindow *) m_parent;
        SoundInstanceBlueprint *seb = sew->GetSoundInstanceBlueprint();
        
        if( seb && seb->m_instance->m_dspFX.ValidIndex( m_fxIndex ) )
        {
            DspEffectEditor *editor = new DspEffectEditor( "Effect Editor" );
            editor->SetSize( 300, 200 );
            editor->m_effect = seb->m_instance->m_dspFX[m_fxIndex];
            EclRegisterWindow( editor, m_parent );
        }
        else if( seb && seb->m_instance->m_dspFX.Size() == m_fxIndex )
        {
            DspHandle *effect = new DspHandle();
            DspBlueprint *blueprint = g_soundSystem->m_blueprints.m_dspBlueprints[ SoundLibrary3d::DSP_DSOUND_CHORUS ];
            effect->m_type = SoundLibrary3d::DSP_DSOUND_CHORUS;
            for( int i = 0; i < MAX_PARAMS; ++i )
            {
                float defaultVal;
                blueprint->GetParameter( i, &defaultVal );
                effect->m_params[i] = SoundParameter( defaultVal );
                effect->m_params[i].Recalculate();
            }

            seb->m_instance->m_dspFX.PutData( effect );             
        }
    }
};


class DeleteDspEffectButton : public InterfaceButton
{
public:
    int m_fxIndex;

public:
    void Render( int realX, int realY, bool highlighted, bool clicked )
    {
        SoundEditorWindow *sew = (SoundEditorWindow *) m_parent;
        SoundInstanceBlueprint *seb = sew->GetSoundInstanceBlueprint();

        if( seb && seb->m_instance->m_dspFX.ValidIndex( m_fxIndex ) )
        {
            InterfaceButton::Render( realX, realY, highlighted, clicked );
        }       
    }

    void MouseUp()
    {
        SoundEditorWindow *sew = (SoundEditorWindow *) m_parent;
        SoundInstanceBlueprint *seb = sew->GetSoundInstanceBlueprint();
        
        if( seb && seb->m_instance->m_dspFX.ValidIndex( m_fxIndex ) )
        {
            DspHandle *effect = seb->m_instance->m_dspFX[m_fxIndex];
            seb->m_instance->m_dspFX.RemoveData( m_fxIndex );

            delete effect;
        }
    }
};


class PurgeSoundsButton : public InterfaceButton
{
public:
    void MouseUp()
    {
        if( !EclGetWindow( "Purge Sounds" ) )
        {
            PurgeSoundsWindow *purge = new PurgeSoundsWindow( "Purge Sounds" );
            purge->SetSize( 400, 500 );
            EclRegisterWindow( purge, m_parent );
        }
    }
};

void ConvertVolume( float &_volume )
{
    float oldValue = _volume;

    if      ( _volume <= 1.0f )         _volume = 0.0f;    
    else if ( _volume <= 1.25f )        _volume = 0.5f; 
    else if ( _volume <= 1.5f )         _volume = 1.8f;
    else if ( _volume <= 1.75f )        _volume = 3.6f;
    else if ( _volume <= 2.0f )         _volume = 5.0f;
    else if ( _volume <= 2.25f )        _volume = 6.1f;
    else if ( _volume <= 2.5f )         _volume = 7.0f;
    else if ( _volume <= 2.75f )        _volume = 7.65f;
    else if ( _volume <= 3.0f )         _volume = 8.5f;
    else                                _volume = 10.0f;

    AppDebugOut( "Converted volume from %2.2f to %2.2f\n", oldValue, _volume );
}

class ConvertVolumesButton : public InterfaceButton
{
public:
    void MouseUp()
    {
        for( int i = 0; i < g_soundSystem->m_blueprints.m_blueprints.Size(); ++i )
        {
            if( g_soundSystem->m_blueprints.m_blueprints.ValidIndex(i) )
            {
                SoundEventBlueprint *ssb = g_soundSystem->m_blueprints.m_blueprints[i];
                for( int j = 0; j < ssb->m_events.Size(); ++j )
                {
                    SoundInstanceBlueprint *seb = ssb->m_events[j];
                    SoundParameter *volume = &seb->m_instance->m_volume;
                    switch( volume->m_type )
                    {
                        case SoundParameter::TypeFixedValue:
                            ConvertVolume( volume->m_outputLower );
                            break;

                        case SoundParameter::TypeRangedRandom:
                            ConvertVolume( volume->m_outputLower );
                            ConvertVolume( volume->m_outputUpper );
                            break;

                        case SoundParameter::TypeLinked:
                            ConvertVolume( volume->m_outputLower );
                            ConvertVolume( volume->m_outputUpper );
                            ConvertVolume( volume->m_inputLower );
                            ConvertVolume( volume->m_inputUpper );
                            break;
                    }
                }
            }
        }
    }
};


//*****************************************************************************
// Class SoundEditorWindow
//*****************************************************************************

SoundEditorWindow::SoundEditorWindow()
:   InterfaceWindow( "Sound Editor" ),
    m_eventIndex(-1)
{
    g_soundSystem->m_propagateBlueprints = true;

    m_w = 500;
    m_h = 550;
}


SoundEditorWindow::~SoundEditorWindow()
{
    g_soundSystem->m_propagateBlueprints = false;
}


void SoundEditorWindow::SelectEvent( int _eventIndex )
{
    RemoveInstanceEditor();
    m_eventIndex = -1;

    SoundEventBlueprint *ssb = GetSoundEventBlueprint();
    if( ssb && ssb->m_events.ValidIndex( _eventIndex ) )
    {
        m_eventIndex = _eventIndex;
        CreateInstanceEditor();
    }    
}


void SoundEditorWindow::StartPlayback( bool looped )
{
    StopPlayback();

    SoundInstanceBlueprint *seb = GetSoundInstanceBlueprint();
    if( seb )
    {
        SoundInstance *instance = new SoundInstance();        
        instance->Copy( seb->m_instance );
        if( instance->m_positionType != SoundInstance::TypeMusic )
        {
            instance->m_positionType = SoundInstance::TypeInEditor;
        }
        instance->m_loopType = looped;        
        instance->m_pos.Zero();
        instance->m_vel = Vector3<float>::ZeroVector();
        bool success = g_soundSystem->InitialiseSound( instance );        
        if( success )
        {
            g_soundSystem->m_editorInstanceId = instance->m_id;
        }
        else
        {
            delete instance;
        }
    }
}


void SoundEditorWindow::StopPlayback()
{
    SoundInstance *instance = g_soundSystem->GetSoundInstance( g_soundSystem->m_editorInstanceId );
    if( instance && instance->IsPlaying() )
    {
        instance->BeginRelease(true);
    }  
}


void SoundEditorWindow::Update()
{
}


void SoundEditorWindow::Create()
{
    InterfaceWindow::Create();
    

    //
    // Sound Source Selectors
    
    ObjectTypeMenu *objType = new ObjectTypeMenu();
    objType->SetProperties( "[ObjectType]", 10, 60, 150, 18, "[ObjectType]", " ", false, false );
    RegisterButton( objType);
    objType->SelectOption( -1 );

    LList<char *> allObjects;
    g_soundSystem->m_interface->ListObjectTypes( &allObjects );
    for( int i = 0; i < allObjects.Size(); ++i )
    {
        char *thisType = allObjects[i];
        objType->AddOption( thisType );
    }


    //
    // Events 

    NewSoundEventMenu *newEvent = new NewSoundEventMenu();
    newEvent->SetProperties( "New Event", 10, 90, 150, 18, "New Event", " ", false, false );
    RegisterButton( newEvent );
    newEvent->SelectOption(-1);
    
    for( int i = 0; i < 18; ++i )
    {
        char name[64];
        sprintf( name, "Event %d", i );
        SoundEventButton *event = new SoundEventButton();
        event->SetProperties( name, 10, 140 + i * 20, 150, 18, name, " ", false, false );
        event->m_eventIndex = i;
        RegisterButton( event );
    }    


    //
    // Control buttons
   
    PurgeSoundsButton *purge = new PurgeSoundsButton();
    purge->SetProperties( "Purge Sounds", 10, m_h - 65, 150, 18, "Purge Sounds", " ", false, false );
    RegisterButton(purge);

    SaveAllButton *save = new SaveAllButton();
    save->SetProperties( "Save", 10, m_h - 25, 150, 18, "Save", " ", false, false );
    RegisterButton( save );
}


void SoundEditorWindow::CreateInstanceEditor()
{
    SoundInstanceBlueprint *seb = GetSoundInstanceBlueprint();
    AppDebugAssert( seb );
    AppDebugAssert( seb->m_instance );

    
    //
    // Live parameters

    SoundPositionButton *spb = new SoundPositionButton();
    spb->SetProperties( "SoundPosition", 190, 50, 150, 100, "SoundPosition", " ", false, false );
    RegisterButton( spb );


    //
    // Play controls

    PlayButton *play = new PlayButton();
    play->SetProperties( "Play", 360, 50, 80, 20, "Play", " ", false, false );
    RegisterButton( play );

    PlayRepeatButton *repeat = new PlayRepeatButton();
    repeat->SetProperties( "Loop", 360, 80, 80, 20, "Loop", " ", false, false );
    RegisterButton( repeat );

    StopButton *stop = new StopButton();
    stop->SetProperties( "Stop", 360, 110, 80, 20, "Stop", " ", false, false );
    RegisterButton( stop );


    int xPos = 190;
    int yPos = 170;
    int yDif = 20;
    int w = 250;


    //
    // Inverted box

    InvertedBox *box = new InvertedBox();
    box->SetProperties( "invert", xPos-10, yPos-10, w+25, 260, " ", " ", false, false );
    RegisterButton( box );


    //
    // Sound name 

    SoundSelectorButton *selector = new SoundSelectorButton();
    selector->SetProperties( "SoundName", xPos + 75, yPos, w-75, 18, " ", " ", false, false );
    RegisterButton( selector );

    
    //
    // Position Type

    DropDownMenu *soundType = new DropDownMenu(false);
    soundType->SetProperties( "PositionType", xPos + 75, yPos += yDif, w - 75, 18, "PositionType", " ", false, false );
    RegisterButton( soundType );
    for( int i = 0; i < SoundInstance::NumPositionTypes; ++i )
    {
        soundType->AddOption( SoundInstance::GetPositionTypeName(i) );
    }
    soundType->RegisterInt( &seb->m_instance->m_positionType );    


    //
    // Loop type

    DropDownMenu *loopType = new DropDownMenu(false);
    loopType->SetProperties( "LoopType", xPos + 75, yPos += yDif, w - 75, 18, "LoopType", " ", false, false );
    RegisterButton( loopType );
    for( int i = 0; i < SoundInstance::NumLoopTypes; ++i )
    {
        loopType->AddOption( SoundInstance::GetLoopTypeName(i) );    
    }
    loopType->RegisterInt( &seb->m_instance->m_loopType );


    //
    // Instance type

    DropDownMenu *instanceType = new DropDownMenu(false);
    instanceType->SetProperties( "InstanceType", xPos + 75, yPos += yDif, w - 75, 18, "InstanceType", " ", false, false );
    RegisterButton( instanceType );
    for( int i = 0; i < SoundInstance::NumInstanceTypes; ++i )
    {
        instanceType->AddOption( SoundInstance::GetInstanceTypeName(i) );
    }
    instanceType->RegisterInt( &seb->m_instance->m_instanceType );
    

    //
    // Source type

    DropDownMenu *sourceType = new DropDownMenu(false);
    sourceType->SetProperties( "SourceType", xPos + 75, yPos += yDif, w - 75, 18, "SourceType", " ", false, false );
    RegisterButton( sourceType );
    for( int i = 0; i < SoundInstance::NumSourceTypes; ++i )
    {
        sourceType->AddOption( SoundInstance::GetSourceTypeName(i) );
    }
    sourceType->RegisterInt( &seb->m_instance->m_sourceType );

    //
    // Static parameters
    
    CreateValueControl          ( "MinDistance", xPos + 75, yPos+=yDif, w-75, 18, InputField::TypeFloat, &seb->m_instance->m_minDistance, 0.5f, 1.0f, 10000.0f, NULL, " ", false );

    yPos += 10;

    CreateSoundParameterButton  ( this, "Volume",       &seb->m_instance->m_volume, yPos += yDif, 0.0f, 10.0f, NULL, xPos, w );
    CreateSoundParameterButton  ( this, "Frequency",    &seb->m_instance->m_freq, yPos += yDif, 0.0f, 3.0f, NULL, xPos, w );
    CreateSoundParameterButton  ( this, "Attack",       &seb->m_instance->m_attack, yPos += yDif, 0.0f, 60.0f, NULL, xPos, w );
    CreateSoundParameterButton  ( this, "Sustain",      &seb->m_instance->m_sustain, yPos += yDif, -60.0f, 240.0f, NULL, xPos, w );
    CreateSoundParameterButton  ( this, "Release",      &seb->m_instance->m_release, yPos += yDif, 0.0f, 60.0f, NULL, xPos, w );
    CreateSoundParameterButton  ( this, "LoopDelay",    &seb->m_instance->m_loopDelay, yPos += yDif, 0.0f, 60.0f, NULL, xPos, w );

    //
    // Effects

    yPos += 60;
    
    for( int i = 0; i < 5; ++i )
    {
        char name[64];
        sprintf( name, "Effect %d", i );
        DspEffectButton *button = new DspEffectButton();
        button->SetProperties( name, xPos, yPos + i * 19, w-70, 17, name, " ", false, false );
        button->m_fxIndex = i;
        RegisterButton( button );

        sprintf( name, "Delete %d", i );
        DeleteDspEffectButton *deleteButton = new DeleteDspEffectButton();
        deleteButton->SetProperties( name, xPos + button->m_w + 10, yPos + i * 19, 60, 17, "Delete", " ", false, false );
        deleteButton->m_fxIndex = i;
        RegisterButton( deleteButton );
    }


    //
    // Control buttons

    DeleteEventButton *deleteEvent = new DeleteEventButton();
    deleteEvent->SetProperties( "Delete Event", m_w - 100, m_h - 30, 90, 18, "Delete Event", " ", false, false );
    RegisterButton( deleteEvent );
}


void SoundEditorWindow::RemoveInstanceEditor()
{
    RemoveButton( "SoundPosition" );
    
    RemoveButton( "Play" );
    RemoveButton( "Loop" );
    RemoveButton( "Stop" );

    RemoveButton( "invert" );
    
    RemoveButton( "PositionType" );
    RemoveButton( "InstanceType" );
    RemoveButton( "LoopType" );
    RemoveButton( "SourceType" );

    RemoveValueControl( "SoundName" );
    RemoveValueControl( "MinDistance" );

    RemoveSoundParameterButton( this, "Volume" );
    RemoveSoundParameterButton( this, "Frequency" );
    RemoveSoundParameterButton( this, "Attack" );
    RemoveSoundParameterButton( this, "Sustain" );
    RemoveSoundParameterButton( this, "Release" );
    RemoveSoundParameterButton( this, "LoopDelay" );

    for( int i = 0; i < 5; ++i )
    {
        char name[64];
        sprintf( name, "Effect %d", i );
        RemoveButton( name );

        sprintf( name, "Delete %d", i );
        RemoveButton( name );
    }

    RemoveButton( "Delete Event" );
}


void SoundEditorWindow::Render( bool hasFocus )
{
    InterfaceWindow::Render( hasFocus );
    
    g_renderer->Text( m_x + 10, m_y + 30, White, 20, "SOUND SOURCE" );
    g_renderer->Text( m_x + 10, m_y + 115, White, 20, "EVENTS" );
    g_renderer->Text( m_x + 200, m_y + 30, White, 20, "PARAMETERS" );
    g_renderer->Text( m_x + 200, m_y + 430, White, 20, "EFFECTS" );

    g_renderer->Text( m_x + 190, m_y + 175, White, 11, "Sample" );
    g_renderer->Text( m_x + 190, m_y + 195, White, 11, "Position" );
    g_renderer->Text( m_x + 190, m_y + 215, White, 11, "LoopType" );
    g_renderer->Text( m_x + 190, m_y + 235, White, 11, "InstanceType" );
    g_renderer->Text( m_x + 190, m_y + 255, White, 11, "SourceType" );
    g_renderer->Text( m_x + 190, m_y + 275, White, 11, "MinDistance" );
    
    glColor4f( 1.0f, 1.0f, 1.0f, 0.5f );
    glBegin( GL_LINES );
        glVertex2i( m_x + 170, m_y + 30 );
        glVertex2i( m_x + 170, m_y + m_h - 20 );
    glEnd();


    //
    // Render all running sounds

/*
    g_renderer->SetupMatricesFor3D();

    glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "sprites/sound.bmp" ) );
	glTexParameteri	( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri	( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
    glDepthMask     ( false );
    
    for( int i = 0; i < g_app->m_soundSystem->m_numChannels; ++i )
    {
        SoundInstanceId soundId = g_app->m_soundSystem->m_channels[i];
        SoundInstance *instance = g_app->m_soundSystem->GetSoundInstance( soundId );
        if( instance && 
            instance->m_positionType != SoundInstance::Type2D &&
            instance->m_positionType != SoundInstance::TypeInEditor )
        {
            float distance = ( instance->m_pos - g_app->m_camera->GetPos() ).Mag();
            float size = 0.5f * sqrtf(distance) * sqrtf(instance->m_channelVolume);
            Vector3 up = g_app->m_camera->GetUp() * size * 0.5f;
            Vector3 right = g_app->m_camera->GetRight() * size * -0.5f;

            int priority = instance->m_calculatedPriority;
            float colour = 0.5f + 0.5f * (float) priority / 10.0f;
            if( colour > 0.9f ) colour = 0.9f;
            glColor4f( 1.0f, 1.0f, 1.0f, colour );

            Vector3 pos = instance->m_pos;

            glBegin( GL_QUADS );
                glTexCoord2i( 0, 0 );       glVertex3fv( (pos - up - right).GetData() );
                glTexCoord2i( 1, 0 );       glVertex3fv( (pos - up + right).GetData() );
                glTexCoord2i( 1, 1 );       glVertex3fv( (pos + up + right).GetData() );
                glTexCoord2i( 0, 1 );       glVertex3fv( (pos + up - right).GetData() );
            glEnd();
        }
    }    
        
    glDepthMask     ( true );
    glDisable       ( GL_TEXTURE_2D );
    
    g_app->m_renderer->SetupMatricesFor2D();*/

}


SoundEventBlueprint *SoundEditorWindow::GetSoundEventBlueprint()
{
    ObjectTypeMenu *objTypeMenu = (ObjectTypeMenu *) GetButton( "[ObjectType]" );
    AppDebugAssert( objTypeMenu );

    char *objType = (char *) objTypeMenu->GetSelectionName();
    if( !objType ) return NULL;
    
    return g_soundSystem->m_blueprints.GetBlueprint( objType );
}


SoundInstanceBlueprint *SoundEditorWindow::GetSoundInstanceBlueprint()
{
    SoundEventBlueprint *ssb = GetSoundEventBlueprint();
    if( ssb && ssb->m_events.ValidIndex( m_eventIndex ) )
    {
        return ssb->m_events[m_eventIndex];
    }

    return NULL;
}


void SoundEditorWindow::GetSelectionName(char *_buf)
{
    DropDownMenu *objTypeMenu = (DropDownMenu *) GetButton( "[ObjectType]" );
	strcpy(_buf, objTypeMenu->GetSelectionName());
//
//    AppDebugAssert( objGroupMenu );
//    AppDebugAssert( objTypeMenu );
//
//    int objGroup = objGroupMenu->GetSelectionIndex();
//    int objType = objTypeMenu->GetSelectionIndex();
//
//    switch( objGroup )
//    {
//        case 0 :    
//            strcpy(_buf, Entity::GetTypeName(objType));
//            break;
//
//        case 1:
//            strcpy(_buf, Building::GetTypeName(objType));
//            break;    
//            
//        case 2:
//			strcpy(_buf, SoundSourceBlueprint::GetSoundSourceName(objType));
//            break;
//    }
}


void SoundEditorWindow::CreateSoundParameterButton( InterfaceWindow *_window,
                                                    char *name, SoundParameter *parameter, int y, 
							                        float _minOutput, float _maxOutput,
                                                    InterfaceButton *callback, int x, int w )
{
    if( x == -1 ) x = 10;
    if( w == -1 ) w = _window->m_w - x*2;

    SoundParameterButton *button = new SoundParameterButton();
    button->SetProperties( name, x, y, w, 18, name, " ", false, false );
    button->m_parameter = parameter;
    button->m_minOutput = _minOutput;
    button->m_maxOutput = _maxOutput;
    _window->RegisterButton( button );
}


void SoundEditorWindow::RemoveSoundParameterButton( InterfaceWindow *_window, char *name )
{
    _window->RemoveButton( name );
}


//*****************************************************************************
// Class DspEffectTypeMenu
//*****************************************************************************

class DspEffectTypeMenu : public DropDownMenu
{
    void SelectOption( int _value )
    {
        int index = 0;
        char name[64];
        sprintf( name, "param %d", index );
        while( m_parent->GetButton(name) )
        {
            m_parent->RemoveButton(name);
            ++index;
            sprintf( name, "param %d", index);
        }
        
        bool changed = m_currentOption != -1 &&
                       m_currentOption != _value;

        DropDownMenu::SelectOption( _value );
		int optionIndex = FindValue(_value);
        AppDebugAssert( g_soundSystem->m_blueprints.m_dspBlueprints[ optionIndex ] );
        DspBlueprint *seb = g_soundSystem->m_blueprints.m_dspBlueprints[ optionIndex ];
        DspEffectEditor *see = (DspEffectEditor *) m_parent;
        
        for( int i = 0; i < seb->m_params.Size(); ++i )
        {            
            char name[64];
            sprintf( name, "param %d", i );

            SoundParameter *param = &see->m_effect->m_params[i];

            char *paramName;
            float minOutput, maxOutput, defaultOutput;
            paramName = seb->GetParameter( i, &minOutput, &maxOutput, &defaultOutput );

            if( changed )
            {
                param->m_type = SoundParameter::TypeFixedValue;
                param->m_outputLower = defaultOutput;
            }

            param->Recalculate();

            SoundEditorWindow::CreateSoundParameterButton( (InterfaceWindow *) m_parent, name, 
                                                            param, 60 + i * 20, 
                                                            minOutput, maxOutput );

            if( m_parent->m_h < 90 + i * 20 ) m_parent->m_h = 90 + i * 20;
            
            m_parent->GetButton( name )->SetCaption( paramName, false );
        }                
    }
};


//*****************************************************************************
// Class DspEffectEditor
//*****************************************************************************

DspEffectEditor::DspEffectEditor( char *_name )
:   InterfaceWindow(_name),
    m_effect(NULL)
{
}

void DspEffectEditor::Create()
{
    InterfaceWindow::Create();

    int y = 30;
    int h = 20;

    DspEffectTypeMenu *fxType = new DspEffectTypeMenu();
    fxType->SetProperties( "Effect Type", 10, y, m_w - 20, 18, "Effect Type", " ", false, false );
    RegisterButton( fxType );
    for( int i = 0; i < g_soundSystem->m_blueprints.m_dspBlueprints.Size(); ++i )
    {
        DspBlueprint *effect = g_soundSystem->m_blueprints.m_dspBlueprints[i];
        fxType->AddOption( effect->m_name );
    }
    
    fxType->RegisterInt( &m_effect->m_type );
}



//*****************************************************************************
// Class SampleGroupEditor
//*****************************************************************************

class SampleGroupFileDialog : public FileDialog
{
public:
    SampleGroup *m_group;
    int m_index;
	
    SampleGroupFileDialog( char *name)
    :   FileDialog( name )
    {
    }

    void FileSelected( char *_filename )
    {
        char const *extensionRemoved = RemoveExtension( _filename );
        //char const *err = g_soundSystem->m_blueprints.IsSoundSourceOK(extensionRemoved);
		//if (err)
		//{
			//MessageDialog *win = new MessageDialog("Alert", err);
			//EclRegisterWindow(win);
		//	return;
		//}

        if( m_group->m_samples.ValidIndex( m_index ) )
        {
            m_group->m_samples.RemoveData(m_index);
            
            char *filenameCopy = strdup( extensionRemoved );
            m_group->m_samples.PutDataAtIndex( filenameCopy, m_index );
        }
        else
        {
            char *filenameCopy = strdup( extensionRemoved );
            m_group->m_samples.PutData( filenameCopy );
        }
    }
};


class SampleGroupSampleName : public InterfaceButton
{
public:
    int m_index;

    void Render( int realX, int realY, bool highlighted, bool clicked )
    {
        EclButton *button = m_parent->GetButton( "SampleGroup" );
        char *groupName = button->m_caption;
        SampleGroup *group = g_soundSystem->m_blueprints.GetSampleGroup( groupName );

        if( group && group->m_samples.ValidIndex(m_index) )
        {
            SetCaption( group->m_samples[m_index], false );
            InterfaceButton::Render( realX, realY, highlighted, clicked );
        }
    }

    void MouseUp()
    {
        EclButton *button = m_parent->GetButton( "SampleGroup" );
        char *groupName = button->m_caption;
        SampleGroup *group = g_soundSystem->m_blueprints.GetSampleGroup( groupName );
    }
};


class SampleGroupSampleDelete : public InterfaceButton
{
public:
    int m_index;

    void Render( int realX, int realY, bool highlighted, bool clicked )
    {
        EclButton *button = m_parent->GetButton( "SampleGroup" );
        char *groupName = button->m_caption;
        SampleGroup *group = g_soundSystem->m_blueprints.GetSampleGroup( groupName );

        int baseOffset = ((SampleGroupEditor *) m_parent)->m_scrollbar->m_currentValue;
        int index = m_index + baseOffset;

        if( group && group->m_samples.ValidIndex(index) )
        {
            InterfaceButton::Render( realX, realY, highlighted, clicked );
        }
    }

    void MouseUp()
    {
        EclButton *button = m_parent->GetButton( "SampleGroup" );
        char *groupName = button->m_caption;
        SampleGroup *group = g_soundSystem->m_blueprints.GetSampleGroup( groupName );

        int baseOffset = ((SampleGroupEditor *) m_parent)->m_scrollbar->m_currentValue;
        int index = m_index + baseOffset;

        if( group && group->m_samples.ValidIndex(index) )
        {
            group->m_samples.RemoveData(index);
        }
    }
};


class SampleGroupAddSample : public InterfaceButton
{
    void MouseUp()
    {
        EclButton *button = m_parent->GetButton( "SampleGroup" );
        char *groupName = button->m_caption;
        SampleGroup *group = g_soundSystem->m_blueprints.GetSampleGroup( groupName );
        
        if( group )
        {
            SampleGroupFileDialog *file = new SampleGroupFileDialog("SampleGroupSelectFile" );
            file->Init( m_parent->m_name, "data/sounds/", NULL );
            file->SetSize( 400, 300 );
            file->m_group = group;
            file->m_index = -1;
            EclRegisterWindow( file, m_parent );
        }
    }
};


class SampleGroupSelector : public DropDownMenu
{
    void SelectOption( int _option )
    {
        DropDownMenu::SelectOption( _option );

        SampleGroupEditor *sge = (SampleGroupEditor *) m_parent;
        sge->m_seb->m_instance->SetSoundName( m_options[m_currentOption]->m_word );
    }
};


class NewSampleGroupButton : public InterfaceButton
{
    void MouseUp()
    {
        SampleGroup *newGroup = g_soundSystem->m_blueprints.NewSampleGroup();
        SampleGroupEditor *sge = (SampleGroupEditor *) m_parent;
        sge->Remove();
        sge->Create();
        sge->SelectSampleGroup( newGroup->m_name );
    }
};


class RenameSampleGroupCommit : public InterfaceButton
{
    void MouseUp()
    {
        RenameSampleGroupWindow *window = (RenameSampleGroupWindow *) m_parent;
        bool success = g_soundSystem->m_blueprints.RenameSampleGroup( window->m_oldName, window->m_newName );
        if( success )
        {
            EclRemoveWindow( m_parent->m_name );        
        }
        else
        {
//			MessageDialog *win = new MessageDialog(
//					"Failed to Rename Sample Group", 
//					"A SampleGroup already exists with that name");
//			EclRegisterWindow(win);
        }        
    }
};


RenameSampleGroupWindow::RenameSampleGroupWindow( char *_name )
:   InterfaceWindow( _name )
{
    SetSize( 250, 120 );
}

void RenameSampleGroupWindow::Create()
{
    InterfaceWindow::Create();

    //CreateValueControl( "Name", 20, 20, 100, 10, InputField::TypeString, m_newName, 20, 0.0f, 0.0f );
    InputField *input = new InputField();
    input->SetProperties( "Name", 20, 30, 100, 20, " ", " ", false, false );
    input->RegisterString( m_newName );
    RegisterButton( input );

    RenameSampleGroupCommit *commit = new RenameSampleGroupCommit();
    commit->SetProperties( "OK", 10, 60, 110, 18, "OK", " ", false, false );
    RegisterButton( commit );

    CloseButton *closeButton = new CloseButton();
    closeButton->SetProperties( "Cancel", 130, 60, 110, 18, "Cancel", " ", false, false );
    RegisterButton( closeButton );
}


class RenameSampleGroupButton : public InterfaceButton
{
    void MouseUp()
    {
        EclButton *button = m_parent->GetButton( "SampleGroup" );
        char *groupName = button->m_caption;

        RenameSampleGroupWindow *window = new RenameSampleGroupWindow( "Rename Sample Group" );
        strcpy( window->m_oldName, groupName );
        strcpy( window->m_newName, groupName );
        EclRegisterWindow( window, m_parent );

        EclRemoveWindow( m_parent->m_name );
    }
};


SampleGroupEditor::SampleGroupEditor( char *_name )
:   InterfaceWindow( _name ),
    m_seb(NULL),
    m_scrollbar(NULL)
{
    m_scrollbar = new ScrollBar(this);
}


void SampleGroupEditor::SelectSampleGroup( char *_group )
{
    DropDownMenu *sampleGroup = (DropDownMenu *) GetButton( "SampleGroup" );
    sampleGroup->SelectOption2( _group );
}


void SampleGroupEditor::Create()
{
    InterfaceWindow::Create();
        
    //
    // Sample group selector / new Sample Group

    SampleGroupSelector *sampleGroup = new SampleGroupSelector();
    sampleGroup->SetProperties( "SampleGroup", 10, 20, m_w - 100, 18, "SampleGroup", " ", false, false );
    RegisterButton( sampleGroup );
    for( int i = 0; i < g_soundSystem->m_blueprints.m_sampleGroups.Size(); ++i )
    {
        if( g_soundSystem->m_blueprints.m_sampleGroups.ValidIndex(i) )
        {
            SampleGroup *group = g_soundSystem->m_blueprints.m_sampleGroups[i];
            sampleGroup->AddOption( group->m_name );
        }
    }
    
    NewSampleGroupButton *newGroup = new NewSampleGroupButton();
    newGroup->SetProperties( "New Group", sampleGroup->m_x + sampleGroup->m_w + 10, 20, 70, 18, "New Group", " ", false, false );
    RegisterButton( newGroup );

    //
    // Rename group

    RenameSampleGroupButton *renameGroup = new RenameSampleGroupButton();
    renameGroup->SetProperties( "Rename Group", 10, 50, m_w - 20, 17, "Rename Group", " ", false, false );
    RegisterButton( renameGroup );


    //
    // Add Samples to group

    SampleGroupAddSample *addSample = new SampleGroupAddSample();
    addSample->SetProperties( "Add Samples", 10, 70, m_w - 20, 17, "Add Samples", " ", false, false );
    RegisterButton( addSample );

    
    //
    // Edit / Remove Samples from group

    int numToCreate = (m_h - 100) / 20;

    for( int i = 0; i < numToCreate; ++i )
    {
        char name[64];
        sprintf( name, "SampleGroupSampleDelete %d", i );
        SampleGroupSampleDelete *sd = new SampleGroupSampleDelete();
        sd->SetProperties( name, 10, 100 + i * 20, 70, 17, "Delete", " ", false, false );
        sd->m_index = i;
        RegisterButton( sd );
    }


    //
    // Scrollbar

    m_scrollbar->Create( "Scrollbar", m_w - 20, 100, 15, m_h - 110, 0, numToCreate );
}


void SampleGroupEditor::Remove()
{
    InterfaceWindow::Remove();
    
    RemoveButton( "SampleGroup" );
    RemoveButton( "New Group" );
    RemoveButton( "Add Samples" );
    
    int i = 0;
    while( true )
    {
        char name[64];
        sprintf( name, "SampleGroupSampleName %d", i );
        if( !GetButton(name) ) break;
        RemoveButton( name );

        sprintf( name, "SampleGroupSampleDelete %d", i );
        RemoveButton( name );

        ++i;
    }

    m_scrollbar->Remove();
}


void SampleGroupEditor::Render( bool _hasFocus )
{
    InterfaceWindow::Render( _hasFocus );

    EclButton *button = GetButton( "SampleGroup" );
    char *groupName = button->m_caption;
    SampleGroup *group = g_soundSystem->m_blueprints.GetSampleGroup( groupName );

    
    //
    // Update the scrollbar
    // Draw the samples in the group
    
    if( group )
    {
        m_scrollbar->SetNumRows( group->m_samples.Size() );

        int numToCreate = (m_h - 100) / 20;
        int baseOffset = m_scrollbar->m_currentValue;
        for( int i = 0; i < numToCreate; ++i )
        {
            int index = baseOffset + i;
            if( group->m_samples.ValidIndex(index) )
            {
                char *thisSample = group->m_samples[index];
                g_renderer->Text( m_x + 90, m_y + 107 + i * 20, White, 14, thisSample );
            }
        }
    }
    else
    {
        m_scrollbar->SetNumRows( 0 );
    }    
}


//*****************************************************************************
// Class PurgeSoundsWindow
//*****************************************************************************

class DeleteSoundButton : public InterfaceButton
{
public:
    int m_index;

    void Render( int realX, int realY, bool highlighted, bool clicked )
    {
        PurgeSoundsWindow *parent = (PurgeSoundsWindow *) m_parent;
        int baseOffset = parent->m_scrollBar->m_currentValue;
        int index = m_index + baseOffset;

        if( parent->m_fileList.ValidIndex( index ) )
        {
            InterfaceButton::Render( realX, realY, highlighted, clicked );
        }
    }

    void MouseUp()
    {
        PurgeSoundsWindow *parent = (PurgeSoundsWindow *) m_parent;
        int baseOffset = parent->m_scrollBar->m_currentValue;
        int index = m_index + baseOffset;

        if( parent->m_fileList.ValidIndex( index ) )
        {
            if( strcmp( m_caption, "delete" ) == 0 )
            {
                SetCaption( "!DELETE!", false );
            }
            else
            {
                char *filename = parent->m_fileList[ index ];
                char fullFilename[256];
                sprintf( fullFilename, "data/sounds/%s", filename );
                DeleteThisFile( fullFilename );
                parent->RefreshFileList();
                SetCaption( "delete", false );
            }
        }
    }
};


class RefreshFileListButton : public InterfaceButton
{
    void MouseUp()
    {
        PurgeSoundsWindow *parent = (PurgeSoundsWindow *) m_parent;
        parent->RefreshFileList();
    }
};


class DeleteAllButton : public InterfaceButton 
{
    void MouseUp()
    {
        if( strcmp( m_caption, "Delete All" ) == 0 )
        {
            SetCaption( "!DELETE ALL!", false );
        }
        else
        {
            PurgeSoundsWindow *parent = (PurgeSoundsWindow *) m_parent;
            for( int i = 0; i < parent->m_fileList.Size(); ++i )
            {
                char *filename = parent->m_fileList[i];
                char fullFilename[256];
                sprintf( fullFilename, "data/sounds/%s", filename );
                DeleteThisFile( fullFilename );
            }
            parent->RefreshFileList();
            SetCaption( "Delete All", false );
        }
    }
};


PurgeSoundsWindow::PurgeSoundsWindow( char *_name )
:   InterfaceWindow( _name ),
    m_scrollBar(NULL)
{
    m_scrollBar = new ScrollBar( this );
}


void PurgeSoundsWindow::Create()
{
    InterfaceWindow::Create();

    int numSlots = (m_h - 75) / 16;

    for( int i = 0; i < numSlots; ++i )
    {
        char name[64];
        sprintf( name, "DeleteSound %d", i );
        DeleteSoundButton *deleteSound = new DeleteSoundButton();
        deleteSound->SetProperties( name, 10, 42 + i * 16, 60, 15, "delete", " ", false, false );
        deleteSound->m_index = i;
        RegisterButton( deleteSound );
    }

    RefreshFileListButton *refresh = new RefreshFileListButton();
    refresh->SetProperties( "Refresh", 10, m_h - 30, (m_w - 40)/2, 18, "Refresh", " ", false, false );
    RegisterButton( refresh );

    DeleteAllButton *deleteAll = new DeleteAllButton();
    deleteAll->SetProperties( "Delete All", refresh->m_x + refresh->m_w + 10, m_h - 30, (m_w - 40)/2, 18, "Delete All", " ", false, false );
    RegisterButton( deleteAll );
    
    m_scrollBar->Create( "Scrollbar", m_w - 20, 42, 15, m_h - 75, 0, numSlots );

    RefreshFileList();
}


void PurgeSoundsWindow::Remove()
{
    InterfaceWindow::Remove();
    
    m_fileList.EmptyAndDelete();

    int i = 0;
    char name[64];
    sprintf( name, "DeleteSound %d", i );
    while( GetButton( name ) )
    {
        RemoveButton( name );

        ++i;
        sprintf( name, "DeleteSound %d", i );
    }

    RemoveButton( "Refresh" );
    RemoveButton( "Delete All" );

    m_scrollBar->Remove();
}


void PurgeSoundsWindow::Render( bool _hasFocus )
{
    InterfaceWindow::Render( _hasFocus );
    
    g_renderer->TextCentre( m_x + m_w/2, m_y + 25, White, 16, "Unused Sounds : " );

    int baseOffset = m_scrollBar->m_currentValue;
    int numSlots = m_scrollBar->m_winSize;

    for( int i = 0; i < numSlots; ++i )
    {
        int index = i + baseOffset;        
        if( m_fileList.ValidIndex(index) )
        {
            char *filename = m_fileList[index];
            g_renderer->Text( m_x + 80, m_y + 50 + i * 16, White, 13, filename );
        }
    }
}


void PurgeSoundsWindow::RefreshFileList()
{
    m_fileList.EmptyAndDelete();

    LList<char *> *allFiles = ListDirectory( "data/sounds/", "*.*", false );
    
    for( int i = 0; i < allFiles->Size(); ++i )
    {
        char *filename = allFiles->GetData(i);
        const char *strippedFilename = RemoveExtension( filename );
        if( !g_soundSystem->m_blueprints.IsSampleUsed( strippedFilename ) )
        {
            m_fileList.PutData(filename);
        }
        else
        {
            delete filename;
        }
    }

    delete allFiles;    

    m_scrollBar->SetNumRows( m_fileList.Size() );
}


