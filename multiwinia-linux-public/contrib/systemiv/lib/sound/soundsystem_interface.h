#ifndef _included_soundsysteminterface_h
#define _included_soundsysteminterface_h

/*
 *	This module is the 'middle-man' that sits between the SoundSystem library
 *  code and the application being written.  The SoundSystem uses this module
 *  to obtain information about the world in a generic fashion.
 * 
 *
 *  Instructions for use:
 *  - Create your own class derived from SoundSystemInterface, and write all 
 *    the virtual methods.  Pass in an instance of that derived class to the 
 *    SoundSystem when you initialise it.
 *
 *
 */


#include "lib/tosser/llist.h"
#include "lib/math/vector3.h"



class SoundObjectId
{
public:
    int m_data;

public:
    SoundObjectId();
    SoundObjectId(int _data);

    bool IsValid();
	bool operator == (SoundObjectId const &w);		
};



class SoundSystemInterface
{
public:       
    virtual bool    GetCameraPosition       ( Vector3<float> &_pos, Vector3<float> &_front, Vector3<float> &_up, Vector3<float> &_vel );

    virtual bool    ListObjectTypes         ( LList<char *> *_list );
    virtual bool    ListObjectEvents        ( LList<char *> *_list, char *_objType );

    virtual bool    DoesObjectExist         ( SoundObjectId _id );
    virtual char    *GetObjectType          ( SoundObjectId _id );
    virtual bool    GetObjectPosition       ( SoundObjectId _id, Vector3<float> &_pos, Vector3<float> &_vel );

    virtual bool    ListProperties          ( LList<char *> *_list );
    virtual bool    GetPropertyRange        ( char *_property, float *_min, float *_max );
    virtual float   GetPropertyValue        ( char *_property, SoundObjectId _id );
};




#endif

