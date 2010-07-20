#include "lib/universal_include.h"
#include "lib/debug_utils.h"
#include "lib/math_utils.h"
#include "lib/profiler.h"

#include "sound/soundsystem.h"
#include "sound/sound_filter.h"

#include "app.h"

#include <string.h>


//*****************************************************************************
// Class DspResLowPass
//*****************************************************************************

// Here is the equation that we use for this filter:
// y(n) = a0*x(n) + a1*x(n-1)  + a2*x(n-2) - b1*y(n-1)  - b2*y(n-2)
DspResLowPass::DspResLowPass(int _sampleRate)
:	DspEffect(_sampleRate)
{
    m_xn1 = m_xn2 = m_yn1 = m_yn2 = 0.0f;
    m_a0 = m_a1 = m_a2 = m_b1 = m_b2 = 0.0f;

	// Initially configure as a low pass filter
	CalcCoefs(1000.0, 1.0f, 1.0f);
}


void DspResLowPass::CalcCoefs( float _frequency, float _resonance, float _gain )
{
	m_gain = _gain;

	if( _frequency < 0.00001f ) _frequency = 0.00001f;
	if( _resonance < 0.00001f ) _resonance = 0.00001f;

	float ratio = _frequency / 44100.0f;
	// Don't let frequency get too close to Nyquist or filter will blow up
	if( ratio >= 0.499f ) ratio = 0.499f;
	float omega = 2.0f * M_PI * ratio;

	m_cosOmega = (float) cos(omega);
	m_sinOmega = (float) sin(omega);
	float alpha = m_sinOmega / (2.0f * _resonance);

	float scalar = 1.0f / (1.0f + alpha);
	float omc = (1.0f - m_cosOmega);

	m_a0 = omc * 0.5f * scalar;
	m_a1 = omc * scalar;
    m_a2 = m_a0;
	m_b1 = -2.0f * m_cosOmega * scalar;
	m_b2 = (1.0f - alpha) * scalar;
}


void DspResLowPass::SetParameters( float const *_params )
{
    float frequency = expf(_params[0]/3850.0f + 4.7f);
    float resonance = expf(_params[1]/4.0f) - 1.0f;
    float gain = _params[2];
    CalcCoefs( frequency, resonance, gain );
}


// Perform core IIR filter calculation with permutation.
void DspResLowPass::Process(signed short *_data, unsigned int _numSamples)
{
	START_PROFILE( g_app->m_profiler, "DspResLowPass" );

    float xn;
	for( unsigned int i=0; i<_numSamples; ++i)
	{
		// Generate outputs by filtering inputs.
		xn = _data[i];
		m_yn2 = (m_a0 * xn) + (m_a1 * m_xn1) + (m_a2 * m_xn2) - (m_b1 * m_yn1) - (m_b2 * m_yn2);
		float result = m_yn2 * m_gain;
		if (result > 32765.0f) result = 32765.0f;
		else if (result < -32765.0f) result = -32765.0f;
		_data[i] = Round(result);

		++i;
		if (i >= _numSamples) break;

		// Permute filter operations to reduce data movement
		// Just substitute variables instead of doing m_xn1=xn, etc.
		m_xn2 = _data[i];
		m_yn1 = (m_a0 * m_xn2) + (m_a1 * xn) + (m_a2 * m_xn1) - (m_b1 * m_yn2) - (m_b2 * m_yn1);
		result = m_yn1 * m_gain;
		if (result > 32765.0f) result = 32765.0f;
		else if (result < -32765.0f) result = -32765.0f;
		_data[i] = Round(result);

		// Only move a little data
		m_xn1 = m_xn2;
		m_xn2 = xn;
	}

	// Apply a small m_impulseOscillator to filter to prevent arithmetic underflow,
	// which can cause the FPU to interrupt the CPU.
	m_yn1 += (float) 1.0E-26;

    END_PROFILE( g_app->m_profiler, "DspResLowPass" );
}



//*****************************************************************************
// Class DspBitCrusher
//*****************************************************************************

DspBitCrusher::DspBitCrusher(int _sampleRate)
:	DspEffect(_sampleRate),
    m_bitRate(16.0f)
{
}


void DspBitCrusher::SetParameters ( float const *_params )
{
    m_bitRate = _params[0];
}


void DspBitCrusher::Process( signed short *_data, unsigned int _numSamples)
{
    float valueRange = powf(2, m_bitRate);
    float stepSize = 65536.0 / valueRange;

    for( int i = 0; i < _numSamples; ++i )
    {
        float sample = (float) _data[i];
        int numSteps = sample / stepSize;
        int output = stepSize * numSteps;
        if( output < -32767 ) output = -32767;
        if( output > 32767 ) output = 32767;
        _data[i] = (signed short) output;
    }
}


//*****************************************************************************
// Class DspGargle
//*****************************************************************************

DspGargle::DspGargle(int _sampleRate)
:	DspEffect(_sampleRate),
	m_wetDryMix(50.0f),
	m_phase(0.0f),
	m_freq(20.0f),
	m_waveType(WaveTriangle)
{
}


void DspGargle::ProcessTriangle(signed short *_data, unsigned int _numSamples)
{
	// m_phase ranges from 0 to 2
	float inc = (2.0f * m_freq) / (float)m_sampleRate;
	float wet = m_wetDryMix / 100.0f;
	float dry = 1.0f - wet;

	int i = 0;
	while (1)
	{
		while (m_phase < 1.0f)
		{
			if (i >= _numSamples) return;

//			_data[i] *= m_phase;
			_data[i] = _data[i] * dry + _data[i] * m_phase * wet;
			i++;
			m_phase += inc;
		}
		while (m_phase < 2.0f)
		{
			if (i >= _numSamples) return;

//			_data[i] *= 2.0f - m_phase;
			_data[i] = _data[i] * dry + _data[i] * (2.0f - m_phase) * wet;
			i++;
			m_phase += inc;
		}
		m_phase -= 2.0f;
	}
}


void DspGargle::ProcessSquare(signed short *_data, unsigned int _numSamples)
{
	// m_phase ranges from 0 to 2
	float inc = (2.0f * m_freq) / (float)m_sampleRate;

	int i = 0;
	while (1)
	{
		while (m_phase < 1.0f)
		{
			if (i >= _numSamples) return;

			i++;
			m_phase += inc;
		}
		while (m_phase < 2.0f)
		{
			if (i >= _numSamples) return;

			_data[i] = 0.0f;
			i++;
			m_phase += inc;
		}
		m_phase -= 2.0f;
	}
}


void DspGargle::Process(signed short *_data, unsigned int _numSamples)
{
    START_PROFILE( g_app->m_profiler, "DspGargle" );

	if (m_waveType == WaveTriangle)
	{
		ProcessTriangle(_data, _numSamples);
	}
	else
	{
		ProcessSquare(_data, _numSamples);
	}

    END_PROFILE( g_app->m_profiler, "DspGargle" );
}


void DspGargle::SetParameters(float const *_params)
{
	m_wetDryMix = _params[0];
	m_freq = _params[1];
	m_waveType = *((int*)&_params[2]);
}



//*****************************************************************************
// Class DspEcho
//*****************************************************************************

DspEcho::DspEcho(int _sampleRate)
:	DspEffect(_sampleRate),
	m_currentBufferIndex(0),
	m_wetDryMix(50.0f),
	m_delay(200.0f),
	m_attenuation(50.0f)
{
	m_buffer = new float [_sampleRate * 3];

	int delayInSamples = (float)_sampleRate * (m_delay * 0.001f);
	memset(m_buffer, 0, sizeof(float) * delayInSamples);
}


DspEcho::~DspEcho()
{
	delete m_buffer;
	m_buffer = NULL;
}


void DspEcho::SetParameters(float const *_params)
{
	int oldEnd = 0.001f * m_delay * m_sampleRate;
	int newEnd = 0.001f * _params[1] * m_sampleRate;

	if (newEnd > oldEnd)
	{
		int growth = newEnd - oldEnd;
		memset(&m_buffer[oldEnd], 0, sizeof(float) * growth);
	}

	m_wetDryMix = _params[0];
	m_delay = _params[1];
	m_attenuation = _params[2];
}


void DspEcho::Process(signed short *_data, unsigned int _numSamples)
{
    START_PROFILE( g_app->m_profiler, "DspEcho" );

	DarwiniaDebugAssert(m_buffer);

	int delayInSamples = 0.001f * m_delay * m_sampleRate;
	float wetProportion = m_wetDryMix * 0.01f;
	float dryProportion = 1.0f - wetProportion;
	float attenuation = m_attenuation * 0.01f;

//	int i, j, k=0, s;
//	int const delaySize = 3 * m_sampleRate;
//
//	while (k < _numSamples)
//	{
//		for(i = m_currentBufferIndex; i < delaySize; ++i, ++k)
//		{
//			if (k >= _numSamples) break;
//
//			// Calc where to read the delayed sample data from
//			if (i >= delayInSamples)	j = i - delayInSamples;
//			else						j = i - delayInSamples + delaySize;
//
//			// Add the delayed sample to the input sample
//			s = _data[k] + m_buffer[j] * attenuation;
//
//			// Store the result in the delay buffer, and output
//			m_buffer[i] = s;
//
//			_data[k] = s;
//		}
//
//		m_currentBufferIndex = i % delaySize;
//	}
	int i = 0;
	while (i < _numSamples)
	{
		int numSamplesRemaining = _numSamples - i;
		int finalIndex = delayInSamples - 1;
		int numSamplesToTakeFromBufferThisIteration = finalIndex - m_currentBufferIndex + 1;
		if (i + numSamplesToTakeFromBufferThisIteration > numSamplesRemaining)
		{
			finalIndex = m_currentBufferIndex + numSamplesRemaining - 1;
		}

		for (int j = m_currentBufferIndex; j <= finalIndex; ++j, ++i)
		{
			float temp = _data[i];
			float result = _data[i] * dryProportion +
						   m_buffer[j] * wetProportion;
			if (result < -32766.0f)
				result = -32766.0f;
			else if (result > 32766.0f)
				result = 32766.0f;
			_data[i] = Round(result);
			m_buffer[j] = m_buffer[j] * attenuation + temp;
		}

		m_currentBufferIndex = (finalIndex + 1) % delayInSamples;
	}

    END_PROFILE( g_app->m_profiler, "DspEcho" );
}



//*****************************************************************************
// Class DspReverb
//*****************************************************************************

DspReverb::DspReverb(int _sampleRate)
:	DspEffect(_sampleRate),
	m_wetDryMix(50.0f),
	m_currentBufferIndex(0)
{
	m_delays[0] = 0.0011f* _sampleRate;
	m_delays[1] = 0.003f * _sampleRate;
	m_delays[2] = 0.011f * _sampleRate;
	m_delays[3] = 0.019f * _sampleRate;
	m_delays[4] = 0.029f * _sampleRate;
	m_delays[5] = 0.046f * _sampleRate;

	m_decays[0] = 0.36f;
	m_decays[1] = 0.25f;
	m_decays[2] = 0.34f;
	m_decays[3] = 0.23f;
	m_decays[4] = 0.32f;
	m_decays[5] = 0.21f;

	m_buffer = new signed short[3 * _sampleRate];
	memset(m_buffer, 0, 3 * _sampleRate * sizeof(signed short));
}


DspReverb::~DspReverb()
{
	delete m_buffer; m_buffer = NULL;
}


void DspReverb::SetParameters(float const *_params)
{
}


void DspReverb::Process(signed short *_data, unsigned int _numSamples)
{
	float wetProportion = m_wetDryMix * 0.01f;
	float dryProportion = 1.0f - wetProportion;

	int i, j, k=0, s;
	int const delaySize = 3 * m_sampleRate;

	while (k < _numSamples)
	{
		for(i = m_currentBufferIndex; i < delaySize; ++i, ++k)
		{
			if (k >= _numSamples) break;

			int x = 0;
			for (int unitIndex = 0; unitIndex < 6; ++unitIndex)
			{
				// Calc where to read the delayed sample data from
				if (i >= m_delays[unitIndex])	j = i - m_delays[unitIndex];
				else							j = i - m_delays[unitIndex] + delaySize;

				// Add the delayed sample to the input sample
				x += m_buffer[j] * m_decays[unitIndex];
			}

			s = _data[k] * 0.23f + x * 0.67f;
			if (s > 32766) s = 32766;
			else if (s < -32766) s = -32766;

			// Store the result in the delay buffer, and output
			m_buffer[i] = s;

			_data[k] = s;
		}

		m_currentBufferIndex = i % delaySize;
	}
}
