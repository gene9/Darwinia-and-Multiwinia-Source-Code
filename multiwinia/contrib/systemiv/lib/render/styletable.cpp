#include "lib/universal_include.h"
#include "lib/debug_utils.h"
#include "lib/string_utils.h"
#include "lib/filesys/file_system.h"
#include "lib/filesys/text_stream_readers.h"
#include "lib/filesys/filesys_utils.h"

#include "styletable.h"
#include "renderer.h"


StyleTable *g_styleTable = NULL;


StyleTable::StyleTable()
:    m_filename(NULL)
{  
}


void StyleTable::Clear()
{
    m_styles.EmptyAndDelete();
}


bool StyleTable::Load( char *_filename )
{
    delete [] m_filename;
    m_filename = newStr(_filename);

    char fullFilename[512];
    sprintf( fullFilename, "data/styles/%s", _filename );
    TextReader *in = g_fileSystem->GetTextReader( fullFilename );

    if(!in || !in->IsOpen())
    {
        return false;
    }

    while( in->ReadLine() )
    {
        if( !in->TokenAvailable() ) continue;
        char *styleHeader = in->GetNextToken();
        AppDebugAssert( stricmp( styleHeader, "STYLE" ) == 0 );
        char *styleName = in->GetNextToken();

        Style *style = GetStyle(styleName);

        if( !style )
        {
            style = new Style(styleName);
            m_styles.PutData( style );
        }

        in->ReadLine();
        char *param = in->GetNextToken();
        while( stricmp( param, "END" ) != 0 )
        {
            if( stricmp(param, "PRIMARYCOL") == 0 )     style->m_primaryColour.LoadFromHex( in->GetNextToken() );
            if( stricmp(param, "SECONDARYCOL") == 0 )   style->m_secondaryColour.LoadFromHex( in->GetNextToken() );
            if( stricmp(param, "FONT") == 0 )           strcpy( style->m_fontName, in->GetNextToken() );
            if( stricmp(param, "NEGATIVE") == 0 )       style->m_negative = (bool) atoi( in->GetNextToken() );
            if( stricmp(param, "HORIZONTAL") == 0 )     style->m_horizontal = (bool) atoi( in->GetNextToken() );
            if( stricmp(param, "UPPERCASE") == 0 )      style->m_uppercase = (bool) atoi( in->GetNextToken() );
            if( stricmp(param, "SIZE") ==  0 )          style->m_size = atof( in->GetNextToken() );
                        
            in->ReadLine();
            param = in->GetNextToken();
        }
    }

    delete in;

    return true;
}


bool StyleTable::Save( char *_filename )
{
    if( !_filename && !m_filename ) 
    {
        AppDebugOut( "StyleTable error : failed to save because filename is NULL\n" );
        return false;
    }

    char fullFilename[512];
    sprintf( fullFilename, "data/styles/%s", _filename ? _filename : m_filename );    

    CreateDirectory( "data/" );
    CreateDirectory( "data/styles/" );

    FILE *file = fopen(fullFilename,"wt");

    if( !file ) return false;
    
    for( int i = 0; i < m_styles.Size(); ++i )
    {
        Style *style = m_styles[i];
        
        char primaryCol[64];
        char secondaryCol[64];
        style->m_primaryColour.GetHexValue( primaryCol );
        style->m_secondaryColour.GetHexValue( secondaryCol );

        fprintf( file, "STYLE %s\n", style->m_name );
        fprintf( file, "\tPRIMARYCOL        %s\n", primaryCol );
        fprintf( file, "\tSECONDARYCOL      %s\n", secondaryCol );
        fprintf( file, "\tHORIZONTAL        %d\n", (int)style->m_horizontal );
        
        if( style->m_font )
        {
            fprintf( file, "\tFONT              %s\n", style->m_fontName );
            fprintf( file, "\tNEGATIVE          %d\n", (int)style->m_negative );
            fprintf( file, "\tUPPERCASE         %d\n", (int)style->m_uppercase );
            fprintf( file, "\tSIZE              %2.2f\n", style->m_size );
        }
        
        fprintf( file, "END\n\n" );
    }

    fclose(file);

    return true;
}


void StyleTable::AddStyle( Style *_style )
{
    if( GetStyle(_style->m_name) )
    {
        AppDebugOut( "StyleTable Warning : Tried to add style to style table, but name is already used : %s\n", _style->m_name );
    }

    m_styles.PutData( _style );
}


Style *StyleTable::GetStyle( char *_name )
{
    for( int i = 0; i < m_styles.Size(); ++i )
    {
        Style *style = m_styles[i];
        if( strcmp(style->m_name, _name) == 0 )
        {
            return style;
        }
    }

    return NULL;
}


Colour StyleTable::GetPrimaryColour( char *_name )
{
    Style *style = GetStyle(_name);
    if( !style)
    {
        AppDebugOut( "StyleTable warning : couldn't find colour '%s'\n", _name );
        return Colour(255,0,255,255);
    }

    Colour result = style->m_primaryColour;
    result.m_a *= g_renderer->m_alpha;

    return result;
}


Colour StyleTable::GetSecondaryColour( char *_name )
{
    Style *style = GetStyle(_name);
    if( !style)
    {
        AppDebugOut( "StyleTable warning : couldn't find colour '%s'\n", _name );
        return Colour(255,0,255,255);
    }

    Colour result = style->m_secondaryColour;
    result.m_a *= g_renderer->m_alpha;

    return result;
}


Style::Style( char *_name )
:   m_size(0),
    m_negative(false),
    m_font(false),
    m_uppercase(false),
    m_horizontal(true)
{
    strcpy( m_name, _name );
    strcpy( m_fontName, "default" );

    if( strstr(m_name, "Font") ) m_font = true;
}
