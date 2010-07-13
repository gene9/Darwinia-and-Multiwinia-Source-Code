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


#include "lib/tosser/llist.h"
#include "interface/components/core.h"

class ScrollBar;



class FileDialog : public InterfaceWindow
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
    FileDialog( char *_name );
    ~FileDialog();

    void Init( char *parent, 
               char *path=NULL, char *filter=NULL,
               bool allowMultiSelect=false );

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
