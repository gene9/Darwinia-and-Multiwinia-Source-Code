#include "lib/universal_include.h"
#include "soundsystem_interface.h"


SoundObjectId::SoundObjectId()
:   m_data(-1)
{
}


SoundObjectId::SoundObjectId(int _data)
{
    m_data = _data;
}


bool SoundObjectId::IsValid()
{
    return( m_data != -1 );
}


bool SoundObjectId::operator == (SoundObjectId const &w)
{
	return( m_data == w.m_data );
}



bool SoundSystemInterface::ListObjectTypes( LList <char *> *_list )
{
    return false;
}


bool SoundSystemInterface::ListObjectEvents( LList <char *> *_list, char *_objType )
{
    return false;
}


bool SoundSystemInterface::GetCameraPosition( Vector3<float> &_pos, Vector3<float> &_front, Vector3<float> &_up, Vector3<float> &_vel )
{
    return false;
}


bool SoundSystemInterface::DoesObjectExist( SoundObjectId _id )
{
    return false;
}


char *SoundSystemInterface::GetObjectType( SoundObjectId _id )
{
    return NULL;
}


bool SoundSystemInterface::ListProperties( LList <char *> *_list )
{
    return false;
}


float SoundSystemInterface::GetPropertyValue( char *_property, SoundObjectId _id )
{
    return 0.0f;
}


bool SoundSystemInterface::GetPropertyRange( char *_property, float *_min, float *_max )
{
    *_min = 0.0f;
    *_max = 0.0f;

    return false;
}


bool SoundSystemInterface::GetObjectPosition( SoundObjectId _id, Vector3<float> &_pos, Vector3<float> &_vel )
{
    return false;
}
