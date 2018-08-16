/*
  ==============================================================================

    CTAGSamplerVoice.cpp
    Created: 16 Jul 2018 10:56:49pm
    Author:  nikla

  ==============================================================================
*/

#include "CTAGSamplerVoice.h"


bool CTAGSamplerVoice::canPlaySound(SynthesiserSound* sampSound)
{
	return ((dynamic_cast<CTAGSamplerSound*>(sampSound)) != nullptr);
}


void CTAGSamplerVoice::startNote(int midiNoteNumber, float velocity, SynthesiserSound * sampSound, int pitchWheel)
{
	

	if (auto* sound = dynamic_cast<CTAGSamplerSound*> (sampSound))
	{
		
			pitchRatio = std::pow(2.0, ((midiNoteNumber - sound->midiRootNote) + pitchVal) / 12.0)
				* sound->sourceSampleRate / getSampleRate();

			sourceSamplePosition = 0.0;
			lgain = velocity;
			rgain = velocity;

			env.startEG();
		
		
		
	}
	else
	{
		jassertfalse; // this object can only play SamplerSounds!
	}
	
}

void CTAGSamplerVoice::pitchWheelMoved(int newValue)
{
}

void CTAGSamplerVoice::controllerMoved(int controllerNumber, int newValue)
{
}

void CTAGSamplerVoice::stopNote(float velocity, bool allowTailOff)
{
		
		if (allowTailOff)
		{
			if (env.canNoteOff())
				env.noteOff();
		}
		else if(!allowTailOff && velocity == 0.0f)
		{
			env.shutDown();
			clearCurrentNote();
			
		}

		else if (!allowTailOff && velocity == 1.0f)
		{
			
			if (env.canNoteOff())
				env.noteOff();
		}
			
	

}

void CTAGSamplerVoice::renderNextBlock(AudioBuffer< float > &outputBuffer, int startSample, int numSamples)
{
	
	
	if (auto* playingSound = dynamic_cast<CTAGSamplerSound*> (getCurrentlyPlayingSound().get()))
	{
		auto& data = *playingSound->data;
		const float* const inL = data.getReadPointer(0);
		const float* const inR = data.getNumChannels() > 1 ? data.getReadPointer(1) : nullptr;

		float* outL = outputBuffer.getWritePointer(0, startSample);
		float* outR = outputBuffer.getNumChannels() > 1 ? outputBuffer.getWritePointer(1, startSample) : nullptr;

		while (--numSamples >= 0)
		{
			auto pos = (int)sourceSamplePosition;
			auto alpha = (float)(sourceSamplePosition - pos);
			auto invAlpha = 1.0f - alpha;
			float envVal = 0.0f;
			// just using a very simple linear interpolation here..
			float l = (inL[pos] * invAlpha + inL[pos + 1] * alpha);
			float r = (inR != nullptr) ? (inR[pos] * invAlpha + inR[pos + 1] * alpha)
				: l;

			//Envelope
			envVal = env.doEnvelope();
			l *= envVal;
			r *= envVal;

			

			//WaveShaper
			setWaveShaperSymmetrical(shaperAmp.getNextValue());
			l = shaper.processSample(l);
			r = shaper.processSample(r);

			//Filter
			filter.update();
			l = filter.doFilter(l);
			r = filter.doFilter(r);


			if (outR != nullptr)
			{
				*outL++ += l;
				*outR++ += r;
			}
			else
			{
				*outL++ += (l + r) * 0.5f;
			}

			sourceSamplePosition += pitchRatio;
			
				
			if (env.getState() == 0 || sourceSamplePosition > playingSound->length)
			{
				clearCurrentNote();
				break;
			}

			
		}
		
	}

}

void CTAGSamplerVoice::setMidiNote(int note) { midiNote.setBit(note); }

bool CTAGSamplerVoice::canPlayCTAGSound(int note) const { return midiNote[note]; }


void CTAGSamplerVoice::parameterChanged(const String &parameterID, float newValue) 
{
	if (parameterID == String("ampEnvAttack" + String(index)))
	{
		setEnvelopeAttack(newValue);

	}
	if (parameterID == String("ampEnvDecay" + String(index)))
	{
		setEnvelopeDecay(newValue);

	}
	if (parameterID == String("ampEnvSustain" + String(index)))
	{
		setEnvelopeSustain(newValue);

	}
	if (parameterID == String("ampEnvRelease" + String(index)))
	{
		setEnvelopeRelease(newValue);

	}
	if (parameterID == String("Filter ON/OFF" + String(index)))
	{
		setFilterActive(newValue);
	}
	if (parameterID == String("filterCutoff" + String(index)))
	{
		setCutoffFreq(newValue);
	}
	if (parameterID == String("Distortion ON/OFF" + String(index)))
	{
		setWaveShaperActive(newValue);
	}
	if (parameterID == String("distortionVal" + String(index)))
	{
		shaperAmp.setValue(newValue);
		
	}
	if (parameterID == String("pitchVal" + String(index)))
	{
		pitchVal = newValue;
	}
}