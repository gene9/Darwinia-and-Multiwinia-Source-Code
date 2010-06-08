
/*
 * =========
 * DIRECTORY
 * =========
 *
 * A registry like data structure that can store arbitrary
 * data in a hierarchical system.
 *
 * Can be easily converted into bytestreams for network use.
 *
 */

#ifndef _included_genericdirectory_h
#define _included_genericdirectory_h

#include <iostream>

#ifdef USE_FIXED
#include "lib/math/fixed.h"
#endif

#include "lib/tosser/darray.h"
#include "lib/tosser/llist.h"
#include <vector>

class DirectoryData;
class UnicodeString;

class Directory
{
public:	// Wants to be private
    char        *m_name;
    DArray      <Directory *>       m_subDirectories;
    DArray      <DirectoryData *>   m_data;

public:
    Directory();
	Directory( const Directory &_copyMe );
    ~Directory();

	bool operator ==( const Directory &_another ) const;
	bool operator !=( const Directory &_another ) const;
	Directory& operator = ( const Directory &_copyMe );

    void SetName ( const char *newName );

	const char *GetName();
	DArray<Directory *> &GetSubDirectories();
	DArray<DirectoryData *> &GetData();

    //
    // Basic data types 

    void    CreateData      ( const char *dataName, int   value );           
    void    CreateData      ( const char *dataName, float value );     
	void	CreateData		( const char *dataName, unsigned long long value );
#ifdef USE_FIXED
	void	CreateData		( const char *dataName, Fixed value );
#endif
    void    CreateData      ( const char *dataName, unsigned char value );
    void    CreateData      ( const char *dataName, char  value );          
	void	CreateData		( const char *dataName, const char *value );
	void    CreateData      ( const char *dataName, bool  value );
    void    CreateData      ( const char *dataName, const void *data, int dataLen );
	void    CreateData      ( const char *dataName, const int *data, int numElems );
	void	CreateData		( const char *dataName, const std::vector<int> &intVector );
    void    CreateData      ( const char *dataName, const wchar_t *data );
	void	CreateData		( const char *dataName, const UnicodeString &_data );

    int             GetDataInt          ( const char *dataName ) const;                       // These are safe read functions.
    float           GetDataFloat        ( const char *dataName ) const;                       // All return some value.
#ifdef USE_FIXED
	Fixed			GetDataFixed		( const char *dataName );
#endif
    unsigned char   GetDataUChar        ( const char *dataName ) const;
    char            GetDataChar         ( const char *dataName ) const;
    char			*GetDataString      ( const char *dataName ) const;                       // You should strdup this
    bool            GetDataBool         ( const char *dataName ) const;
	unsigned long long GetDataULLong	( const char *dataName ) const;
    void            *GetDataVoid        ( const char *dataName, int *_dataLen ) const;
	int				*GetDataInts		( const char *dataName, unsigned *_numElems ) const;
	bool			GetDataInts			( const char *dataName, std::vector<int> &intVector ) const;
    wchar_t         *GetDataUnicode     ( const char *dataName ) const;
	bool			GetDataUnicodeWithStringFallback( const char *dataName, UnicodeString &_dest );
	
    void    RemoveData      ( const char *dataName );
    bool    HasData         ( const char *dataName, int _mustBeType =-1 ) const;


    //
    // Tosser data types

    void    CreateData      ( const char *dataName, DArray    <int> *darray );
    void    CreateData      ( const char *dataName, LList     <int> *llist );
    void    CreateData      ( const char *dataName, LList     <Directory *> *llist );

    void    GetDataDArray   ( const char *dataName, DArray    <int> *darray ) const;
    void    GetDataLList    ( const char *dataName, LList     <int> *llist ) const;
    void    GetDataLList    ( const char *dataName, LList     <Directory *> *llist ) const;

    
    //
    // Low level interface stuff

    Directory       *GetDirectory       ( const char *dirName ) const;
    DirectoryData   *GetData            ( const char *dataName ) const;
    Directory       *AddDirectory       ( const char *dirName );                        // Will create all neccesary subdirs recursivly
    void             AddDirectory       ( Directory *dir );
    void             RemoveDirectory    ( const char *dirName );                        // Directory is NOT deleted
    void             CopyData           ( const Directory *dir, 
                                          bool overWrite = false,                       // Overwrite existing data if found
                                          bool directories = true);                     // Copy subdirs as well

    //
    // Saving / Loading to streams

    bool Read  ( std::istream &input );                                                 // returns false if an error occurred while reading
    void Write ( std::ostream &output ) const;

    bool Read   ( const char *input, int length );
    char *Write ( int &length ) const;                                                  // Creates new string

    //
    // Other 

    void DebugPrint ( int indent ) const;
	void WriteXML ( std::ostream &o, int indent = 0 ) const;

    int  GetByteSize() const;

    static char     *GetFirstSubDir     ( const char *dirName );                      // eg returns "bob" from "bob/hello/poo". Caller must delete.
    static char     *GetOtherSubDirs    ( const char *dirName );                      // eg returns "hello/poo" from above.  Doesn't create new data.

};



/*
 * ==============
 * DIRECTORY DATA
 * ==============
 */

#define DIRECTORY_TYPE_UNKNOWN  0
#define DIRECTORY_TYPE_INT      1
#define DIRECTORY_TYPE_FLOAT    2
#define DIRECTORY_TYPE_CHAR     3
#define DIRECTORY_TYPE_STRING   4
#define DIRECTORY_TYPE_BOOL     5
#define DIRECTORY_TYPE_VOID     6
#define DIRECTORY_TYPE_FIXED    7
#define DIRECTORY_TYPE_INTS     8
#define DIRECTORY_TYPE_ULLONG	9
#define DIRECTORY_TYPE_UNICODE  10

#define DIRECTORY_SAFEINT        -1
#define DIRECTORY_SAFEFLOAT      -1.0f
#define DIRECTORY_SAFECHAR       '?'
#define DIRECTORY_SAFESTRING     "[SAFESTRING]"
#define DIRECTORY_SAFEUNICODE    L"[SAFESTRING"
#define DIRECTORY_SAFEBOOL       false
#define DIRECTORY_SAFEULLONG	 ((unsigned long long) -1)


class DirectoryData
{
public:
    char    *m_name;
    int     m_type;
    
    int     m_int;                  // type 1
    float   m_float;                // type 2
    char    m_char;                 // type 3
    char    *m_string;              // type 4
    bool    m_bool;                 // type 5
    char    *m_void;                // type 6  

	
#ifdef USE_FIXED
#ifdef FLOAT_NUMERICS
	double  m_fixed;                // type 7 (stored as raw double to guarantee memory representation)
#elif defined(FIXED64_NUMERICS)
	long long m_fixed;
#endif
#endif
    int     m_voidLen;

	int		*m_ints;				// type 8
	unsigned m_intsLen;			

	unsigned long long m_ullong;	// type 9

    wchar_t  *m_unicode;            // type 10

public:
    DirectoryData();
	DirectoryData( const DirectoryData &_copyMe );
    ~DirectoryData();

	bool operator == ( const DirectoryData &_another ) const;
	DirectoryData& operator = ( const DirectoryData &_copyMe );

    void SetName ( const char *newName );
    void SetData ( int newData );
	void SetData ( unsigned long long newData );
    void SetData ( float newData );
#ifdef USE_FIXED
	void SetData ( Fixed newData );
#endif
    void SetData ( char newData );
    void SetData ( const char *newData );
    void SetData ( bool newData );
    void SetData ( const void *newData, int dataLen );
	void SetData ( const int *newData, int numElems );
    void SetData ( const DirectoryData *data );
    void SetData ( const wchar_t *newData );
	

    bool IsInt() const		{ return (m_type == DIRECTORY_TYPE_INT      ); }
    bool IsFloat() const	{ return (m_type == DIRECTORY_TYPE_FLOAT    ); }
	bool IsFixed() const	{ return (m_type == DIRECTORY_TYPE_FIXED    ); }
    bool IsChar() const		{ return (m_type == DIRECTORY_TYPE_CHAR     ); }
    bool IsString() const	{ return (m_type == DIRECTORY_TYPE_STRING   ); }
    bool IsBool() const		{ return (m_type == DIRECTORY_TYPE_BOOL     ); }
    bool IsVoid() const		{ return (m_type == DIRECTORY_TYPE_VOID     ); }
	bool IsInts() const		{ return (m_type == DIRECTORY_TYPE_INTS     ); }
	bool IsULLong() const	{ return (m_type == DIRECTORY_TYPE_ULLONG   ); }
    bool IsUnicode() const  { return (m_type == DIRECTORY_TYPE_UNICODE  ); }

	int GetByteSize() const;

    void DebugPrint ( int indent ) const;
	void WriteXML ( std::ostream &o, int indent = 0 ) const;

    // Saving / Loading to streams

    bool Read  ( std::istream &input );                              // returns false if something went wrong
    void Write ( std::ostream &output ) const;

};

inline 
const char *Directory::GetName()
{
	return m_name;
}

inline
DArray<Directory *> &Directory::GetSubDirectories()
{
	return m_subDirectories;
}

inline
DArray<DirectoryData *> &Directory::GetData()
{
	return m_data;
}



#endif
