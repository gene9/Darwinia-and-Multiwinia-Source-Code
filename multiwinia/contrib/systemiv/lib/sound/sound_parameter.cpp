#include "lib/universal_include.h"

#include <string.h>

#include "lib/tosser/llist.h"
#include "lib/filesys/text_stream_readers.h"
#include "lib/filesys/text_file_writer.h"
#include "lib/math/random_number.h"
#include "lib/debug_utils.h"

#include "soundsystem.h"
#include "sound_parameter.h"


SoundParameter::SoundParameter()
:   m_type( TypeFixedValue ),
    m_link( -1 ),
    m_updateType( UpdateConstantly ),
    m_inputLower(0.0f),
    m_outputLower(0.0f),
    m_inputUpper(0.0f),
    m_outputUpper(0.0f),
    m_input(0.0f),
    m_output(0.0f),
    m_desiredOutput(0.0f),
    m_smooth(0.0f)
{
}


SoundParameter::SoundParameter( float _fixedValue )
:   m_type( TypeFixedValue ),
    m_link( -1 ),
    m_updateType( UpdateConstantly ),
    m_inputLower(0.0f),
    m_outputLower(0.0f),
    m_inputUpper(0.0f),
    m_outputUpper(0.0f),
    m_input(0.0f),
    m_output(0.0f),
    m_desiredOutput(0.0f),
    m_smooth(0.0f)
{
    m_outputLower = _fixedValue;
}


void SoundParameter::Copy( SoundParameter *_copyMe )
{
    m_type          = _copyMe->m_type;
    m_link          = _copyMe->m_link;
    m_updateType    = _copyMe->m_updateType;
    m_inputLower    = _copyMe->m_inputLower;
    m_inputUpper    = _copyMe->m_inputUpper;
    m_outputLower   = _copyMe->m_outputLower;
    m_outputUpper   = _copyMe->m_outputUpper;
    m_smooth     = _copyMe->m_smooth;    
}

void SoundParameter::Recalculate( float _input )
{
    m_input = _input;
    
    switch( m_type )
    {
        case TypeFixedValue:
        {
            m_output = m_outputLower;
            break;
        }

        case TypeRangedRandom:
        {
            float diff = m_outputUpper - m_outputLower;
            float random = frand( diff );
            m_desiredOutput = m_outputLower + random;
			float smooth = GetSmooth();
            m_output = m_desiredOutput * (1.0f-smooth) + m_output * smooth;
            break;
        }

        case TypeLinked:
        {
            if( _input <= m_inputLower &&
                _input <= m_inputUpper ) 
            {
                m_desiredOutput = m_inputLower < m_inputUpper ? m_outputLower : m_outputUpper;
            }
            else if( _input >= m_inputUpper &&
                     _input >= m_inputLower )
            {
                m_desiredOutput = m_inputLower < m_inputUpper ? m_outputUpper : m_outputLower;
            }
            else
            {
                float inputFraction = (_input - m_inputLower) / (m_inputUpper - m_inputLower);
                m_desiredOutput = m_outputLower + inputFraction * (m_outputUpper - m_outputLower);
            }
			float smooth = GetSmooth();
            m_output = m_desiredOutput * (1.0f-smooth) + m_output * smooth;
            break;
        }
    }
}


float SoundParameter::GetOutput()
{
    return m_output;
}


void SoundParameter::Read( TextReader *_in )
{
    char *parameter = _in->GetNextToken();
    AppDebugAssert( stricmp( parameter, "PARAMETER" ) == 0 );

    char *paramType     = _in->GetNextToken();
    m_type              = GetParameterType( paramType );

    switch( m_type )
    {
        case TypeFixedValue:
            m_outputLower       = atof( _in->GetNextToken() );    
            break;

        case TypeRangedRandom:
            m_outputLower       = atof( _in->GetNextToken() );
            m_outputUpper       = atof( _in->GetNextToken() );
            m_smooth            = atof( _in->GetNextToken() );
            break;

        case TypeLinked:            
            m_inputLower        = atof( _in->GetNextToken() );
            m_outputLower       = atof( _in->GetNextToken() );
            m_inputUpper        = atof( _in->GetNextToken() );
            m_outputUpper       = atof( _in->GetNextToken() );
            m_smooth            = atof( _in->GetNextToken() );
            char *linkType      =       _in->GetNextToken();
            m_link = GetLinkType( linkType );
            break;
    }

    char *updateType = _in->GetNextToken();
    m_updateType = GetUpdateType( updateType );

    Recalculate();
}


void SoundParameter::Write( TextFileWriter *_file, char *_paramName, int _tabs )
{
    for( int i = 0; i < _tabs; ++i )
    {
        _file->printf( "\t" );
    }

    _file->printf( "%-18s PARAMETER %-18s",
                            _paramName,
                            GetParameterTypeName( m_type ) );
    
    switch( m_type )
    {
        case TypeFixedValue:
            _file->printf( "%8.2f", m_outputLower );
            break;

        case TypeRangedRandom:
            _file->printf( "%8.2f %8.2f %8.2f", m_outputLower, m_outputUpper, m_smooth );
            break;

        case TypeLinked:
            _file->printf( "%8.2f %8.2f %8.2f %8.2f %8.2f  %s", 
                            m_inputLower, m_outputLower,
                            m_inputUpper, m_outputUpper,
                            m_smooth,
                            GetLinkName( m_link ) );
            break;
    }

    _file->printf( "  %s", GetUpdateTypeName( m_updateType ) );
    _file->printf( "\n" );
}


bool SoundParameter::IsFixedValue( float _value )
{
    return( m_type == TypeFixedValue && m_outputLower == _value );
}


float SoundParameter::GetSmooth()
{
	return sqrtf(m_smooth);
}


char *SoundParameter::GetParameterTypeName( int _type )
{
    char *names[] = {   
                        "TypeFixedValue",
                        "TypeRangedRandom",
                        "TypeLinked"
                    };

    AppDebugAssert( _type >= 0 && _type < NumParameterTypes );
    return names[_type];
}

int SoundParameter::GetParameterType( char *_name )
{
    for( int i = 0; i < NumParameterTypes; ++i )
    {
        if( stricmp( GetParameterTypeName(i), _name ) == 0 )
        {
            return i;
        }
    }
    return -1;
}


char *SoundParameter::GetUpdateTypeName( int _type )
{
    char *names[] = { 
                        "UpdateConstantly",
                        "UpdateOncePerLoop"
                    };

    AppDebugAssert( _type >= 0 && _type < NumUpdateTypes );
    return names[ _type ];
}


int SoundParameter::GetUpdateType( char *_name )
{
    for( int i = 0; i < NumUpdateTypes; ++i )
    {
        if( stricmp( GetUpdateTypeName(i), _name ) == 0 )
        {
            return i;
        }
    }
    return -1;
}



char *SoundParameter::GetLinkName( int _type )
{
    if( _type == -1 ) return "Nothing";

    LList<char *> properties;
    g_soundSystem->m_interface->ListProperties( &properties );

    AppDebugAssert( _type >= 0 && _type < properties.Size() );
    return properties[_type];
}


int SoundParameter::GetLinkType( char *_name )
{
    LList<char *> properties;
    g_soundSystem->m_interface->ListProperties( &properties );

    for( int i = 0; i < properties.Size(); ++i )
    {
        if( stricmp( properties[i], _name ) == 0 )
        {
            return i;
        }
    }

    return -1;
}
