#include "lib/universal_include.h"
#include "lib/render/styletable.h"
#include "lib/render/renderer.h"
#include "lib/debug_utils.h"
#include "lib/preferences.h"
#include "lib/language_table.h"

#include "interface/components/drop_down_menu.h"
#include "interface/components/inputfield.h"
#include "interface/components/checkbox.h"
#include "interface/components/filedialog.h"
#include "interface/components/scrollbar.h"
#include "interface/components/message_dialog.h"

#include "renderer/map_renderer.h"

#include "styleeditor_window.h"

#include <string.h>

class ApplyStyleButton : public InterfaceButton
{
    void MouseUp();
};


class ColourEditorWindow : public InterfaceWindow
{
public:
    int     m_styleIndex;
    bool    m_primary;
    
    int     m_r;
    int     m_g;
    int     m_b;
    int     m_a;

    ApplyStyleButton m_applyStyleButton;

public:
    ColourEditorWindow( char *_name, char *_title, bool _titleIsLanguagePhrase, int styleIndex, int primary )
        :   InterfaceWindow(_name, _title, _titleIsLanguagePhrase),
            m_styleIndex(styleIndex),
            m_primary(primary)
    {
        SetSize( 300, 200 );

        AppAssert( g_styleTable->m_styles.ValidIndex(styleIndex) );

        Style *style = g_styleTable->m_styles[styleIndex];

        Colour col = m_primary ? style->m_primaryColour : style->m_secondaryColour;
        
        m_r = col.m_r;
        m_g = col.m_g;
        m_b = col.m_b;
        m_a = col.m_a;

        m_applyStyleButton.SetParent( this );
    }


    void Create()
    {
        InterfaceWindow::Create();
        
        int xPos = 90;
        int yPos = 30;
        int height = 20;
        int gap = 4;
        int width = m_w - 110;

        CreateValueControl( "Red", xPos, yPos+=height+gap, width, height, InputField::TypeInt, &m_r, 1, 0, 255, &m_applyStyleButton, " ", false );
        CreateValueControl( "Green", xPos, yPos+=height+gap, width, height, InputField::TypeInt, &m_g, 1, 0, 255, &m_applyStyleButton, " ", false );
        CreateValueControl( "Blue", xPos, yPos+=height+gap, width, height, InputField::TypeInt, &m_b, 1, 0, 255, &m_applyStyleButton, " ", false );
        CreateValueControl( "Alpha", xPos, yPos+=height+gap, width, height, InputField::TypeInt, &m_a, 1, 0, 255, &m_applyStyleButton, " ", false );
    }

    void Render( bool _hasFocus )
    {
        InterfaceWindow::Render( _hasFocus );
        
        Style *style = g_styleTable->m_styles[m_styleIndex];
        g_renderer->Text( m_x+10, m_y+30, White, 15, style->m_name );
        g_renderer->TextRight( m_x+m_w-10, m_y+30, White, 15, m_primary ? LANGUAGEPHRASE("dialog_style_primary") : LANGUAGEPHRASE("dialog_style_secondary") );
        
        int xPos = m_x+20;
        int yPos = m_y+35;
        int height = 24;
        
        g_renderer->Text( xPos, yPos+=height, White, 12, LANGUAGEPHRASE("dialog_style_red") );
        g_renderer->Text( xPos, yPos+=height, White, 12, LANGUAGEPHRASE("dialog_style_green") );
        g_renderer->Text( xPos, yPos+=height, White, 12, LANGUAGEPHRASE("dialog_style_blue") );
        g_renderer->Text( xPos, yPos+=height, White, 12, LANGUAGEPHRASE("dialog_style_alpha") );

    }
};


void ApplyStyleButton::MouseUp()
{
    ColourEditorWindow *parent = (ColourEditorWindow *)m_parent;

    Style *style = g_styleTable->m_styles[ parent->m_styleIndex ];

    Colour *col = parent->m_primary ? &style->m_primaryColour : &style->m_secondaryColour;

    col->m_r = parent->m_r;
    col->m_g = parent->m_g;
    col->m_b = parent->m_b;
    col->m_a = parent->m_a;
}



// ============================================================================


class ColourButton : public EclButton
{
public:
    int     m_styleIndex;
    bool    m_primary;

    void Render( int realX, int realY, bool highlighted, bool clicked )
    {
        StyleEditorWindow *parent = (StyleEditorWindow *)m_parent;
        int realIndex = m_styleIndex + parent->m_scrollbar->m_currentValue;

        if( g_styleTable->m_styles.ValidIndex(realIndex) )
        {
            Style *style = g_styleTable->m_styles[realIndex];
            Colour col = m_primary ? style->m_primaryColour : style->m_secondaryColour;

            if( style->m_negative )
            {
                g_renderer->SetBlendMode( Renderer::BlendModeSubtractive );
            }

            g_renderer->RectFill( realX, realY, m_w, m_h, col );

            g_renderer->SetBlendMode( Renderer::BlendModeNormal );
            
            Colour borderCol(255,255,255,100);
            if( highlighted || clicked ) borderCol.m_a = 255;

            g_renderer->Rect( realX, realY, m_w, m_h, borderCol );
        }
    }


    void MouseUp()
    {
        StyleEditorWindow *parent = (StyleEditorWindow *)m_parent;
        int realIndex = m_styleIndex + parent->m_scrollbar->m_currentValue;

        if( g_styleTable->m_styles.ValidIndex(realIndex) )
        {
            char name[256];
            sprintf( name, "Colour Editor %d %d", realIndex, m_primary );
			char title[256];
            sprintf( title, LANGUAGEPHRASE("dialog_style_colour_editor") );
			LPREPLACEINTEGERFLAG( 'I', realIndex, title );
			LPREPLACEINTEGERFLAG( 'P', m_primary, title );
            EclRegisterWindow( new ColourEditorWindow( name, title, false, realIndex, m_primary ), m_parent );
        }
    }
};


class ListStylesDialog : public FileDialog
{
public:
    ListStylesDialog()
        :   FileDialog("List Styles")
    {
    }

    void FileSelected( char *_filename )
    {
        StyleEditorWindow *parent = (StyleEditorWindow *) EclGetWindow(m_parent);
        parent->Remove();

        bool success = g_styleTable->Load(_filename);

        if( success )
        {
            strcpy( parent->m_filename, _filename);
            g_preferences->SetString( PREFS_INTERFACE_STYLE, _filename );            
            g_preferences->Save();        
        }
        else
        {
            char caption[512];
            sprintf( caption, LANGUAGEPHRASE("dialog_style_failed_load_style") );
			strcat( caption, "data/styles/" );
			strcat( caption, _filename );
            MessageDialog *msg = new MessageDialog( "Style", caption, false, "dialog_style", true );
            EclRegisterWindow( msg);
        }

        parent->Create();
    }
};


class LoadStylesButton : public InterfaceButton
{
    void MouseUp()
    {
        ListStylesDialog *list = new ListStylesDialog();
        list->Init( m_parent->m_name, "data/styles/" );
        list->SetSize( 300, 300 );
        EclRegisterWindow( list, m_parent );
    }
};


class SaveStylesButton : public InterfaceButton
{
    void MouseUp()
    {
        StyleEditorWindow *parent = (StyleEditorWindow *)m_parent;

        bool success = g_styleTable->Save(parent->m_filename);

        char caption[512];
        if( success )
        {
            sprintf( caption, LANGUAGEPHRASE("dialog_style_style_saved") );
        }
        else
        {
            sprintf( caption, LANGUAGEPHRASE("dialog_style_failed_save_style") );
        }
		strcat( caption, "data/styles/" );
		strcat( caption, parent->m_filename );
        MessageDialog *msg = new MessageDialog( "Style", caption, false, "dialog_style", true );
        EclRegisterWindow( msg, m_parent );
    }
};


class FontFileDialog : public FileDialog
{
public:
    int m_styleIndex;

    FontFileDialog( char *_name )
        : FileDialog(_name)
    {
    }

    void FileSelected( char *_filename )
    {
        StyleEditorWindow *parent = (StyleEditorWindow*)EclGetWindow(m_parent);
        if( parent )
        {
            char filenamePrepped[256];
            strcpy( filenamePrepped, _filename );

            char *dot = strchr(filenamePrepped, '.' );
            if( *dot ) *dot = '\x0';

            if( g_styleTable->m_styles.ValidIndex(m_styleIndex) )
            {
                Style *style = g_styleTable->m_styles[m_styleIndex];
                strcpy( style->m_fontName, filenamePrepped );
            }
        }
    }
};


class FontFilenameButton : public InterfaceButton
{
public:
    int m_styleIndex;

    void Render( int realX, int realY, bool highlighted, bool clicked )
    {
        StyleEditorWindow *parent = (StyleEditorWindow *)m_parent;
        int realIndex = m_styleIndex + parent->m_scrollbar->m_currentValue;

        if( g_styleTable->m_styles.ValidIndex(realIndex) )
        {
            Style *style = g_styleTable->m_styles[realIndex];       
            if( style->m_font )
            {
                InterfaceButton::Render( realX, realY, highlighted, clicked );
                g_renderer->Text( realX+10, realY+5, White, 12, style->m_fontName );
            }
        }
    }

    void MouseUp()
    {
        StyleEditorWindow *parent = (StyleEditorWindow *)m_parent;
        int realIndex = m_styleIndex + parent->m_scrollbar->m_currentValue;

        if( g_styleTable->m_styles.ValidIndex(realIndex) )
        {
            Style *style = g_styleTable->m_styles[realIndex];       
            if( style->m_font )
            {
                FontFileDialog *font = new FontFileDialog("Select Font");
                font->Init( m_parent->m_name, "data/fonts/" );
                font->SetSize( 300, 300 );
                font->m_styleIndex = realIndex;
                EclRegisterWindow( font, m_parent );
            }
        }
    }
};


class StyleCheckBox : public CheckBox
{
public:
    int m_styleIndex;
    int m_data;

    void Render( int realX, int realY, bool highlighted, bool clicked )
    {
        StyleEditorWindow *parent = (StyleEditorWindow *)m_parent;
        int realIndex = m_styleIndex + parent->m_scrollbar->m_currentValue;

        if( g_styleTable->m_styles.ValidIndex(realIndex) )
        {
            Style *style = g_styleTable->m_styles[realIndex];  
            switch( m_data )
            {
                case 1 :    RegisterBool( &style->m_horizontal );           break;
                case 2 :    RegisterBool( &style->m_negative );             break;
                case 3 :    RegisterBool( &style->m_uppercase );            break;
            }

            if( m_data == 1 || style->m_font )
            {
                CheckBox::Render( realX, realY, highlighted, clicked );
            }
        }
    }

    void MouseUp()
    {
        StyleEditorWindow *parent = (StyleEditorWindow *)m_parent;
        int realIndex = m_styleIndex + parent->m_scrollbar->m_currentValue;

        if( g_styleTable->m_styles.ValidIndex(realIndex) )
        {
            Style *style = g_styleTable->m_styles[realIndex];  
            if( m_data == 1 || style->m_font )
            {
                CheckBox::MouseUp();
            }
        }
    }
};


class StyleInputField : public InputField
{
public:
    int m_styleIndex;
    void Render( int realX, int realY, bool highlighted, bool clicked )
    {
        StyleEditorWindow *parent = (StyleEditorWindow *)m_parent;
        int realIndex = m_styleIndex + parent->m_scrollbar->m_currentValue;

        if( g_styleTable->m_styles.ValidIndex(realIndex) )
        {
            Style *style = g_styleTable->m_styles[realIndex];  
            RegisterFloat( &style->m_size );

            if( style->m_font )
            {
                InputField::Render( realX, realY, highlighted, clicked );
            }
        }
    }
};


class StyleDefaultButton : public InterfaceButton
{
    void MouseUp()
    {
        StyleEditorWindow *parent = (StyleEditorWindow *) m_parent;
        parent->Remove();

        g_styleTable->Load("default.txt");
        g_preferences->SetString( PREFS_INTERFACE_STYLE, "default.txt" );
        g_preferences->Save();

        parent->Create();
        strcpy( parent->m_filename, "default.txt");
    }
};


StyleEditorWindow::StyleEditorWindow()
:   InterfaceWindow( "StyleEditor", "dialog_style_editor", true )
{    
    SetSize( 700, 500 );
    
    m_minH = 100;

    m_scrollbar = new ScrollBar(this);

    sprintf( m_filename, "default.txt" );
    if( g_styleTable->m_filename ) sprintf( m_filename, g_styleTable->m_filename );    
}


StyleEditorWindow::~StyleEditorWindow()
{
    delete m_scrollbar;
}


void StyleEditorWindow::Create()
{
    InterfaceWindow::Create();

    InvertedBox *box = new InvertedBox();
    box->SetProperties( "box", 10, 30, m_w-40, m_h-70, " ", " ", false, false );
    RegisterButton(box);
    
    int yPos = 60;
    int height = 20;
    int gap = 8;

    int numRows = g_styleTable->m_styles.Size();
    int winSize = (m_h-100)/(height+gap);

    m_scrollbar->Create( "scrollbar", m_w-30, 30, 20, m_h-70, numRows, winSize );
    
    for( int i = 0; i < winSize; ++i )
    {
        int xPos = 160;
        char name[256];

        Style *style = g_styleTable->m_styles[i];

        sprintf( name, "primary %d", i );
        ColourButton *primary = new ColourButton();
        primary->SetProperties( name, xPos, yPos, 80, height, name, " ", false, false );
        primary->m_styleIndex = i;
        primary->m_primary = true;
        RegisterButton( primary );

        xPos += 100;

        sprintf( name, "secondary %d", i );
        ColourButton *secondary = new ColourButton();
        secondary->SetProperties( name, xPos, yPos, 80, height, name, " ", false, false );
        secondary->m_styleIndex = i;
        secondary->m_primary = false;
        RegisterButton( secondary );

        xPos += 100;

        sprintf( name, "horizontal %d", i );
        StyleCheckBox *horizontal = new StyleCheckBox();
        horizontal->SetProperties( name, xPos, yPos, height, height, name, " ", false, false );
        horizontal->m_styleIndex = i;
        horizontal->m_data = 1;
        RegisterButton( horizontal );

        xPos += 40;

        sprintf( name, "negative %d", i );
        StyleCheckBox *negative = new StyleCheckBox();
        negative->SetProperties( name, xPos, yPos, height, height, name, " ", false, false );
        negative->m_styleIndex = i;
        negative->m_data = 2;
        RegisterButton( negative );

        xPos += 40;

        sprintf( name, "uppercase %d", i );
        StyleCheckBox *uppercase = new StyleCheckBox();
        uppercase->SetProperties( name, xPos, yPos, height, height, name, " ", false, false );
        uppercase->m_styleIndex = i;
        uppercase->m_data = 3;
        RegisterButton( uppercase );

        xPos += 40;

        sprintf( name, "size %d", i );
        StyleInputField *size = new StyleInputField();
        size->SetProperties( name, xPos, yPos, 60, height, " ", " ", false, false );
        size->m_lowBound = 0.0f;
        size->m_highBound = 100.0f;
        size->m_styleIndex = i;
        RegisterButton(size);

        xPos += 80;

        sprintf( name, "font %d", i );
        FontFilenameButton *fontSelect = new FontFilenameButton();
        fontSelect->SetProperties( name, xPos, yPos, 100, height, " ", " ", false, false );
        fontSelect->m_styleIndex = i;
        RegisterButton(fontSelect);

        yPos += height;
        yPos += gap;
    }
    

    //
    // Filename 

    InputField *filename = new InputField();
    filename->SetProperties( "Filename", 10, m_h-30, 150, 20, " ", " ", false, false );
    filename->RegisterString( m_filename );
    RegisterButton( filename );


    //
    // Control buttons

    StyleDefaultButton *styleDefault = new StyleDefaultButton();
    styleDefault->SetProperties( "Restore Default", m_w-390, m_h-30, 120, 20, "dialog_restore_default", " ", true, false );
    RegisterButton( styleDefault );

    LoadStylesButton *load = new LoadStylesButton();
    load->SetProperties( "Load", m_w-260, m_h-30, 120, 20, "dialog_load", " ", true, false );
    RegisterButton( load );

    SaveStylesButton *save = new SaveStylesButton();
    save->SetProperties( "Save", m_w-130, m_h-30, 120, 20, "dialog_save", " ", true, false );
    RegisterButton( save );
}


void StyleEditorWindow::Render( bool _hasFocus )
{
    InterfaceWindow::Render( _hasFocus );

    g_renderer->Text( m_x+160, m_y+40, White, 14, LANGUAGEPHRASE("dialog_style_col_primary") );
    g_renderer->Text( m_x+260, m_y+40, White, 14, LANGUAGEPHRASE("dialog_style_col_secondary") );
    g_renderer->Text( m_x+360, m_y+43, White, 10, LANGUAGEPHRASE("dialog_style_col_horiz") );
    g_renderer->Text( m_x+400, m_y+43, White, 10, LANGUAGEPHRASE("dialog_style_col_ngtive") );
    g_renderer->Text( m_x+440, m_y+43, White, 10, LANGUAGEPHRASE("dialog_style_col_caps") );
    g_renderer->Text( m_x+490, m_y+40, White, 14, LANGUAGEPHRASE("dialog_style_col_size") );
    g_renderer->Text( m_x+590, m_y+40, White, 14, LANGUAGEPHRASE("dialog_style_col_font"));
    

    int yPos = m_y+70;
    int height = 20;
    int gap = 8;

    for( int i = m_scrollbar->m_currentValue; i < m_scrollbar->m_currentValue + m_scrollbar->m_winSize; ++i )
    {
        int xPos = m_x+20;
             
        if( g_styleTable->m_styles.ValidIndex(i) )
        {
            Style *style = g_styleTable->m_styles[i];

			char styleLanguagePhrase[512];
			snprintf( styleLanguagePhrase, sizeof(styleLanguagePhrase), "style_%s", style->m_name );
			styleLanguagePhrase[ sizeof(styleLanguagePhrase) - 1 ] = '\0';
			if( g_languageTable->DoesTranslationExist( styleLanguagePhrase ) || g_languageTable->DoesPhraseExist( styleLanguagePhrase ) )
			{
				g_renderer->Text( xPos, yPos, White, 12, LANGUAGEPHRASE(styleLanguagePhrase) );
			}
			else
			{
				g_renderer->Text( xPos, yPos, White, 12, style->m_name );
			}

            yPos += height;
            yPos += gap;
        }
    }
}


