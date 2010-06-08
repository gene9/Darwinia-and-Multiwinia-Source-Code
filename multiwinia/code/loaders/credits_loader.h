
#ifndef _included_creditsloader_h
#define _included_creditsloader_h

#include "lib/tosser/darray.h"
#include "lib/vector2.h"
#include "loaders/loader.h"

#include "lib/unicode/unicode_string.h"

class CreditsItem;
class CreditsDarwinian;


class CreditsLoader : public Loader
{
public:
    DArray<CreditsItem *>       m_items;
    DArray<CreditsDarwinian *>  m_darwinians;

    int m_sceneIndex;

public:
    void GenerateChars          ( float _x, float _y, float _size, const UnicodeString &_string );
    void GenerateCharsCentre    ( float _x, float _y, float _size, const UnicodeString &_string );
    void GenerateWords          ( float _x, float _y, float _size, const UnicodeString &_string );
    void GenerateWordsCentre    ( float _x, float _y, float _size, const UnicodeString &_string );
    int  AddString              ( float _x, float _y, float _size, const UnicodeString &_string );

    void GenerateBetaTesterList ();
#ifdef EGAMESBUILD
    void GenerateEGamesList();
#endif

    void RemoveUnusedStrings    ();             // Takes them offscreen
    void DeleteUnusedStrings    ();             // Deletes them

    void Advance( float _time );
    void Render();
    void RenderDarwinian( float _x, float _y );

    void FlipBuffers( float alpha );

public:
    CreditsLoader();

    bool    CreateNextScene();
    Vector2 GenerateOffscreenPosition( float _distanceOut );
    bool    IsOffscreen( Vector2 const &_pos );

    void Run();
};



class CreditsItem 
{
public:
    UnicodeString   m_string;
    Vector2 m_pos;
    Vector2 m_targetPos;
    float   m_size;
    bool    m_used;

    void    SetString( const UnicodeString &_string ) { m_string = _string; }
};


class CreditsDarwinian
{
public:
    Vector2 m_pos;
    Vector2 m_targetPos;
    int     m_itemId;
    
    enum
    {
        StateWalkingToItem,
        StateCarryingItem,
        StateLeaving
    };
    int     m_state;

public:
    bool    AdvanceToTargetPos( float _time );
    bool    Advance( float _time );
};


#endif
