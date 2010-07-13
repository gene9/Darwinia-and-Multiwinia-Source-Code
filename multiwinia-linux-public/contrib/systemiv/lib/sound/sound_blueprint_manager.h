#ifndef _included_soundblueprintmanager_h
#define _included_soundblueprintmanager_h


/*
 *	Loads blueprints for Sound Events from sounds.txt
 * 
 *  Stores all of those blueprints and allows them to be
 *  easily recalled, changed etc
 *
 */

#include "lib/tosser/hash_table.h"
#include "lib/tosser/darray.h"
#include "lib/tosser/llist.h"

class SoundInstance;
class SoundEventBlueprint;
class SoundInstanceBlueprint;
class DspBlueprint;
class SampleGroup;
class TextReader;
class TextFileWriter;


//*****************************************************************************
// Class SoundBlueprintManager
//*****************************************************************************

class SoundBlueprintManager
{
public:
    HashTable       <SoundEventBlueprint *>     m_blueprints;           // Indexed on Object Name 
    DArray          <DspBlueprint *>            m_dspBlueprints;        // Indexed on SoundLibrary3d::FX types    
    LList           <SampleGroup *>             m_sampleGroups;

protected:
    void ParseSoundInstanceBlueprint    ( TextReader *_in, char *_objectName, SoundEventBlueprint *_source );
    void WriteSoundInstanceBlueprint    ( TextFileWriter *_file, SoundInstanceBlueprint *_event );
    void ParseSampleGroup               ( TextReader *_in, SampleGroup *_group );
    void WriteSampleGroup               ( TextFileWriter *_file, SampleGroup *_group );
    void ParseSoundEffect		        ( TextReader *_in, SoundInstance *_event );

public:
    SoundBlueprintManager();
    ~SoundBlueprintManager();

    void LoadEffects			();
    void LoadBlueprints			();
    void SaveBlueprints			();
	bool AreBlueprintsModified	();

    void ClearAll();

    SampleGroup *GetSampleGroup     ( char *_name );
    SampleGroup *NewSampleGroup     ( char *_name=NULL );
    bool        RenameSampleGroup   ( char *_oldName, char *_newName );

    bool IsSampleUsed(char const *_soundName);

    SoundEventBlueprint *GetBlueprint     ( char *_objectType );                // Will create if required
    DspBlueprint        *GetDspBlueprint  ( int _dspType );

	void LoadtimeVerify			();												// Verifies that the data load from sounds.txt is OK
	char const *IsSoundSourceOK	(char const *_soundName);						// Tests that file names and file formats are OK, returns an error code from the SoundSource enum
};



//*****************************************************************************
// Class SoundInstanceBlueprint
//*****************************************************************************

class SoundInstanceBlueprint
{
public:
    char m_eventName[256];
    SoundInstance *m_instance;
};


//*****************************************************************************
// Class SoundEventBlueprint
//*****************************************************************************

class SoundEventBlueprint
{   
public:
    LList<SoundInstanceBlueprint *> m_events;
};


//*****************************************************************************
// Class DspParameterBlueprint
//*****************************************************************************

class DspParameterBlueprint
{
public:
    char    m_name[256];
    float   m_min;
    float   m_max;
    float   m_default;
    int     m_dataType;                     // 0 = float, 1 = long
};


//*****************************************************************************
// Class DspBlueprint
//*****************************************************************************

class DspBlueprint
{
public:
    char    m_name[256];
    DArray  <DspParameterBlueprint *> m_params;

public:
    char *GetParameter   ( int _param, float *_min=NULL, float *_max=NULL, float *_default=NULL, int *_dataType=NULL );
};


//*****************************************************************************
// Class SampleGroup
//*****************************************************************************

class SampleGroup
{
public:
    char    m_name[256];
    LList   <char *> m_samples;

public:
    void    SetName         ( char *_name );
    void    AddSample       ( char *_sample );
};



#endif
