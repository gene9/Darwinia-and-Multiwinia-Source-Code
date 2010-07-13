#ifndef INCLUDED_SOUND_FILTER_H
#define INCLUDED_SOUND_FILTER_H


//*****************************************************************************
// Class DspEffect
//*****************************************************************************

class DspEffect 
{
protected:
	int		m_sampleRate;

public:
	DspEffect(int _sampleRate): m_sampleRate(_sampleRate) {}
	virtual ~DspEffect() {};

    virtual void SetParameters  ( float const *_params ) = 0;

	// _data is both input and output
    virtual void Process        ( signed short *_data, unsigned int _numSamples ) = 0;
};


//*****************************************************************************
// Class DspResLowPass
//*****************************************************************************

class DspResLowPass : public DspEffect
{
protected:
    float   m_xn1;   // storage for delayed signals
	float   m_xn2;
	float   m_yn1;
	float   m_yn2;

	float   m_a0;    // coefficients
	float   m_a1;
	float   m_a2;

	float   m_b1;
	float   m_b2;

	float   m_cosOmega;
	float   m_sinOmega;

	float	m_gain;

    void CalcCoefs      ( float _frequency, float _resonance, float _gain );

public:
	DspResLowPass(int _sampleRate);

    void SetParameters  ( float const *_params );
	void Process        ( signed short *_data, unsigned int _numSamples);
};


//*****************************************************************************
// Class DspBitCrusher
//*****************************************************************************

class DspBitCrusher : public DspEffect
{
protected:
    float m_bitRate;

public:
    DspBitCrusher(int _sampleRate);

    void SetParameters  ( float const *_params );
	void Process        ( signed short *_data, unsigned int _numSamples);
   
};


//*****************************************************************************
// Class DspGargle
//*****************************************************************************

class DspGargle : public DspEffect
{
protected:
	enum
	{
		WaveTriangle,
		WaveSquare
	};

	float				m_wetDryMix;		// Percentage figure
	float				m_freq;
	float				m_phase;
	int					m_waveType;

	void ProcessTriangle(signed short *_data, unsigned int _numSamples);
	void ProcessSquare	(signed short *_data, unsigned int _numSamples);

public:
	DspGargle(int _sampleRate);

	void SetParameters	(float const *_params);
	void Process		(signed short *_data, unsigned int _numSamples);
};


//*****************************************************************************
// Class DspEcho
//*****************************************************************************

class DspEcho : public DspEffect
{
protected:
	float				*m_buffer;
	float				m_wetDryMix;		// Percentage figure
	float				m_delay;			// In ms
	float				m_attenuation;		// Amount echo attenuates by with each repeat. Range 0 to 100. 100=echo repeats forever

	int					m_currentBufferIndex;

public:
	DspEcho(int _sampleRate);
	~DspEcho();

	void SetParameters	(float const *_params);
	void Process		(signed short *_data, unsigned int _numSamples);
};


//*****************************************************************************
// Class DspReverb
//*****************************************************************************

#define NUM_REVERB_DELAY_UNITS	6
class DspReverb : public DspEffect
{
protected:
	signed short		*m_buffer;
	unsigned int		m_bufferSize;	// In samples
	int					m_currentBufferIndex;
	float				m_wetDryMix;
	int					m_delays[NUM_REVERB_DELAY_UNITS];
	float				m_decays[NUM_REVERB_DELAY_UNITS];

public:
	DspReverb(int _sampleRate);
	~DspReverb();

	void SetParameters	(float const *_params);
	void Process		(signed short *_data, unsigned int _numSamples);
};



//*****************************************************************************
// Class DspSmoother
//*****************************************************************************

class DspSmoother : public DspEffect
{
protected:
    float m_smoothFactor;

public:
    DspSmoother(int _sampleRate);

    void SetParameters	(float const *_params);
    void Process		(signed short *_data, unsigned int _numSamples);
};


#endif
