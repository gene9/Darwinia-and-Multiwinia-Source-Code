/*
 * ===========
 * FILE DIALOG
 * ===========
 *
 * A generic file dialog that allows the user
 * to select a file
 *
 */

#ifndef _included_filedialog_h
#define _included_filedialog_h


#include "lib/darray.h"
#include "interface/darwinia_window.h"

class ScrollBar;



class FileDialog : public DarwiniaWindow
{
public:
    char    *m_path;
    char    *m_filter;
    char    *m_parent;
	bool    m_allowMultiSelect;
    bool	m_okPressed;

    LList   <char *> *m_files;
    LList   <int>     m_selected;

    ScrollBar   *m_scrollBar;

public:
    FileDialog( char const *name, char const *parent, 
                char const *path=NULL, char const *filter=NULL,
                bool allowMultiSelect=false );
    ~FileDialog();

    void Create();
    void Remove();

    void SetDirectory   ( char const *path );
    void SetParent      ( char const *parent );
    void SetFilter      ( char const *filter );

    void FileClicked    ( int index );
    int  IsFileSelected ( int index );                                  // Returns index within m_selected, or -1 if not found

    void RefreshFileList();

    virtual void FileSelected( char *filename );                        // This will be called for all selected files
};


#endif
