#include "lib/universal_include.h"

#include <string.h>

#include "lib/filesys_utils.h"
#include "lib/hi_res_time.h"
#include "lib/text_renderer.h"
#include "lib/input/input.h"
#include "lib/resource.h"
#include "lib/language_table.h"

#include "eclipse.h"

#include "interface/scrollbar.h"
#include "interface/filedialog.h"

#include "app.h"


//*****************************************************************************
// Class FileOKButton
//*****************************************************************************

class FileOKButton: public DarwiniaButton
{
public:
    void MouseUp()
    {
        FileDialog *fd = (FileDialog *) m_parent;

        for( int i = 0; i < fd->m_selected.Size(); ++i )
        {
            int index = fd->m_selected[i];
            DarwiniaDebugAssert( fd->m_files->ValidIndex(index) );
            char *filename = fd->m_files->GetData( index );
            fd->FileSelected( filename );
        }

        EclRemoveWindow( m_parent->m_name );
    }
};


//*****************************************************************************
// Class FileButton
//*****************************************************************************

class FileButton: public EclButton
{
public:
    int		m_index;
	double	m_lastClickTime;

public:
	FileButton(int _index)
	:	m_index(_index),
		m_lastClickTime(-1.0)
	{
	}

    void MouseUp()
	{
		FileDialog *fd = (FileDialog *) m_parent;
		int index = m_index + fd->m_scrollBar->m_currentValue;

		if( fd->m_files && fd->m_files->ValidIndex( index ) )
		{
            fd->FileClicked( index );
		}

		double timeNow = GetHighResTime();
		double delta = timeNow - m_lastClickTime;
		if (delta < 0.2)
		{
			FileOKButton *ok = (FileOKButton*)fd->GetButton(LANGUAGEPHRASE("dialog_ok"));
			ok->MouseUp();
			return;
		}
		m_lastClickTime = timeNow;
	}

    void Render( int realX, int realY, bool highlighted, bool clicked )
	{
		FileDialog *fd = (FileDialog *) m_parent;
		int index = m_index + fd->m_scrollBar->m_currentValue;

		if( fd->m_files && fd->m_files->ValidIndex( index ) )
		{
            if( fd->IsFileSelected(index) != -1 )
            {
				glColor4f( 0.3f, 0.3f, 1.0f, 0.5f );
				glBegin( GL_QUADS );
					glVertex2i( realX, realY );
					glVertex2i( realX + m_w, realY );
					glVertex2i( realX + m_w, realY + m_h );
					glVertex2i( realX, realY + m_h );
				glEnd();
            }

    		glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );

			if( clicked || highlighted )
			{
				glBegin( GL_LINE_LOOP );
					glVertex2i( realX, realY );
					glVertex2i( realX + m_w, realY );
					glVertex2i( realX + m_w, realY + m_h );
					glVertex2i( realX, realY + m_h );
				glEnd();
			}

			char *fileName = fd->m_files->GetData( index );
			g_editorFont.DrawText2D( realX + 30, realY + 8, 11, fileName);
		}
	}
};


class FileCancelButton: public DarwiniaButton
{
    void MouseUp()
    {
        EclRemoveWindow( m_parent->m_name );
    }
};


//*****************************************************************************
// Class SelectedButton
//*****************************************************************************

class SelectedButton : public DarwiniaButton
{
    void Render( int realX, int realY, bool highlighted, bool clicked )
    {
 		FileDialog *fd = (FileDialog *) m_parent;
        if( fd->m_selected.Size() > 1 )
        {
            SetCaption( LANGUAGEPHRASE("dialog_multiplefiles") );
        }
        else if( fd->m_selected.Size() == 1 )
        {
            int index = fd->m_selected[0];
            char *filename = fd->m_files->GetData(index);
            SetCaption( filename );
        }
        else
        {
            SetCaption( " " );
        }

        DarwiniaButton::Render( realX, realY, highlighted, clicked );
    }
};


//*****************************************************************************
// Class FileDialog
//*****************************************************************************

FileDialog::FileDialog( char const *name, char const *parent,
                        char const *path, char const *filter,
                        bool allowMultiSelect )
:   DarwiniaWindow( name ),
    m_files(NULL),
    m_path(NULL),
    m_filter(NULL),
    m_parent(NULL),
    m_scrollBar(NULL),
    m_allowMultiSelect(allowMultiSelect)
{
    SetFilter( filter ? filter : "*" );
    SetDirectory( path ? path : "c:\\" );
    SetParent( parent );

    m_scrollBar = new ScrollBar(this);
}


FileDialog::~FileDialog()
{
    free(m_path);
    free(m_filter);
    free(m_parent);

    if( m_files )
    {
        m_files->EmptyAndDelete();
        delete m_files;
        m_files = NULL;
    }

    m_selected.Empty();

    delete m_scrollBar;
}


void FileDialog::Create()
{
    DarwiniaWindow::Create();

    int numRows = (m_h - 60) / 13;

    for( int i = 0; i < numRows; ++i )
    {
        char name[32];
        sprintf( name, "File %d", i );
        FileButton *button = new FileButton(i);
        button->SetProperties( name, 5, 25 + i * 13, m_w - 25, 12, " ", " " );
        RegisterButton( button );
    }

    SelectedButton *selected = new SelectedButton();
    selected->SetProperties( "Selected", 10, m_h - 30, m_w - 140, 20, "", " " );
    RegisterButton( selected );

    FileCancelButton *cancel = new FileCancelButton();
    cancel->SetProperties( LANGUAGEPHRASE("dialog_cancel"), m_w - 60, m_h - 30, 55, 20, LANGUAGEPHRASE("dialog_cancel") );
    RegisterButton( cancel );

    FileOKButton *ok = new FileOKButton();
    ok->SetProperties( LANGUAGEPHRASE("dialog_ok"), m_w - 120, m_h - 30, 55, 20, LANGUAGEPHRASE("dialog_ok") );
    RegisterButton( ok );

    m_scrollBar->Create( "FileScroll", m_w - 20, 25, 15, numRows * 13, m_files->Size(), numRows );
}


void FileDialog::Remove()
{
    DarwiniaWindow::Remove();

    m_scrollBar->Remove();
}


void FileDialog::SetDirectory( char const *path )
{
	free(m_path);
    m_path = strdup( path );
    SetTitle( (char *)path );
    RefreshFileList();
}


void FileDialog::SetFilter( char const *filter )
{
	free(m_filter);
    m_filter = strdup( filter );
}


void FileDialog::SetParent( char const *parent )
{
	free(m_parent);
    m_parent = strdup( parent );
}


void FileDialog::FileSelected( char *filename )
{
}


void FileDialog::RefreshFileList()
{
    if( m_files )
    {
        m_files->EmptyAndDelete();
        delete m_files;
        m_files = NULL;
    }

    m_selected.Empty();

    m_files = g_app->m_resource->ListResources( m_path, m_filter, false );

    EclDirtyWindow( m_name );
}

void FileDialog::FileClicked( int index )
{
    bool ctrlKey = g_inputManager->controlEvent( ControlFileMultiSelect );

    if( !m_allowMultiSelect || !ctrlKey )
    {
        m_selected.Empty();
    }

    int alreadySelected = IsFileSelected( index );
    if( alreadySelected != -1 && m_allowMultiSelect )
    {
        m_selected.RemoveData( alreadySelected );
    }
    else
    {
        m_selected.PutData( index );
    }
}


int FileDialog::IsFileSelected( int index )
{
    for( int i = 0; i < m_selected.Size(); ++i )
    {
        if( m_selected[i] == index )
        {
            return i;
        }
    }

    return -1;
}
