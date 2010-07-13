#include "lib/universal_include.h"

#include <strstream>
#include <sstream>
#include <algorithm>

#include <stdio.h>
#include <string.h>

#include "lib/string_utils.h"
#include "lib/debug_utils.h"
#include "lib/network_stream.h"
#include "lib/unicode/unicode_string.h"

#include "directory.h"

#ifdef TARGET_OS_LINUX
#include <iconv.h>
#endif


#define DIRECTORY_MARKERSIZE     1
#define DIRECTORY_MARKERSTART    "<"
#define DIRECTORY_MARKEREND      ">"

#define DIRECTORY_MAXDIRSIZE        1024
#define DIRECTORY_MAXSTRINGLENGTH   10240


using namespace std;

#ifdef TARGET_DEBUG
#define _DEBUG
#endif

Directory::Directory()
:   m_name(NULL)
{
    SetName("NewDirectory");
}


Directory::Directory( const Directory &_copyMe )
:   m_name(NULL)
{
    SetName( _copyMe.m_name );
    CopyData( &_copyMe, false, true );
}


Directory& Directory::operator = (const Directory&_copyMe)
{
    SetName( _copyMe.m_name );
    CopyData( &_copyMe, false, true );	
	return *this;
}


template <class T> 
static
bool DirectoryEqual( DArray<T *> const &_a, DArray<T *> const &_b )
{
	int size = std::max( _a.Size(), _b.Size() );

	for( int i = 0; i < size; i++ )
	{
		bool aValid = _a.ValidIndex( i ),
			 bValid = _b.ValidIndex( i );

		if( aValid != bValid )
			return false;

		if( aValid && bValid &&
			!(*_a.GetData( i ) == *_b.GetData( i )) )
			return false;
	}

	return true;
}

bool Directory::operator ==( const Directory &_another ) const
{
	return 
		DirectoryEqual( m_subDirectories, _another.m_subDirectories ) &&
		DirectoryEqual( m_data, _another.m_data );
}

bool Directory::operator !=( const Directory &_another ) const
{
	return !(*this == _another);
}


Directory::~Directory()
{
    delete[] m_name;
    m_name = NULL;

    m_data.EmptyAndDelete();
    m_subDirectories.EmptyAndDelete();    
}


void Directory::SetName ( const char *newName )
{    
    if( newName && strlen(newName) > 0 )
    {
		char *oldName = m_name;
		m_name = newStr(newName);
		delete[] oldName;
    }
}


Directory *Directory::GetDirectory ( const char *dirName ) const
{
    AppAssert( dirName );

    char *firstSubDir = GetFirstSubDir ( dirName );
    char *otherSubDirs = GetOtherSubDirs ( dirName );

    if ( firstSubDir )
    {
        Directory *subDir = NULL;

        //
        // Search for that subdirectory

        for ( int i = 0; i < m_subDirectories.Size(); ++i )
        {
            if ( m_subDirectories.ValidIndex(i) )
            {
                Directory *thisDir = m_subDirectories[i];
                AppAssert( thisDir );
                if ( strcmp ( thisDir->m_name, firstSubDir ) == 0 )
                {
                    subDir = thisDir;
                    break;
                }
            }
        }

		delete[] firstSubDir;

        if ( !otherSubDirs )
        {
            return subDir;
        }
        else
        {
            return subDir->GetDirectory( otherSubDirs );
        }                       
    }
    else
    {
        AppReleaseAssert( false, "Error getting directory %s", dirName );
        return NULL;
    }
}


void Directory::RemoveDirectory( const char *dirName )
{
    for( int i = 0; i < m_subDirectories.Size(); ++i )
    {
        if( m_subDirectories.ValidIndex(i) )
        {
            Directory *subDir = m_subDirectories[i];
            if( strcmp(subDir->m_name, dirName ) == 0 )
            {
                m_subDirectories.RemoveData(i);             
            }
        }
    }
}


Directory *Directory::AddDirectory ( const char *dirName )
{
    AppAssert( dirName );

    char *firstSubDir = GetFirstSubDir ( dirName );
    char *otherSubDirs = GetOtherSubDirs ( dirName );

    if ( firstSubDir )
    {
        Directory *subDir = NULL;

        //
        // Search for that subdirectory

        for ( int i = 0; i < m_subDirectories.Size(); ++i )
        {
            if ( m_subDirectories.ValidIndex(i) )
            {
                Directory *thisDir = m_subDirectories[i];
                AppAssert( thisDir );
                if ( strcmp ( thisDir->m_name, firstSubDir ) == 0 )
                {
                    subDir = thisDir;
                    break;
                }
            }
        }

        // If the directory doesn't exist, create it and recurse into it

        if ( !subDir )
        {
            subDir = new Directory();
            subDir->SetName( firstSubDir );
            m_subDirectories.PutData( subDir );
        }

        delete[] firstSubDir;
        firstSubDir = NULL;

        //
        // Have we finished building directories?
        // Otherwise, recurse in

        if ( !otherSubDirs )
        {
            return subDir;
        }
        else
        {
            return subDir->AddDirectory( otherSubDirs );
        }                       

    }
    else
    {
        AppReleaseAssert(false, "Failed to get first SubDir from %s", dirName );
        return NULL;
    }    
}


void Directory::AddDirectory ( Directory *dir )
{
    AppAssert( dir );
    m_subDirectories.PutData( dir );
}


void Directory::CopyData ( const Directory *dir, bool overWrite, bool directories )
{
    AppAssert( dir );

    //
    // Copy Data

    for( int i = 0; i < dir->m_data.Size(); ++i )
    {
        if( dir->m_data.ValidIndex(i) )
        {
            DirectoryData *data = dir->m_data[i];
            AppAssert(data);

            DirectoryData *existing = GetData( data->m_name );
            if( existing && overWrite )
            {
                existing->SetData( data );            
            }
            else
            {
                DirectoryData *newData = new DirectoryData();
                newData->SetData( data );
                m_data.PutData( newData );
            }
        }
    }

    //
    // Copy subdirectories

    if( directories )
    {
        for( int d = 0; d < dir->m_subDirectories.Size(); ++d )
        {
            if( dir->m_subDirectories.ValidIndex(d) )
            {
                Directory *subDir = dir->m_subDirectories[d];
                AppAssert( subDir );
    
                Directory *newDir = new Directory( *subDir );
                AddDirectory( newDir );
            }
        }
    }
}


char *Directory::GetFirstSubDir ( const char *dirName )
{
    AppAssert( dirName );

    //
    // Is there a starting slash
    
    const char *startPoint = dirName;
    if ( dirName[0] == '/' )
    {
        startPoint = (dirName+1);
    }

    //
    // Find the first slash after that

    const char *endPoint = strchr( startPoint, '/' );

    int dirSize;
    
    if ( endPoint )
    {
        dirSize = (endPoint - startPoint);
    }
    else
    {
        dirSize = strlen(startPoint);
    }

    if ( dirSize == 0 )
    {
        return NULL;
    }
    else
    {
        char *result = new char[dirSize+1];
        strncpy ( result, startPoint, dirSize );
        result[dirSize] = '\x0';
        return result;
    }
}


char *Directory::GetOtherSubDirs ( const char *dirName )
{
    AppAssert ( dirName );

    //
    // Is there a starting slash
    
    const char *startPoint = dirName;
    if ( dirName[0] == '/' )
    {
        startPoint = (dirName+1);
    }

    //
    // Find the first slash after that

    const char *endPoint = strchr( startPoint, '/' );
    
    if ( endPoint && (endPoint < (startPoint + strlen(startPoint) - 1) ) )
    {
        return const_cast<char *>(endPoint+1);
    }
    else
    {
        return NULL;
    }
}




void Directory::CreateData ( const char *dataName, int value )
{
    AppAssert ( dataName );
    
    if ( GetData( dataName ) )
    {
        RemoveData( dataName );
    }

    DirectoryData *newData = new DirectoryData();
    newData->SetName( dataName );
    newData->SetData( value );
    m_data.PutData( newData );
}


void Directory::CreateData ( const char *dataName, unsigned long long value )
{
    AppAssert ( dataName );
    
    if ( GetData( dataName ) )
    {
        RemoveData( dataName );
    }

    DirectoryData *newData = new DirectoryData();
    newData->SetName( dataName );
    newData->SetData( value );
    m_data.PutData( newData );
}

void Directory::CreateData ( const char *dataName, float value )
{
    AppAssert ( dataName );
    
    if ( GetData( dataName ) )
    {
        RemoveData( dataName );
    }

    DirectoryData *newData = new DirectoryData();
    newData->SetName( dataName );
    newData->SetData( value );
    m_data.PutData( newData );
}


#ifdef USE_FIXED
void Directory::CreateData ( const char *dataName, Fixed value )
{
    AppAssert ( dataName );
    
    if ( GetData( dataName ) )
    {
        RemoveData( dataName );
    }

    DirectoryData *newData = new DirectoryData();
    newData->SetName( dataName );
    newData->SetData( value );
    m_data.PutData( newData );
}
#endif

void Directory::CreateData( const char *dataName, unsigned char value )
{
    CreateData( dataName, (char) value );
}


void Directory::CreateData ( const char *dataName, char value )
{
    AppAssert ( dataName );
    
    if ( GetData( dataName ) )
    {
        RemoveData( dataName );
    }

    DirectoryData *newData = new DirectoryData();
    newData->SetName( dataName );
    newData->SetData( value );
    m_data.PutData( newData );
}


void Directory::CreateData ( const char *dataName, const char *value )
{
    AppAssert ( dataName );
    
    if ( GetData( dataName ) )
    {
        RemoveData( dataName );
    }

    DirectoryData *newData = new DirectoryData();
    newData->SetName( dataName );
    newData->SetData( value );
    m_data.PutData( newData );
}


void Directory::CreateData ( const char *dataName, bool value )
{
    AppAssert ( dataName );
    
    if ( GetData( dataName ) )
    {
        RemoveData( dataName );
    }

    DirectoryData *newData = new DirectoryData();
    newData->SetName( dataName );
    newData->SetData( value );
    m_data.PutData( newData );
}


void Directory::CreateData( const char *dataName, const void *data, int dataLen )
{
    AppAssert( dataName );

    if ( GetData( dataName ) )
    {
        RemoveData( dataName );
    }
    
    DirectoryData *newData = new DirectoryData();
    newData->SetName( dataName );
    newData->SetData( data, dataLen );
    m_data.PutData( newData );
}


void Directory::CreateData( const char *dataName, const int *data, int numElems )
{
    AppAssert( dataName );

    if ( GetData( dataName ) )
    {
        RemoveData( dataName );
    }
    
    DirectoryData *newData = new DirectoryData();
    newData->SetName( dataName );
    newData->SetData( data, numElems );
    m_data.PutData( newData );
}

void Directory::CreateData( const char *dataName, const std::vector<int> &intVector )
{
	CreateData( dataName, &intVector[0], intVector.size() );
}

void Directory::CreateData( const char *dataName, const wchar_t *data )
{
    AppAssert( dataName );

    if( GetData( dataName ) )
    {
        RemoveData( dataName );
    }

    DirectoryData *newData = new DirectoryData();
    newData->SetName( dataName );
    newData->SetData( data );
    m_data.PutData( newData );
}

void Directory::CreateData( const char *dataName, const UnicodeString &_data )
{
	CreateData( dataName, _data.m_unicodestring );
}

int Directory::GetDataInt ( const char *dataName ) const
{
    DirectoryData *data = GetData( dataName );

    if ( data && data->IsInt() )
    {
        return data->m_int;
    }
    else
    {
        AppDebugOut( "Int Data not found : %s\n", dataName );
        return DIRECTORY_SAFEINT;
    }
}


unsigned long long Directory::GetDataULLong ( const char *dataName ) const
{
    DirectoryData *data = GetData( dataName );

    if ( data && data->IsULLong() )
    {
        return data->m_ullong;
    }
    else
    {
        AppDebugOut( "Int Data not found : %s\n", dataName );
        return DIRECTORY_SAFEULLONG;
    }
}


float Directory::GetDataFloat ( const char *dataName ) const
{
    DirectoryData *data = GetData( dataName );

    if ( data && data->IsFloat() )
    {
        return data->m_float;
    }
    else
    {
        AppDebugOut( "Float Data not found : %s\n", dataName );
        return DIRECTORY_SAFEFLOAT;
    }
}

#ifdef USE_FIXED
Fixed Directory::GetDataFixed ( const char *dataName ) const
{
    DirectoryData *data = GetData( dataName );

    if ( data && data->IsFixed() )
    {
#ifdef FLOAT_NUMERICS
        return Fixed::FromDouble(data->m_fixed);
#elif defined(FIXED64_NUMERICS)
		return Fixed::Fixed(data->m_fixed);
#endif
    }
    else
    {
        AppDebugOut( "Fixed Data not found : %s\n", dataName );
        return Fixed(DIRECTORY_SAFEINT);
    }
}
#endif

unsigned char Directory::GetDataUChar( const char *dataName ) const
{
    return (unsigned char) GetDataChar(dataName);
}


char Directory::GetDataChar ( const char *dataName ) const
{
    DirectoryData *data = GetData( dataName );

    if ( data && data->IsChar() )
    {
        return data->m_char;
    }
    else
    {
        // AppDebugOut( "Char Data not found : %s\n", dataName );
        return DIRECTORY_SAFECHAR;
    }
}


char *Directory::GetDataString ( const char *dataName ) const
{
    DirectoryData *data = GetData( dataName );

    if ( data && data->IsString() && data->m_string )
    {
        return data->m_string;
    }
    else
    {
        AppDebugOut( "String Data not found : %s\n", dataName );
        return DIRECTORY_SAFESTRING;
    }
}

wchar_t *Directory::GetDataUnicode(const char *dataName) const
{
    DirectoryData *data = GetData( dataName );
    {
        if( data && data->IsUnicode() && data->m_unicode )
        {
            return data->m_unicode;
        }
        else
        {
            AppDebugOut("Unicode Data not found : %s\n", dataName );
            return DIRECTORY_SAFEUNICODE;
        }
    }
}

bool Directory::GetDataUnicodeWithStringFallback( const char *dataName, UnicodeString &_dest )
{
    DirectoryData *data = GetData( dataName );
    
    if( data )
	{
		if( data->IsUnicode() && data->m_unicode )
		{
			_dest = data->m_unicode; 
			return true;
		}
		else if( data->IsString() && data->m_string )
		{
			_dest = data->m_string;
			return true;
		}
	}

	AppDebugOut("Unicode/String Data not found : %s\n", dataName );
	_dest = DIRECTORY_SAFEUNICODE;
	return false;
}



bool Directory::GetDataBool ( const char *dataName ) const
{
    DirectoryData *data = GetData( dataName );

    if ( data && data->IsBool() )
    {
        return data->m_bool;
    }
    else
    {
        AppDebugOut( "Bool Data not found : %s\n", dataName );
        return DIRECTORY_SAFEBOOL;
    }
}


void *Directory::GetDataVoid( const char *dataName, int *_dataLen ) const
{
    DirectoryData *data = GetData( dataName );
    
    if( data && data->IsVoid() )
    {
        *_dataLen = data->m_voidLen;
        return data->m_void;
    }
    else
    {
        AppDebugOut( "Void data not found : %s\n", dataName );
        return const_cast<char *>(DIRECTORY_SAFESTRING);
    }
}


int	*Directory::GetDataInts( const char *dataName, unsigned *_numElems ) const
{
    DirectoryData *data = GetData( dataName );
    
    if( data && data->IsInts() )
    {
        *_numElems = data->m_intsLen;
        return data->m_ints;
    }
    else
    {
        AppDebugOut( "Void data not found : %s\n", dataName );
        return NULL;
    }
}

bool Directory::GetDataInts( const char *dataName, std::vector<int> &intVector ) const
{
    DirectoryData *data = GetData( dataName );
    
	if( data && data->IsInts() )
    {
		intVector.resize( data->m_intsLen );
		std::copy( data->m_ints, data->m_ints + data->m_intsLen, intVector.begin() );
		return true;
    }
    else
    {
        AppDebugOut( "Void data not found : %s\n", dataName );
        return false;
    }
}


DirectoryData *Directory::GetData ( const char *dataName ) const
{
    for ( int i = 0; i < m_data.Size(); ++i )
    {
        if ( m_data.ValidIndex(i) )
        {
            DirectoryData *data = m_data[i];
            AppAssert (data);
            if ( strcmp(data->m_name, dataName) == 0 )
            {
                return data;
            }
        }
    }

    return NULL;
}


void Directory::CreateData ( const char *dataName, DArray <int> *darray )
{
    if ( GetData( dataName ) )
    {
        AppReleaseAssert(false, "Attempted to create duplicate data %s at location\n%s", dataName, m_name);
    }
    else
    {
        Directory *directory = AddDirectory ( dataName );
        AppAssert( directory );
        directory->CreateData( "tID", "DArray<int>" );
        directory->CreateData( "Size", darray->Size() );

        for ( int i = 0; i < darray->Size(); ++i )
        {
            char indexName[16];
            sprintf ( indexName, "[i %d]", i );
            Directory *thisDir = directory->AddDirectory( indexName );

            if ( darray->ValidIndex(i) ) 
            {
                thisDir->CreateData( "[Used]", true );
                thisDir->CreateData( "[Data]", darray->GetData(i) );
            }
            else
            {               
                thisDir->CreateData( "[Used]", false );
            }
        }
    }
}


void Directory::CreateData ( const char *dataName, LList <Directory *> *llist )
{
    if ( GetData( dataName ) )
    {
        AppReleaseAssert(false,"Attempted to create duplicate data %s at location\n%s", dataName, m_name);
    }
    else
    {
        Directory *directory = AddDirectory ( dataName );
        AppAssert( directory );
        directory->CreateData( "tID", "LList<Directory *>" );
        directory->CreateData( "Size", llist->Size() );

        for ( int i = 0; i < llist->Size(); ++i )
        {
            char indexName[16];
            sprintf ( indexName, "[i %d]", i );
            Directory *newDir = directory->AddDirectory( indexName );
            Directory *obj = llist->GetData(i);
            AppAssert( obj );
            newDir->AddDirectory( obj );
        }
    }
}


void Directory::CreateData ( const char *dataName, LList<int> *llist )
{
    if( GetData( dataName ) )
    {
        AppReleaseAssert(false, "Attempted to create duplicate data %s at location\n%s", dataName, m_name);
    }
    else
    {
        Directory *directory = AddDirectory ( dataName );
        AppAssert( directory );
        directory->CreateData( "tID", "LList<int>" );
        directory->CreateData( "Size", llist->Size() );

        for ( int i = 0; i < llist->Size(); ++i )
        {
            char indexName[16];
            sprintf ( indexName, "[i %d]", i );
            int data = llist->GetData(i);
            directory->CreateData( indexName, data );
        }
    }
}


void Directory::GetDataDArray ( const char *dataName, DArray<int> *darray ) const
{
    AppAssert( dataName );
    AppAssert( darray );

    //
    // First try to find a directory representing this DArray

    Directory *dir = GetDirectory( dataName );
    if ( !dir )
    {
        AppReleaseAssert(false, "Failed to find DArray named %s", dataName );
        return;
    }

    //
    // Get info about the DArray

    char *tID = dir->GetDataString("tID");
    int size = dir->GetDataInt("Size");
    if ( strcmp(tID, "DArray<int>") != 0 || size == DIRECTORY_SAFEINT)
    {
        AppDebugOut("Found object %s but it doesn't appear to be a DArray\n" );
    }

    //
    // Iterate through it, loading data
    
    darray->SetSize( size );
    for ( int i = 0; i < size; ++i )
    {
        char indexName[16];
        sprintf ( indexName, "[i %d]", i );
        Directory *thisIndex = dir->GetDirectory( indexName );
        if ( !thisIndex )
        {
            AppDebugOut("Error loading DArray %s : index %d is missing\n", dataName, i );
        }
        else
        {
            bool used = thisIndex->GetDataBool("[Used]");
            
            if ( !used )
            {
                if ( darray->ValidIndex(i) )
                    darray->RemoveData(i);
            }
            else
            {
                int theData = thisIndex->GetDataInt("[Data]");
                darray->PutData( theData, i );
            }
        }
    }
}


void Directory::GetDataLList ( const char *dataName, LList<Directory *> *llist ) const
{
    AppAssert( dataName );
    AppAssert( llist );

    //
    // First try to find a directory representing this LList

    Directory *dir = GetDirectory( dataName );
    if ( !dir )
    {
        AppDebugOut("Failed to find LList named %s\n", dataName);
        return;
    }

    //
    // Get info about the LList

    char *tID = dir->GetDataString("tID");
    int size = dir->GetDataInt("Size");
    if ( strcmp(tID, "LList<Directory *>") != 0 || size == DIRECTORY_SAFEINT)
    {
        AppDebugOut("Found object %s but it doesn't appear to be a LList\n");
    }

    //
    // Iterate through it, loading data
    
    for ( int i = 0; i < size; ++i )
    {
        char indexName[16];
        sprintf ( indexName, "[i %d]", i );
        Directory *thisIndex = dir->GetDirectory( indexName );
        if ( !thisIndex )
        {
            AppDebugOut("Error loading LList %s : index %d is missing\n", dataName, i);
        }
        else
        {
            if( thisIndex->m_subDirectories.ValidIndex(0) )
            {
                Directory *theDir = thisIndex->m_subDirectories[0];
                llist->PutData( theDir );
            }
            else
            {
                AppDebugOut("Error loading LList %s : could not find directory in index %d\n", dataName, i);
            }
        }
    }
}


void Directory::GetDataLList( const char *dataName, LList<int> *llist ) const
{
    AppAssert( dataName );
    AppAssert( llist );

    //
    // First try to find a directory representing this LList

    Directory *dir = GetDirectory( dataName );
    if ( !dir )
    {
        AppDebugOut("Failed to find LList %s\n", dataName);
        return;
    }

    //
    // Get info about the LList

    char *tID = dir->GetDataString("tID");
    int size = dir->GetDataInt("Size");
    if ( strcmp(tID, "LList<int>") != 0 || size == DIRECTORY_SAFEINT)
    {
        AppDebugOut("Found object %s but it doesn't appear to be a LList\n");
    }

    //
    // Iterate through it, loading data
    
    for ( int i = 0; i < size; ++i )
    {
        char indexName[16];
        sprintf ( indexName, "[i %d]", i );
        int theData = dir->GetDataInt("indexName");
        llist->PutDataAtEnd( theData );
    }
}


void Directory::RemoveData( const char *dataName )
{
    for( int i = 0; i < m_data.Size(); ++i )
    {
        if( m_data.ValidIndex(i) )
        {
            DirectoryData *data = m_data[i];
            if( strcmp( data->m_name, dataName ) == 0 )
            {
                m_data.RemoveData(i);                
                delete data;
            }
        }
    }
}


bool Directory::HasData( const char *dataName, int _mustBeType ) const
{
    //
    // Look for a directory first

    Directory *dir = GetDirectory(dataName);
    if( dir && _mustBeType==-1 ) return true;


    //
    // Look for data

    DirectoryData *data = GetData(dataName);
    
    if( !data ) return false;

    if( _mustBeType == -1 ) return true;

    if( _mustBeType == data->m_type ) return true;

    return false;
}

static void WriteIndent( std::ostream &o, int indent )
{
	for ( int t = 0; t < indent; ++t )
		o.put(' ');
}

void Directory::WriteXML ( std::ostream &o, int indent ) const
{
    //
    // Print our name

	WriteIndent(o, indent);
	o << '<' << m_name << ">\n";
	
    //
    // Print our data	

    for ( int j = 0; j < m_data.Size(); ++j )
    {
        if ( m_data.ValidIndex(j) )
        {	
            DirectoryData *data = m_data[j];
            AppAssert( data );
			data->WriteXML( o, indent + 1 );
		}
	}

    //
    // Recurse into subdirs
	
    for ( int i = 0; i < m_subDirectories.Size(); ++i )
    {
        if ( m_subDirectories.ValidIndex(i) )
        {
            Directory *subDir = m_subDirectories[i];
            AppAssert( subDir );
            subDir->WriteXML( o, indent + 1 );        
        }
    }	
	
	o << "</" << m_name << ">\n";
}
	
void Directory::DebugPrint ( int indent ) const
{
    //
    // Print our name

    for ( int t = 0; t < indent; ++t )
    {
        AppDebugOut( "    " );
    }
    AppDebugOut ( "+===" );
    AppDebugOut ( "%s\n", m_name );

    //
    // Print our data

    for ( int j = 0; j < m_data.Size(); ++j )
    {
        if ( m_data.ValidIndex(j) )
        {
            DirectoryData *data = m_data[j];
            AppAssert( data );
            data->DebugPrint( indent + 1 );
        }
    }

    //
    // Recurse into subdirs

    for ( int i = 0; i < m_subDirectories.Size(); ++i )
    {
        if ( m_subDirectories.ValidIndex(i) )
        {
            Directory *subDir = m_subDirectories[i];
            AppAssert( subDir );
            subDir->DebugPrint( indent + 1 );        
        }
    }
}


bool Directory::Read ( istream &input )
{
    // 
    // Start marker

    char marker[DIRECTORY_MARKERSIZE];
    input.read( marker, DIRECTORY_MARKERSIZE );
    if( strncmp( marker, DIRECTORY_MARKERSTART, DIRECTORY_MARKERSIZE ) != 0 )
    {
        return false;
    }

    
    //
    // Our own information

    if ( m_name )
    {
        delete[] m_name;
        m_name = NULL;
    }
	m_name = ReadDynamicString( input, DIRECTORY_MAXSTRINGLENGTH, DIRECTORY_SAFESTRING );
    
    //
    // Our data
    // Indexes are not important and can be forgotten

    int numUsed = ReadPackedInt(input);
    
    if( numUsed < 0 || numUsed > DIRECTORY_MAXDIRSIZE )
    {
        return false;
    }

    for ( int d = 0; d < numUsed; ++d )
    {
        DirectoryData *data = new DirectoryData();        
        if (!data->Read( input )) 
		{
			AppDebugOut("Warning: failed to read directory data from stream\n");
			delete data;
			return false;
		}
        m_data.PutData( data );
    }

       
    // 
    // Our subdirectories

    int numSubdirs = ReadPackedInt( input );

    if( numUsed < 0 || numUsed > DIRECTORY_MAXDIRSIZE )
    {
        return false;
    }
    
    for ( int s = 0; s < numSubdirs; ++s )
    {
        Directory *newDir = new Directory();
        if( !newDir->Read( input ) ) 
		{
			AppDebugOut("Warning failed to read directory from stream\n");
			delete newDir;
			return false;
		}
        m_subDirectories.PutData( newDir );
    }

    //
    // End marker

    input.read( marker, DIRECTORY_MARKERSIZE );
    if( strncmp( marker, DIRECTORY_MARKEREND, DIRECTORY_MARKERSIZE ) != 0 )
    {
        return false;
    }


    return true;
}


void Directory::Write ( ostream &output ) const
{
    // 
    // Start marker

    output.write( DIRECTORY_MARKERSTART, DIRECTORY_MARKERSIZE );
    
    //
    // Our own information

    WriteDynamicString( output, m_name );
    
    //
    // Our data
    // Indexes are not important and can be forgotten
    
    int numUsed = m_data.NumUsed();
    WritePackedInt(output, numUsed);
    
    for ( int d = 0; d < m_data.Size(); ++d )
    {
        if ( m_data.ValidIndex(d) ) 
        {
            DirectoryData *data = m_data[d];
            AppAssert( data );
            data->Write( output );
        }
    }
    

    // 
    // Our subdirectories
    // Indexes are not important and can be forgotten

    int numSubDirs = m_subDirectories.NumUsed();
    WritePackedInt(output, numSubDirs);

    for ( int s = 0; s < m_subDirectories.Size(); ++s )
    {
        if ( m_subDirectories.ValidIndex(s) )
        {
            Directory *subDir = m_subDirectories[s];
            AppAssert( subDir );
            subDir->Write( output );
        }
    }

    //
    // End marker

    output.write( DIRECTORY_MARKEREND, DIRECTORY_MARKERSIZE );
}

void HexDumpData( const char *input, int length, int highlight )
{
	const int linewidth = 16;

	for (int i = 0; i < length; i += linewidth ) {
		for (int j = 0; j < linewidth; j++) {
			if (i + j < length) {
				unsigned char ch = (unsigned char)( input[i + j] );

				AppDebugOut("%c", (i + j == highlight) ? '[' : ' ');				
				AppDebugOut("%c", isprint(ch) ? ch : ' ');
				AppDebugOut("%c", (i + j == highlight) ? ']' : ' ');				
			}
			else
				AppDebugOut("   ");
		}
		AppDebugOut("\t");

		for (int j = 0; j < linewidth; j++) {
			if (i +j < length) {
				unsigned char ch = (unsigned char)( input[i + j] );

				AppDebugOut("%c", (i + j == highlight) ? '[' : ' ');								
				AppDebugOut("%02x", ch );				
				AppDebugOut("%c", (i + j == highlight) ? ']' : ' ');				
			}
			else
				AppDebugOut("    ");
		}
		AppDebugOut("\n");
	}
}

bool Directory::Read( const char *input, int length )
{
    istrstream inputStream( input, length );
    bool result = Read( inputStream );
	
	if (!result) 
	{
		AppDebugOut( "Failed to parse directory at position %d.\n", (int) inputStream.tellg() );
		AppDebugOut( "Packet length: %d\n", length );
		AppDebugOut( "Packet data:\n" );
		HexDumpData( input, length, inputStream.tellg() );
		AppDebugOut( "\n" );
		
		istrstream inputStream2( input, length );
		Read( inputStream2 );
	}
    return result;
}


char *Directory::Write( int &length ) const
{
    ostrstream outputStream;
    Write( outputStream );
	length = outputStream.tellp();
    outputStream << '\x0';
    return outputStream.str();
}


int Directory::GetByteSize() const
{
    int result = 0;
    
    result += strlen(m_name);
    result += sizeof(m_data);
    result += sizeof(m_subDirectories);

    for( int i = 0; i < m_data.Size(); ++i )
    {
        if( m_data.ValidIndex(i) )
        {
            DirectoryData *data = m_data[i];
			result += data->GetByteSize();
        }
    }


    for( int i = 0; i < m_subDirectories.Size(); ++i )
    {
        if( m_subDirectories.ValidIndex(i) )
        {
            Directory *subDir = m_subDirectories[i];
            result += subDir->GetByteSize();
        }
    }

    return result;
}


DirectoryData::DirectoryData()
:   m_name(NULL),
    m_type(DIRECTORY_TYPE_UNKNOWN),
    m_int(-1),
    m_float(-1.0),
    m_char('?'),
    m_string(NULL),
    m_bool(false),
    m_void(NULL),
    m_voidLen(0),
	m_ints(NULL),
	m_intsLen(0),
	m_ullong(0),
    m_unicode(NULL)
{
    SetName("NewData" );
}


DirectoryData::DirectoryData( const DirectoryData &_copyMe )
:	m_name(NULL),
	m_string(NULL),
	m_void(NULL),
    m_ints(NULL),
    m_unicode(NULL)
{
	SetData( &_copyMe );
}

DirectoryData& DirectoryData::operator = ( const DirectoryData &_copyMe )
{
	SetData( &_copyMe );
	return *this;
}


DirectoryData::~DirectoryData()
{
    delete[] m_name;
    delete[] m_string;
    delete[] m_void;
	delete[] m_ints;
    delete[] m_unicode;

    m_name = NULL;
    m_string = NULL;
    m_void = NULL;
	m_ints = NULL;
    m_unicode = NULL;
}


void DirectoryData::SetName ( const char *newName )
{
    if ( m_name )
    {
        delete[] m_name;
        m_name = NULL;
    }
    m_name = newStr( newName );
}


void DirectoryData::SetData ( int newData )
{
    m_type = DIRECTORY_TYPE_INT;
    m_int = newData;
}

void DirectoryData::SetData ( unsigned long long newData )
{
	m_type = DIRECTORY_TYPE_ULLONG;
	m_ullong = newData;
}


void DirectoryData::SetData ( float newData )
{
    m_type = DIRECTORY_TYPE_FLOAT;
    m_float = newData;
}


#ifdef USE_FIXED
void DirectoryData::SetData ( Fixed newData )
{
    m_type = DIRECTORY_TYPE_FIXED;
#ifdef FLOAT_NUMERICS
    m_fixed = newData.DoubleValue();
#elif defined(FIXED64_NUMERICS)
	m_fixed = newData.m_value;
#endif
}
#endif

void DirectoryData::SetData ( char newData )
{
    m_type = DIRECTORY_TYPE_CHAR;
    m_char = newData;
}


void DirectoryData::SetData ( const char *newData )
{
    m_type = DIRECTORY_TYPE_STRING;

    if( newData /*&& strlen(newData) > 0*/ )
    {
		char *oldString = m_string;

		m_string = newStr( newData );

		delete[] oldString;
    }
}


void DirectoryData::SetData ( const void *newData, int dataLen )
{
	char *oldVoid = m_void;

    m_type = DIRECTORY_TYPE_VOID;    
    m_voidLen = dataLen;
    m_void = new char[m_voidLen];
    memcpy( m_void, newData, m_voidLen );

	delete[] oldVoid;    
}


void DirectoryData::SetData ( const int *newData, int numElems )
{
	int *oldInts = m_ints;

    m_type = DIRECTORY_TYPE_INTS;	
	m_intsLen = numElems;
	m_ints = new int[numElems];
	memcpy( m_ints, newData, numElems * sizeof(int) );

	delete[] oldInts;
}

void DirectoryData::SetData( const wchar_t *newData )
{
    wchar_t *oldUnicode = m_unicode;

    m_type = DIRECTORY_TYPE_UNICODE;
    m_unicode = newStr( newData );

    delete[] oldUnicode;
}

void DirectoryData::SetData ( bool newData )
{
    m_type = DIRECTORY_TYPE_BOOL;
    m_bool = newData;
}


void DirectoryData::SetData ( const DirectoryData *data )
{
    AppAssert( data );

    delete[] m_name;
    m_name = NULL;

	delete[] m_string;
    m_string = NULL;
	
	delete[] m_void;
    m_void = NULL;
    
	delete[] m_ints;
	m_ints = NULL;

    delete[] m_unicode;
    m_unicode = NULL;

    m_name = newStr( data->m_name );
    m_type = data->m_type;
    m_int = data->m_int;
    m_float = data->m_float;
#ifdef USE_FIXED
	m_fixed = data->m_fixed;
#endif
    m_char = data->m_char;
    m_bool = data->m_bool;
	m_ullong = data->m_ullong;

    if( data->m_string )
    {
        m_string = newStr(data->m_string);
    }

    if( data->m_void )
    {
        m_voidLen = data->m_voidLen;
        m_void = new char[m_voidLen];
        memcpy( m_void, data->m_void, m_voidLen );
    }

	if( data->m_ints )
	{
		m_intsLen = data->m_intsLen;
		m_ints = new int[m_intsLen];
		memcpy( m_ints, data->m_ints, m_intsLen * sizeof(int) );
	}

    if( data->m_unicode )
    {
        m_unicode = newStr( data->m_unicode );
    }
}

static void WriteXMLString( ostream &o, const char *x ) 
{
	while (*x) {
		const char *sep = strpbrk( x, "<>" );
		if (sep) {
			int distance = sep - x;
			if (distance) {
				o.write(x, distance);
				x += distance;
			}
			switch (*x) {
				case '<': o << "&lt;"; break;
				case '>': o << "&gt;"; break;
			}
			x++;
		}
		else {
			o << x;
			return;
		}
	}
}

static void WriteAsUTF8( ostream &o, const wchar_t *x, int amount = -1 )
{
#ifdef TARGET_OS_LINUX
	iconv_t utf32_to_utf8 = iconv_open( "UTF8", "UTF32LE" );
	if( utf32_to_utf8 == (iconv_t) -1 )
	{
		perror("Failed to open iconv from UTF32 -> UTF8" );
		return;
	}
		
	char *inBuf = reinterpret_cast<char *>( const_cast<wchar_t *>( x ) );
	size_t inBytesLeft = amount == -1 ? wcslen( x ) : amount;

	inBytesLeft *= 4; // (4 bytes per UTF32)

	while( inBytesLeft > 0 )
	{

		char buf[1024];
		char *outBuf = buf;
		size_t outBytesLeft = sizeof( buf );
		size_t result = iconv( utf32_to_utf8, &inBuf, &inBytesLeft, &outBuf, &outBytesLeft );
			
		if( result == (size_t) -1 )
		{
			perror( "Failed to convert from UTF32 -> UTF8" );
			break;
		}

		o.write( buf, sizeof( buf ) - outBytesLeft );
	}

	iconv_close( utf32_to_utf8 );
#endif
}


static void WriteXMLString( ostream &o, const wchar_t *x ) 
{
	while (*x) {
		const wchar_t *sep = wcspbrk( x, L"<>" );
		if (sep) {
			int distance = sep - x;
			if (distance) {
				WriteAsUTF8( o, x, distance );
				x += distance;
			}
			switch (*x) {
				case L'<': o << "&lt;"; break;
				case L'>': o << "&gt;"; break;
			}
			x++;
		}
		else {
			WriteAsUTF8( o, x );
			return;
		}
	}
}


void DirectoryData::WriteXML( ostream &o, int indent ) const
{
	if (m_type == DIRECTORY_TYPE_UNKNOWN)
		return;

	WriteIndent(o, indent);

	o << '<' << m_name << '>';

	AppDebugAssert(m_type != DIRECTORY_TYPE_FIXED); // fixed case not implemented
    switch ( m_type )
    {
        case DIRECTORY_TYPE_INT  :          o << m_int;            			    break;
        case DIRECTORY_TYPE_FLOAT :         o << m_float;          		        break;
	    case DIRECTORY_TYPE_CHAR  :         o << (int) ((unsigned char) m_char); break;
  	    case DIRECTORY_TYPE_STRING  :       WriteXMLString(o, m_string);    	break;
  	    case DIRECTORY_TYPE_UNICODE :       WriteXMLString(o, m_unicode);    	break;
	    case DIRECTORY_TYPE_BOOL  :         o << (m_bool ? "true" : "false"); 	break;
	    case DIRECTORY_TYPE_ULLONG :        o << m_ullong;                      break;
    }

	o << "</" << m_name << ">\n";
}


void DirectoryData::DebugPrint ( int indent ) const
{
#ifdef _DEBUG
    for ( int t = 0; t < indent; ++t )
    {
        AppDebugOut( "    " );
    }
    AppDebugOut ( "+---" );
    AppDebugOut ( "%s : ", m_name );

    switch ( m_type )
    {
        case DIRECTORY_TYPE_UNKNOWN  :      AppDebugOut ( "[UNASSIGNED]\n" );                               break;
        case DIRECTORY_TYPE_INT  :          AppDebugOut ( "%d\n", m_int );                                  break;
        case DIRECTORY_TYPE_FLOAT :         AppDebugOut ( "%f\n", m_float );                                break;
#ifdef USE_FIXED
#ifdef FLOAT_NUMERICS
		case DIRECTORY_TYPE_FIXED :         AppDebugOut ( "%f\n", m_fixed );								break;
#elif defined(FIXED64_NUMERICS)
		case DIRECTORY_TYPE_FIXED :         AppDebugOut ( "%f\n", Fixed::Fixed(m_fixed).DoubleValue() );	break;
#endif
#endif
        case DIRECTORY_TYPE_CHAR  :         AppDebugOut ( "(val=%d)\n", (int)m_char );                      break;
        case DIRECTORY_TYPE_STRING  :       AppDebugOut ( "'%s'\n", m_string );                             break;
        case DIRECTORY_TYPE_BOOL  :         AppDebugOut ( "[%s]\n", m_bool ? 
                                                            "true" : "false" );                             break;
        
		case DIRECTORY_TYPE_VOID :          
            AppDebugOut( "Raw Data, Length %d bytes", m_voidLen );        
            AppDebugOutData( m_void, m_voidLen );
            break;

		case DIRECTORY_TYPE_INTS : 
            AppDebugOut( "Int Data, Length %d bytes", m_voidLen );
            AppDebugOutData( m_ints, m_intsLen );
            break;

		case DIRECTORY_TYPE_ULLONG : 
			AppDebugOut( "%I64u\n", m_ullong );
			break;
    }
#else
    AppDebugOut( "Directory::DebugPrint is hashed out\n" );
#endif
}


bool DirectoryData::Read ( istream &input )
{
    delete[] m_name;
    m_name = NULL;
	
	if (!input)
		return false;
		
    m_name = ReadDynamicString( input, DIRECTORY_MAXSTRINGLENGTH, DIRECTORY_SAFESTRING );

	if (!input)
		return false;
		
	m_type = input.get();
    
    switch ( m_type )
    {
        case    DIRECTORY_TYPE_INT   :   ReadNetworkValue( input, m_int );						break;
        case    DIRECTORY_TYPE_FLOAT :   ReadNetworkValue( input, m_float );					break;
#ifdef USE_FIXED
		case    DIRECTORY_TYPE_FIXED :   ReadNetworkValue( input, m_fixed );					break;
#endif
        case    DIRECTORY_TYPE_CHAR  :   m_char = input.get();        break;
        case    DIRECTORY_TYPE_BOOL  :   m_bool = input.get();        break;
        
        case DIRECTORY_TYPE_STRING:
            if ( m_string )
            {
                delete[] m_string;
                m_string = NULL;
            }
            m_string = ReadDynamicString( input, DIRECTORY_MAXSTRINGLENGTH, DIRECTORY_SAFESTRING );
            break;

        case DIRECTORY_TYPE_UNICODE:
            if( m_unicode )
            {
                delete[] m_unicode;
                m_unicode = NULL;
            }
            m_unicode = ReadUnicodeString( input, DIRECTORY_MAXSTRINGLENGTH, DIRECTORY_SAFEUNICODE );
            break;
           
        case DIRECTORY_TYPE_VOID:
            delete[] m_void;
			m_void = (char *) ReadVoidData( input, &m_voidLen );
            break;


		case DIRECTORY_TYPE_INTS:
            delete[] m_ints;
			m_ints = ReadNumbersData<int>( input, &m_intsLen, DIRECTORY_MAXSTRINGLENGTH );
            break;

		case DIRECTORY_TYPE_ULLONG:  
			ReadNetworkValue( input, m_ullong );
			break;

        default:
            return false;
    }

    return true;
}


int DirectoryData::GetByteSize() const
{
	std::ostringstream s;
	Write( s );
	return s.tellp();
}


void DirectoryData::Write ( ostream &output ) const
{
    //
    // Our own information

    WriteDynamicString( output, m_name );
    
	output.put((unsigned char) m_type);
        
    switch ( m_type )
    {
        case    DIRECTORY_TYPE_INT   :   WriteNetworkValue( output, m_int );					break;
        case    DIRECTORY_TYPE_FLOAT :   WriteNetworkValue( output, m_float );					break;
#ifdef USE_FIXED
		case    DIRECTORY_TYPE_FIXED :   WriteNetworkValue( output, m_fixed );					break;
#endif
        case    DIRECTORY_TYPE_CHAR  :   output.put( m_char );				break;
        case    DIRECTORY_TYPE_BOOL  :   output.put( m_bool ? 1 : 0 );      break;
        
        case DIRECTORY_TYPE_STRING:
            WriteDynamicString(output, m_string); 
            break;

        case DIRECTORY_TYPE_UNICODE:
            WriteUnicodeString(output, m_unicode);
            break;

        case DIRECTORY_TYPE_VOID:   
            WriteVoidData(output, m_void, m_voidLen);
            break;

		case DIRECTORY_TYPE_INTS:   
			WriteNumbersData(output, m_ints, m_intsLen, DIRECTORY_MAXSTRINGLENGTH );
			break;

		case DIRECTORY_TYPE_ULLONG :  
			WriteNetworkValue( output, m_ullong );
			break;
    }
}

bool DirectoryData::operator == ( const DirectoryData &_another ) const
{
	if( m_type != _another.m_type )
		return false;

	switch( m_type )
	{
	case DIRECTORY_TYPE_INT:
		return m_int == _another.m_int;
		
	case DIRECTORY_TYPE_FLOAT:
		return m_float == _another.m_float;
		
#ifdef USE_FIXED
	case DIRECTORY_TYPE_FIXED:
		return m_fixed == _another.m_fixed;
#endif

	case DIRECTORY_TYPE_CHAR:
		return m_char == _another.m_char;

	case DIRECTORY_TYPE_BOOL:
		return m_bool == _another.m_bool;

	case DIRECTORY_TYPE_STRING:
		return strcmp( m_string, _another.m_string ) == 0;

    case DIRECTORY_TYPE_UNICODE:
        return wcscmp( m_unicode, _another.m_unicode ) == 0;

	case DIRECTORY_TYPE_VOID:
		return m_voidLen == _another.m_voidLen &&
			memcmp( m_void, _another.m_void, m_voidLen ) == 0;

	case DIRECTORY_TYPE_INTS:
		return m_intsLen == _another.m_intsLen &&
			memcmp( m_ints, _another.m_ints, m_intsLen ) == 0;
		
	case DIRECTORY_TYPE_ULLONG:
		return m_ullong == _another.m_ullong;		

	default:
		return false;
	}
}
