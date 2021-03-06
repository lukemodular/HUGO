/* 
 * *** Sequences.h ****
 *
 * created by Lukas Vogel 15.04.14
 * 
 */
 
#ifndef Sequences_h_
#define Sequences_h_

#include <vector>

//------------------------------------------------
//------------------ Tone Volume -----------------
//------------------------------------------------

extern AudioMixer4 mixSources;

class ToneVolumeSeq
{
public:
	ToneVolumeSeq(int* _seq, int _seqLength)
		:seq(_seq),
		pSeq(_seq),
		seqLength(_seqLength)
	{}
	~ToneVolumeSeq() {}
	
	void setInterpolation(float _nextGain_dB)
	{
		gain_dB_step = (_nextGain_dB - gain_dB) / (float)stepCounter;
	}
	
	void seqProceed()
	{
		
		if (stepCounter == 0)
		{
			gain_dB = *pSeq++;
			float gain = pow(10, (float)gain_dB/20.0);
			mixSources.gain(1, gain);
			mixSources.gain(2, gain);
			mixSources.gain(3, gain);
			
			stepCounter = ((*pSeq++) / (MOD_RATE)) * (1.0/speed);
			
			if(seqCounter+1 < seqLength)
				pSeqNext = pSeq;
			else
				pSeqNext = seq;
				
			float nextGain_dB= *pSeqNext;
			setInterpolation(nextGain_dB);

		}
		else
		{
			gain_dB = gain_dB + gain_dB_step;
			float gain = pow(10, (float)gain_dB/20.0);
			mixSources.gain(1, gain);
			mixSources.gain(2, gain);
			mixSources.gain(3, gain);
			
			stepCounter--;
			if(stepCounter == 0)
				seqCounter++;
		}
	}
	
	void reset()
	{
		pSeq = seq;
		seqCounter = 0;
		stepCounter = 0;	
	}
	
	float speed = 1.0;
	int stepCounter = 0;
	int seqCounter = 0;
	int* seq;
	int* pSeq;
	int* pSeqNext;
	int seqLength = 0;
	bool loop = true;
	
	float gain_dB;
	float gain_dB_step;
};

int toneVolumeSeq1_raw[8] = { 1, 1000, -10, 1000, 1, 1000, -40, 1000};
int toneVolumeSeq2_raw[4] = { -10, 1000, -50, 1000};

ToneVolumeSeq toneVolumeSeq1(toneVolumeSeq1_raw, 4);
ToneVolumeSeq toneVolumeSeq2(toneVolumeSeq2_raw, 2);

std::vector<ToneVolumeSeq*> masterToneVolSeq;

//------------------------------------------------
//----------------- Delay State ------------------
//------------------------------------------------

extern Delay staticDelay;
extern AudioMixer4 mixSources;
extern float delayVolume;
class DelayStateSeq
{
public:	
	DelayStateSeq(int* _seq, int _seqLength)
		:seq(_seq),
		pSeq(_seq),
		seqLength(_seqLength)
	{}
	~DelayStateSeq() {}

	void seqProceed()
	{	
		if (stepCounter == 0)
		{
			switch(*pSeq++)
			{
				// delay mute
				case 0: mixSources.gain(0, 0.00004);
						staticDelay.hold(false);
						staticDelay.setLoopLength(AUDIO_BLOCK_SAMPLES * 65); // eqal the delayBufferLength of 8192 + AUDIO_BLOCK_SAMPLES
						staticDelay.setTargetDelay(AUDIO_BLOCK_SAMPLES * 64);
						break;
				// normal delay mode
				case 1: mixSources.gain(0, delayVolume);
						staticDelay.hold(false);
						staticDelay.setLoopLength(AUDIO_BLOCK_SAMPLES * 65); // eqal the delayBufferLength of 8192 + AUDIO_BLOCK_SAMPLES
						staticDelay.setTargetDelay(AUDIO_BLOCK_SAMPLES * 64);
						break;
				// delay hold
				case 2: mixSources.gain(0, delayVolume);
						staticDelay.hold(true);
						staticDelay.setLoopLength(AUDIO_BLOCK_SAMPLES * 65); // eqal the delayBufferLength of 8192 + AUDIO_BLOCK_SAMPLES
						staticDelay.setTargetDelay(AUDIO_BLOCK_SAMPLES * 32);
						break;
				// talkThrough without delay
				case 3: mixSources.gain(0, delayVolume);
						staticDelay.hold(false);
						staticDelay.setLoopLength(AUDIO_BLOCK_SAMPLES * 65); // eqal the delayBufferLength of 8192 + AUDIO_BLOCK_SAMPLES
						staticDelay.setTargetDelay(0);
						break;
				// delay oneShot mode
				case 4:	break;
				default: break;			
			}			
			stepCounter = ((*pSeq++) / (MOD_RATE)) * (1.0/speed);
			seqCounter++;
		}
		else
		{
			stepCounter--;
		}
	}

	void reset()
	{
		pSeq = seq;
		seqCounter = 0;
		stepCounter = 0;	
	}

	float speed = 1.0;
	int stepCounter = 0;
	int seqCounter = 0;
	int* seq;
	int* pSeq;
	int* pSeqNext;
	int seqLength = 0;
	bool loop = true;
};

int delayStateSeq1_raw[4] = {1, 200, 2, 200};
int delayStateSeq2_raw[8] = {1, 50, 2, 300, 1, 1000, 2, 300};


DelayStateSeq delayStateSeq1(delayStateSeq1_raw, 2);
DelayStateSeq delayStateSeq2(delayStateSeq2_raw, 4);

std::vector<DelayStateSeq*> masterDelayStateSeq;


//------------------------------------------------
//----------------- Formant Filter ---------------
//------------------------------------------------

#define a1 0
#define e1 1
#define i1 2
#define o1 3
#define u1 4

class FormantFilterSequence
{
public:
	FormantFilterSequence(int* _seq, int _seqLength)
		:seq(_seq),
		pSeq(_seq),
		seqLength(_seqLength)
	{}
	~FormantFilterSequence() {}
	
	void setFormants(float* _fc, float* _gain, float* _BW) {
		// calcBiquad(FilterType,FrequencyC,dBgain,BW,QuantizationUnit,SampleRate,int*);
		calcBiquad(FILTER_BANDPASS,_fc[0],_gain[0],0,_BW[0],2147483648,44100,updateFilter);
		formantFilter1.updateCoefs(updateFilter);
		calcBiquad(FILTER_BANDPASS,_fc[1],_gain[1],0,_BW[1],2147483648,44100,updateFilter);
		formantFilter2.updateCoefs(updateFilter);
		calcBiquad(FILTER_BANDPASS,_fc[2],_gain[2],0,_BW[2],2147483648,44100,updateFilter);
		formantFilter3.updateCoefs(updateFilter);
		fc[0] = _fc[0];
		fc[1] = _fc[1];
		fc[2] = _fc[2];
		gain[0] = _gain[0];
		gain[1] = _gain[1];
		gain[2] = _gain[2];
		BW[0] = _BW[0];
		BW[1] = _BW[1];
		BW[2] = _BW[2];
	}
	
	void setInterpolation(float* _fc, float* _gain, float* _BW)
	{
		fc_delta[0] = (_fc[0] - fc[0]) / (float)stepCounter;
		fc_delta[1] = (_fc[1] - fc[1]) / (float)stepCounter;
		fc_delta[2] = (_fc[2] - fc[2]) / (float)stepCounter;
		gain_delta[0] = (_gain[0] - gain[0]) / (float)stepCounter;
		gain_delta[1] = (_gain[1] - gain[1]) / (float)stepCounter;
		gain_delta[2] = (_gain[2] - gain[2]) / (float)stepCounter;
		BW_delta[0] = (_BW[0] - BW[0]) / (float)stepCounter;
		BW_delta[1] = (_BW[1] - BW[1]) / (float)stepCounter;
		BW_delta[2] = (_BW[2] - BW[2]) / (float)stepCounter;
	}
	
	void seqProceed()
	{
		if (stepCounter == 0)
		{
			switch (*pSeq++)
			{
				case a1: 	setFormants(sopranA_fc, sopranA_gain, sopranA_BW);
							break;
				case e1: 	setFormants(sopranE_fc, sopranE_gain, sopranE_BW);
							break;
				case i1: 	setFormants(sopranI_fc, sopranI_gain, sopranI_BW);
							break;
				case o1: 	setFormants(sopranO_fc, sopranO_gain, sopranO_BW);
							break;
				case u1:	setFormants(sopranU_fc, sopranU_gain, sopranU_BW);
							break;
				default:	break;
			}
			formantInterpolation = *pSeq++;
			stepCounter = ((*pSeq++) / (MOD_RATE)) * (1.0/speed);
			
			if(formantInterpolation)
			{
				if(seqCounter+1 < seqLength)
					pSeqNext = pSeq;
				else
					pSeqNext = seq;
						
				switch (*pSeqNext)
				{
					case a1: 	setInterpolation(sopranA_fc, sopranA_gain, sopranA_BW);
								break;          
					case e1: 	setInterpolation(sopranE_fc, sopranE_gain, sopranE_BW);
								break;          
					case i1: 	setInterpolation(sopranI_fc, sopranI_gain, sopranI_BW);
								break;          
					case o1: 	setInterpolation(sopranO_fc, sopranO_gain, sopranO_BW);
								break;          
					case u1:	setInterpolation(sopranU_fc, sopranU_gain, sopranU_BW);
								break;
					default:	break;
				}
				formantIsInterpolating = true;
			}
			else
				formantIsInterpolating = false;
		}
		else
		{
			if(formantIsInterpolating)
			{
				fc[0] += fc_delta[0];
				fc[1] += fc_delta[1];
				fc[2] += fc_delta[2];
				gain[0] += gain_delta[0];
				gain[1] += gain_delta[1];
				gain[2] += gain_delta[2];
				BW[0] += BW_delta[0];
				BW[1] += BW_delta[1];
				BW[2] += BW_delta[2];
				setFormants(fc, gain, BW);
			}
			stepCounter--;
			if(stepCounter == 0)
				seqCounter++;
		}
	}
	
	void reset()
	{
		pSeq = seq;
		seqCounter = 0;
		stepCounter = 0;	
	}
	
	float speed = 1.0;
	int stepCounter = 0;
	int seqCounter = 0;
	int* seq;
	int* pSeq;
	int* pSeqNext;
	int seqLength = 0;
	bool loop = true;
	bool formantInterpolation = true;
	bool formantIsInterpolating = false;
	
	float fc[3] = {440, 440, 440};
	float gain[3] = {0, 0, 0};
	float BW[3] = {0.5, 0.5, 0.5};
	
	float fc_delta[3] = {0, 0, 0};
	float gain_delta[3] = {0, 0, 0};
	float BW_delta[3] = {0, 0, 0};
	
	float factor = 0.3;
	float sopranA_fc[3] = {800, 1150, 2900};
	float sopranA_gain[3] = {0, -6, -32};
	float sopranA_BW[3] = {0.5*factor, 0.5*factor, 0.3*factor};
	float sopranE_fc[3] = {350, 2000, 2800};
	float sopranE_gain[3] = {0, -20, -15};
	float sopranE_BW[3] = {0.5*factor, 0.4*factor, 0.3*factor};
	float sopranI_fc[3] = {270, 2140, 2950};
	float sopranI_gain[3] = {0, -12, -26};
	float sopranI_BW[3] = {0.5*factor, 0.4*factor, 0.3*factor};
	float sopranO_fc[3] = {450, 800, 2830};
	float sopranO_gain[3] = {0, -11, -22};
	float sopranO_BW[3] = {0.5*factor, 0.4*factor, 0.3*factor};
	float sopranU_fc[3] = {325, 700, 2700};
	float sopranU_gain[3] = {0, -16, -35};
	float sopranU_BW[3] = {0.5*factor, 0.4*factor, 0.3*factor};
	
};

int formantSeq1_raw[24] = { a1, 1, 1000, e1, 0, 1000, i1, 1, 1000, o1, 0, 1000, u1, 1, 1000, o1, 0, 1000, i1, 1, 1000, e1, 0, 1000};
int formantSeq2_raw[3] = { a1, 0, 1000};

FormantFilterSequence formantSeq1(formantSeq1_raw, 8);
FormantFilterSequence formantSeq2(formantSeq2_raw, 1);

std::vector<FormantFilterSequence*> masterFormantSeq;



//------------------------------------------------
//-------------------- Tone Osc ------------------
//------------------------------------------------

class ToneSequence
{
public:	
	ToneSequence(int* _seq, int _seqLength, AudioSynthWaveform* _osc, short _waveform)
		:seq(_seq),
		pSeq(_seq),
		seqLength(_seqLength),
		osc(_osc),
		waveform(_waveform)
	{}
	~ToneSequence() {}

	void seqProceed()
	{
		if (stepCounter == 0)
		{
			if(*pSeq != 0) // start note
				osc->begin(0.4,tune_frequencies2_PGM[*pSeq++], waveform);
			else	// stop note
			{
				osc->amplitude(0);
				pSeq++;
			}
			stepCounter = ((*pSeq++) / (MOD_RATE)) * (1.0/speed);
			seqCounter++;
		}
		else
		{
			stepCounter--;
		}
	}

	void reset()
	{
		pSeq = seq;
		seqCounter = 0;
		stepCounter = 0;	
	}

	float speed = 1.0;
	int stepCounter = 0;
	int seqCounter = 0;
	int* seq;
	int* pSeq;
	int* pSeqNext;
	int seqLength = 0;
	bool loop = true;

	short waveform;
	AudioSynthWaveform* osc;
};

int toneSeq1_1_raw[4] = {35, 1000, 0, 300};
int toneSeq1_2_raw[4] = {42, 1000, 0, 300};
int toneSeq1_3_raw[4] = {49, 1000, 0, 300};

int toneSeq2_1_raw[8] = {35, 50, 0, 300, 35, 1000, 0, 300};
int toneSeq2_2_raw[8] = {42, 50, 0, 300, 35, 1000, 0, 300};
int toneSeq2_3_raw[8] = {49, 50, 0, 300, 35, 1000, 0, 300};

ToneSequence toneSeq1_1(toneSeq1_1_raw, 2, &osc1, TONE_TYPE_SQUARE);
ToneSequence toneSeq1_2(toneSeq1_2_raw, 2, &osc2, TONE_TYPE_SQUARE);
ToneSequence toneSeq1_3(toneSeq1_3_raw, 2, &osc3, TONE_TYPE_SQUARE);

ToneSequence toneSeq2_1(toneSeq2_1_raw, 4, &osc1, TONE_TYPE_SQUARE);
ToneSequence toneSeq2_2(toneSeq2_2_raw, 4, &osc2, TONE_TYPE_SQUARE);
ToneSequence toneSeq2_3(toneSeq2_3_raw, 4, &osc3, TONE_TYPE_SQUARE);

std::vector<ToneSequence*> masterToneSeq;

#endif