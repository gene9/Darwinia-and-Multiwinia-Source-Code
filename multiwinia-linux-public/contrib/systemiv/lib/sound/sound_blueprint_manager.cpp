#include "lib/universal_include.h"
#include "lib/filesys/text_stream_readers.h"
#include "lib/filesys/filesys_utils.h"
#include "lib/filesys/text_file_writer.h"
#include "lib/filesys/binary_stream_readers.h"
#include "lib/filesys/file_system.h"

#include "sound_blueprint_manager.h"
#include "sound_library_3d.h"
#include "sound_instance.h"
#include "sound_sample_decoder.h"




SoundBlueprintManager::SoundBlueprintManager()
{
}


SoundBlueprintManager::~SoundBlueprintManager()
{
    m_dspBlueprints.EmptyAndDelete();
    m_sampleGroups.EmptyAndDelete();
}


SoundEventBlueprint *SoundBlueprintManager::GetBlueprint( char *_objectType )
{
    SoundEventBlueprint *result = m_blueprints.GetData(_objectType);

    if( !result )
    {
        result = new SoundEventBlueprint();
        m_blueprints.PutData( _objectType, result );
    }
    
    return result;
}


DspBlueprint  *SoundBlueprintManager::GetDspBlueprint( int _dspType )
{
    if( m_dspBlueprints.ValidIndex(_dspType) )
    {
        return m_dspBlueprints[_dspType];
    }

    return NULL;
}


void SoundBlueprintManager::ClearAll()
{
	for( int i = m_blueprints.Size() - 1; i >= 0; i-- )
	{
		if( m_blueprints.ValidIndex( i ) )
		{
			SoundEventBlueprint *blueprint = m_blueprints.GetData( i );
			blueprint->m_events.EmptyAndDelete();
		}
	}
    m_blueprints.EmptyAndDelete();

	m_dspBlueprints.EmptyAndDelete();
    m_sampleGroups.EmptyAndDelete();
}



void SoundBlueprintManager::LoadEffects()
{
    m_dspBlueprints.SetSize( SoundLibrary3d::NUM_FILTERS );
    
    TextReader *in = g_fileSystem->GetTextReader( "data/effects.txt" );
	AppReleaseAssert(in && in->IsOpen(), "Couldn't load data/effects.txt");

    while( in->ReadLine() )
    {
        if( !in->TokenAvailable() ) continue;
        char *effect = in->GetNextToken();
        AppDebugAssert( stricmp( effect, "EFFECT" ) == 0 );

        DspBlueprint *bp = new DspBlueprint();
        m_dspBlueprints.PutData( bp );
        strcpy( bp->m_name, in->GetNextToken() );

        in->ReadLine();
        char *param = in->GetNextToken();
        while( stricmp( param, "END" ) != 0 )
        {
            DspParameterBlueprint *sb = new DspParameterBlueprint();
            bp->m_params.PutData( sb );

            strcpy( sb->m_name,     param );
            sb->m_min       = atof( in->GetNextToken() );
            sb->m_max       = atof( in->GetNextToken() );
            sb->m_default   = atof( in->GetNextToken() );
            sb->m_dataType  = atoi( in->GetNextToken() );
            
            in->ReadLine();
            param = in->GetNextToken();
        }
    }

	delete in;
}


bool SoundBlueprintManager::AreBlueprintsModified()
{
    // Currently broken due to mod support
    // ie sounds_new.txt and sounds.txt coule be anywhere,
    // you just don't know anymore :)
    return false;

#ifdef SOUND_EDITOR
	SaveBlueprints("sounds_new.txt");
	bool result = AreFilesIdentical("data/sounds.txt", "data/sounds_new.txt");
	if (result == false)
	{
		SaveOnQuitWindow *saveWin = new SaveOnQuitWindow(LANGUAGEPHRASE("editor_savesettings"));
		EclRegisterWindow(saveWin);
	}
	return !result;
#else
	return false;
#endif
}


void SoundBlueprintManager::LoadBlueprints()
{    
    TextReader *in = g_fileSystem->GetTextReader( "data/sounds.txt" );
	AppReleaseAssert(in && in->IsOpen(), "Couldn't open data/sounds.txt");

    while( in->ReadLine() )
    {
        if( !in->TokenAvailable() ) continue;
        char *group = in->GetNextToken();
        char *objectName = in->GetNextToken();
        
        if( stricmp( group, "OBJECT" ) == 0 )
        {
            SoundEventBlueprint *ssb = new SoundEventBlueprint();
            m_blueprints.PutData( objectName, ssb );
            
            while( in->ReadLine() )
            {
                if( in->TokenAvailable() )
                {
                    char *word = in->GetNextToken();
                    if( stricmp( word, "END" ) == 0 ) break;
                    AppDebugAssert( stricmp( word, "EVENT"  ) == 0 );
                    ParseSoundInstanceBlueprint( in, objectName, ssb );
                }
            }            
        }
        else if( stricmp( group, "SAMPLEGROUP" ) == 0 )
        {
            SampleGroup *sampleGroup = NewSampleGroup( objectName );
            ParseSampleGroup( in, sampleGroup );
        }
    }
     
    delete in;

	// 
	// Verify the data we just loaded - make sure all the samples exist, are
	// in the right format etc.

	//LoadtimeVerify();
}


void SoundBlueprintManager::SaveBlueprints()
{
    char *_filename = "data/sounds.txt";

    TextFileWriter file( _filename, false );
     
    for( int i = 0; i < m_blueprints.Size(); ++i )
    {
        if( m_blueprints.ValidIndex(i) )
        {
            SoundEventBlueprint *ssb = m_blueprints[i];
        
            if( ssb->m_events.Size() > 0 )
            {
                file.printf( "# =========================================================\n" );                        
                file.printf( "OBJECT %s\n", m_blueprints.GetName(i) );
        
                for( int j = 0; j < ssb->m_events.Size(); ++j )
                {
                    SoundInstanceBlueprint *seb = ssb->m_events[j];
                    WriteSoundInstanceBlueprint( &file, seb );
                }

                file.printf( "END\n\n\n" );
            }
        }
    }


    for( int i = 0; i < m_sampleGroups.Size(); ++i )
    {
        if( m_sampleGroups.ValidIndex(i) )
        {
            SampleGroup *group = m_sampleGroups[i];

            file.printf( "# =========================================================\n" );
            file.printf( "SAMPLEGROUP %s\n", group->m_name );
            
            WriteSampleGroup( &file, group );

            file.printf( "END\n\n\n" );
        }
    }
}


void SoundBlueprintManager::ParseSoundInstanceBlueprint( TextReader *_in, char *_objectName, SoundEventBlueprint *_source )
{   
    char *eventName = _in->GetNextToken();
    
    SoundInstanceBlueprint *sib = new SoundInstanceBlueprint();

    _source->m_events.PutData( sib );
    
    strcpy( sib->m_eventName, eventName );
    sib->m_instance = new SoundInstance();
    sib->m_instance->SetEventName(_objectName, eventName);
    
    _in->ReadLine();
    char *fieldName = _in->GetNextToken();
    while( stricmp( fieldName, "END" ) != 0 )
    {
        if ( stricmp( fieldName, "SOUNDNAME" ) == 0 )      
		{
			char *soundName = _in->GetNextToken();
			strlwr(soundName);
            const char *extensionRemoved = RemoveExtension( soundName );
			sib->m_instance->SetSoundName( extensionRemoved );
		}
        else if ( stricmp( fieldName, "SOURCETYPE" ) == 0 )     sib->m_instance->m_sourceType       = atoi( _in->GetNextToken() );
        else if ( stricmp( fieldName, "POSITIONTYPE" ) == 0 )   sib->m_instance->m_positionType     = atoi( _in->GetNextToken() );
        else if ( stricmp( fieldName, "INSTANCETYPE" ) == 0 )   sib->m_instance->m_instanceType     = atoi( _in->GetNextToken() );
        else if ( stricmp( fieldName, "LOOPTYPE" ) == 0 )       sib->m_instance->m_loopType         = atoi( _in->GetNextToken() );
        else if ( stricmp( fieldName, "MINDISTANCE" ) == 0 )    sib->m_instance->m_minDistance      = atof( _in->GetNextToken() );
        else if ( stricmp( fieldName, "VOLUME" ) == 0 )         sib->m_instance->m_volume.Read( _in );
        else if ( stricmp( fieldName, "FREQUENCY" ) == 0 )	    sib->m_instance->m_freq.Read( _in );
        else if ( stricmp( fieldName, "ATTACK" ) == 0 )         sib->m_instance->m_attack.Read( _in );
        else if ( stricmp( fieldName, "SUSTAIN" ) == 0 )        sib->m_instance->m_sustain.Read( _in );
        else if ( stricmp( fieldName, "RELEASE" ) == 0 )        sib->m_instance->m_release.Read( _in );
        else if ( stricmp( fieldName, "LOOPDELAY" ) == 0 )      sib->m_instance->m_loopDelay.Read( _in );
        else if ( stricmp( fieldName, "EFFECT" ) == 0 )         ParseSoundEffect( _in, sib->m_instance );
        else												    AppDebugAssert( false );
        
        // This is bad, we have a looping sound that won't be attached
        // to any one object
//        AppDebugAssert( !( seb->m_instance->m_loopType &&
//                        seb->m_instance->m_positionType != SoundInstance::Type3DAttachedToObject ) );

        _in->ReadLine();
        fieldName = _in->GetNextToken();
    }
}


void SoundBlueprintManager::ParseSoundEffect( TextReader *_in, SoundInstance *_event )
{
    char *effectName = _in->GetNextToken();
    int fxType = -1;

    for( int i = 0; i < m_dspBlueprints.Size(); ++i )
    {
        DspBlueprint *seb = m_dspBlueprints[i];
        if( stricmp( seb->m_name, effectName ) == 0 )
        {
            fxType = i;
            break;
        }
    }
    
    AppDebugAssert( fxType != -1 );
    
    DspHandle *effect = new DspHandle();
    effect->m_type = fxType;

    int paramIndex = 0;
    while( true )
    {
        _in->ReadLine();
        char *paramName = _in->GetNextToken();
        if( stricmp( paramName, "END" ) == 0 ) break;        
        effect->m_params[paramIndex].Read( _in );
        ++paramIndex;        
    }
    _event->m_dspFX.PutData( effect );
}


void SoundBlueprintManager::ParseSampleGroup( TextReader *_in, SampleGroup *_group )
{
    while( true )
    {
        _in->ReadLine();
        char *paramType = _in->GetNextToken();
        if( stricmp( paramType, "END" ) == 0 )
        {
            break;
        }

        char *sample = _in->GetNextToken();
		strlwr(sample);
        char *extensionRemoved = (char *) RemoveExtension( sample );
		_group->AddSample( extensionRemoved );
    }
}


void SoundBlueprintManager::WriteSoundInstanceBlueprint( TextFileWriter *_file, SoundInstanceBlueprint *_event )
{
    AppDebugAssert( _event );
    AppDebugAssert( _event->m_instance );

    _file->printf( "\tEVENT %-20s\n"
                    "\t\tSOUNDNAME          %s\n"
                    "\t\tSOURCETYPE         %d\n"
                    "\t\tPOSITIONTYPE       %d\n"
                    "\t\tINSTANCETYPE       %d\n"
                    "\t\tLOOPTYPE           %d\n"
                    "\t\tMINDISTANCE        %2.2f\n",
                                        _event->m_eventName,
                                        _event->m_instance->m_soundName,
                                        _event->m_instance->m_sourceType,
                                        _event->m_instance->m_positionType,
                                        _event->m_instance->m_instanceType,
                                        _event->m_instance->m_loopType,
                                        _event->m_instance->m_minDistance
                                                                            );

    _event->m_instance->m_volume.Write( _file, "VOLUME", 2 );
    
    if( !_event->m_instance->m_freq.IsFixedValue( 1.0f ) )      _event->m_instance->m_freq.Write        ( _file, "FREQUENCY", 2 );
    if( !_event->m_instance->m_attack.IsFixedValue( 0.0f ) )    _event->m_instance->m_attack.Write      ( _file, "ATTACK", 2 );
    if( !_event->m_instance->m_sustain.IsFixedValue( 0.0f ) )   _event->m_instance->m_sustain.Write     ( _file, "SUSTAIN", 2 );
    if( !_event->m_instance->m_release.IsFixedValue( 0.0f ) )   _event->m_instance->m_release.Write     ( _file, "RELEASE", 2 );
    if( !_event->m_instance->m_loopDelay.IsFixedValue( 0.0f ) ) _event->m_instance->m_loopDelay.Write   ( _file, "LOOPDELAY", 2 );

    for( int i = 0; i < _event->m_instance->m_dspFX.Size(); ++i )
    {
        DspHandle *effect = _event->m_instance->m_dspFX[i];
        DspBlueprint *blueprint = m_dspBlueprints[ effect->m_type ];
        
        _file->printf( "\t\tEFFECT             %s\n", blueprint->m_name );
        int paramIndex = 0;
        while( true )
        {
            char *paramName = blueprint->GetParameter( paramIndex );            
            if( !paramName ) break;
            SoundParameter *param = &effect->m_params[ paramIndex ];
            param->Write( _file, paramName, 3 );
            ++paramIndex;
        }
        _file->printf( "\t\tEND\n" );
    }

    _file->printf( "\tEND" );
    _file->printf( "\n" );
}


void SoundBlueprintManager::WriteSampleGroup( TextFileWriter *_file, SampleGroup *_group )
{
    for( int i = 0; i < _group->m_samples.Size(); ++i )
    {
        char *sample = _group->m_samples[i];
        _file->printf( "\tSAMPLE  %s\n", sample );
    }
}



SampleGroup *SoundBlueprintManager::GetSampleGroup ( char *_name )
{
    for( int i = 0; i < m_sampleGroups.Size(); ++i )
    {
        SampleGroup *group = m_sampleGroups[i];
        if( stricmp( group->m_name, _name ) == 0 )
        {
            return group;
        }
    }

    return NULL;
}


SampleGroup *SoundBlueprintManager::NewSampleGroup ( char *_name )
{
    SampleGroup *group = new SampleGroup();
    m_sampleGroups.PutData( group );

    if( _name )
    {
        group->SetName( _name );
        return group;
    }
    else
    {
        int i = 1;
        while( true )
        {
            char nameCandidate[256];
            sprintf( nameCandidate, "newsamplegroup%d", i );
            if( !GetSampleGroup( nameCandidate ) )
            {
                group->SetName( nameCandidate );
                return group;
            }
            ++i;
        }
    }
}


bool SoundBlueprintManager::RenameSampleGroup( char *_oldName, char *_newName )
{
    //
    // Check the new name is unique

    if( GetSampleGroup( _newName ) )
    {
        return false;
    }


    //
    // Rename it

    SampleGroup *group = GetSampleGroup( _oldName );
    if( group )
    {
        group->SetName( _newName );

        //
        // Update entity blueprints
        
        for( int i = 0; i < m_blueprints.Size(); ++i )
        {
            if( m_blueprints.ValidIndex(i) )
            {
                SoundEventBlueprint *blueprint = m_blueprints[i];
                for( int j = 0; j < blueprint->m_events.Size(); ++j )
                {
                    if( blueprint->m_events.ValidIndex(j) )
                    {
                        SoundInstanceBlueprint *instanceBlueprint = blueprint->m_events[j];
                        if( instanceBlueprint &&
                            instanceBlueprint->m_instance->m_sourceType > SoundInstance::Sample &&
                            strcmp( instanceBlueprint->m_instance->m_soundName, _oldName ) == 0 )
                        {
                            instanceBlueprint->m_instance->SetSoundName( _newName );
                        }
                    }
                }
            }
        }
    }

    
    return true;
}



bool SoundBlueprintManager::IsSampleUsed(char const *_soundName)
{
    for( int i = 0; i < m_blueprints.Size(); ++i )
    {
        if( m_blueprints.ValidIndex(i) )
        {
            SoundEventBlueprint *sourceBlueprint = m_blueprints[i];
            for( int j = 0; j < sourceBlueprint->m_events.Size(); ++j )
            {
                if( sourceBlueprint->m_events.ValidIndex(j) )
                {
                    SoundInstanceBlueprint *instanceBlueprint = sourceBlueprint->m_events[j];
                    if( instanceBlueprint )
                    {
                        if( instanceBlueprint->m_instance->m_sourceType == SoundInstance::Sample &&
                            stricmp( instanceBlueprint->m_instance->m_soundName, _soundName ) == 0 )
                        {
                            return true;
                        }
                        else if( instanceBlueprint->m_instance->m_sourceType != SoundInstance::Sample )
                        {
                            SampleGroup *group = GetSampleGroup( instanceBlueprint->m_instance->m_soundName );
                            if( group )
                            {
                                for( int k = 0; k < group->m_samples.Size(); ++k )
                                {
                                    char *thisSample = group->m_samples[k];
                                    if( stricmp( thisSample, _soundName ) == 0 )
                                    {
                                        return true;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return false;
}




void SoundBlueprintManager::LoadtimeVerify()
{
/*
	// If you want to comment this out then comment out the call.
    
	//
	// Verify that the samples referred to in SampleGroups are valid

    FILE *soundErrors = fopen( "sounderrors.txt", "wt" );
    bool errorFound = false;
    
	for (int i = 0; i < m_sampleGroups.Size(); ++i)
	{
		if (!m_sampleGroups.ValidIndex(i)) continue;
		
		SampleGroup *sg = m_sampleGroups[i];
		int size = sg->m_samples.Size();
		for (int j = 0; j < size; ++j)
		{
			char *soundName = sg->m_samples[j];
			char const *err = IsSoundSourceOK(soundName);
			if(err != NULL )
            {
                fprintf( soundErrors, "%s: %s In sound group %s\n", 
							            err, soundName, sg->m_name);
                errorFound = true;
            }
		}
	}

	
	//
	// Verify that the samples referred to in SoundInstanceBlueprints are valid

	// Entities
	for (int i = 0; i < m_entityBlueprints.Size(); ++i)
	{
		if (!m_entityBlueprints.ValidIndex(i)) continue;

		SoundEventBlueprint *ssb = m_entityBlueprints[i];
		int size = ssb->m_events.Size();
		
		for (int j = 0; j < size; ++j)
		{
			SoundInstanceBlueprint *seb = ssb->m_events[j];
			SoundInstance *si = seb->m_instance;
            if ( si->m_sourceType == SoundInstance::Sample )
			{
				char const *err = IsSoundSourceOK(si->m_soundName);
				if(err != NULL )
                {
                    fprintf( soundErrors, "%s: %s In entity %s, event %s\n", 
					        err, si->m_soundName, Entity::GetTypeName(i), seb->m_eventName);
                    errorFound = true;
                }
			}
			else
			{
				// Just need to verify that the group exists here
				SampleGroup *sg = GetSampleGroup(si->m_soundName);
				if(!sg)
                {
                    fprintf( soundErrors, "Sound Group %s does not exist. "
								    "Referenced by entity %s, event %s\n",
								    si->m_soundName, Entity::GetTypeName(i), 
								    seb->m_eventName);
                    errorFound = true;
                }
			}
		}
	}

	// Buildings
	for (int i = 0; i < m_buildingBlueprints.Size(); ++i)
	{
		if (!m_buildingBlueprints.ValidIndex(i)) continue;

		SoundEventBlueprint *ssb = m_buildingBlueprints[i];
		int size = ssb->m_events.Size();
		
		for (int j = 0; j < size; ++j)
		{
			SoundInstanceBlueprint *seb = ssb->m_events[j];
			SoundInstance *si = seb->m_instance;
            if( si->m_sourceType == SoundInstance::Sample )
			{
				char const *err = IsSoundSourceOK(si->m_soundName);
				if(err != NULL)
                {
                    fprintf( soundErrors, "%s: %s In building %s, event %s\n", 
					            err, si->m_soundName, Building::GetTypeName(i), seb->m_eventName);
                    errorFound = true;
                }
			}
			else
			{
				// Just need to verify that the group exists here
				SampleGroup *sg = GetSampleGroup(si->m_soundName);
				if(!sg)
                {
                    fprintf( soundErrors, "Sound Group %s does not exist. "
								    "Referenced by building %s, event %s\n",
								    si->m_soundName, Building::GetTypeName(i), 
								    seb->m_eventName);
                    errorFound = true;
                }
			}
		}
	}

	// Others
	for (int i = 0; i < m_otherBlueprints.Size(); ++i)
	{
		if (!m_otherBlueprints.ValidIndex(i)) continue;

		SoundEventBlueprint *ssb = m_otherBlueprints[i];
		int size = ssb->m_events.Size();
		
		for (int j = 0; j < size; ++j)
		{
			SoundInstanceBlueprint *seb = ssb->m_events[j];
			SoundInstance *si = seb->m_instance;
            if( si->m_sourceType == SoundInstance::Sample )
			{
				char const *err = IsSoundSourceOK(si->m_soundName);
				if(err != NULL)
                {
                    fprintf( soundErrors, "%s: %s In Others %s, event %s\n", 
					        err, si->m_soundName, ssb->GetSoundSourceName(i), seb->m_eventName);
                    errorFound = true;
                }
			}
			else
			{
				// Just need to verify that the group exists here
				SampleGroup *sg = GetSampleGroup(si->m_soundName);
				if(!sg)
                {
                    fprintf( soundErrors, "Sound Group %s does not exist. "
								    "Referenced by \"others\" %s, event %s\n",
								    si->m_soundName, ssb->GetSoundSourceName(i), 
								    seb->m_eventName);
                    errorFound = true;
                }
			}
		}
	}


    fclose( soundErrors );
    AppReleaseAssert( !errorFound, "Errors found in sounds.txt : refer to sounderrors.txt for details" );
*/
}


char const *SoundBlueprintManager::IsSoundSourceOK(char const *_soundName)
{
#ifdef _DEBUG
	if (strchr(_soundName, ' '))
	{
		// File name contains a space
		return "Sound sample name contains spaces";
	}

	char fullPath[256] = "data/sounds/";
	strcat(fullPath, _soundName);

	SoundSampleDecoder *sound = new SoundSampleDecoder(new BinaryFileReader(fullPath));
	if (!sound)
	{
		// File does not exist
		return "Sound sample does not exist";
	}

	if (sound->m_numChannels != 1)
	{
		// File isn't mono
		return "Sound sample not mono";
	}
	delete sound;
#endif

	// Everything is dandy
	return NULL;
}




//*****************************************************************************
// Class DspBlueprint
//*****************************************************************************

char *DspBlueprint::GetParameter( int _param, float *_min, float *_max, float *_default, int *_dataType )
{
    if( m_params.ValidIndex(_param) )
    {
        DspParameterBlueprint *sb = m_params[_param];
        if( _min ) *_min = sb->m_min;
        if( _max ) *_max = sb->m_max;
        if( _default ) *_default = sb->m_default;
        if( _dataType ) *_dataType = sb->m_dataType;
        return sb->m_name;
    }

    return NULL;
}


//*****************************************************************************
// Class SampleGroup
//*****************************************************************************

void SampleGroup::SetName( char *_name )
{
    strcpy( m_name, _name );
}


void SampleGroup::AddSample( char *_sample )
{
    char *sampleCopy = strdup( _sample );
    m_samples.PutData( sampleCopy );
}


