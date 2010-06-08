#ifndef INCLUDED_SOUND_PARAMETER
#define INCLUDED_SOUND_PARAMETER


class TextReader;
class FileWriter;


//*****************************************************************************
// Class SoundParameter
//*****************************************************************************

class SoundParameter
{
public:
    enum
    {
        TypeFixedValue,
        TypeRangedRandom,
        TypeLinked,
        NumParameterTypes
    };

    enum
    {
        LinkedToNothing,
        LinkedToHeightAboveGround,
        LinkedToXpos,
        LinkedToYpos,
        LinkedToZpos,
        LinkedToVelocity,
        LinkedToCameraDistance,
        NumLinkTypes
    };

    enum
    {
        UpdateConstantly,
        UpdateOnceEveryRestart,
        NumUpdateTypes
    };

    int         m_type;
    int         m_link;
    int         m_updateType;

    float       m_inputLower;
    float       m_outputLower;
    float       m_inputUpper;
    float       m_outputUpper;

    float       m_desiredOutput;

    float       m_smooth;

    float       m_input;
    float       m_output;

public:
    SoundParameter();
    SoundParameter( float _fixedValue );

    void        Copy( SoundParameter *_copyMe );

    void        Recalculate( float _input=0.0f );
    float       GetOutput();

    void        Read    ( TextReader *_in );
    void        Write   ( FileWriter *_file, char *_paramName, int _tabs );

	float		GetSmooth();	// Currently returns sqrtf(m_smooth)

    bool        IsFixedValue( float _value );

    static char *GetParameterTypeName   ( int _type );
    static int   GetParameterType       ( char *_name );

    static char *GetLinkName            ( int _type );
    static int   GetLinkType            ( char *_name );

    static char *GetUpdateTypeName      ( int _type );
    static int   GetUpdateType          ( char *_name );
};


#endif
