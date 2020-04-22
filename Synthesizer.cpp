#include <iostream>
using namespace std;

#include "olcNoiseMaker.h"
struct sEnvelopeADSR
{
	double dAttackTime;
	double dDecayTime;
	double dReleaseTime;
	double dTriggerOnTime;
	double dTriggerOffTime;
	double dSustainAmplitude;
	double dStartAmplitude;
	bool bNoteOn = false;
	sEnvelopeADSR()
	{
		dAttackTime = 0.1;
		dDecayTime = 0.01;
		dReleaseTime = 0.2;
		dTriggerOnTime = 0.0;
		dTriggerOffTime = 0.0;
		dSustainAmplitude = 0.8;
		dStartAmplitude = 1.0;
	}
	double GetAmplitude(double dTime)
	{
		double dAmplitude = 0.0;
		double dLifeTime = dTime - dTriggerOnTime;
		if (bNoteOn)
		{
			if (dLifeTime <= dAttackTime)
				dAmplitude = (dLifeTime / dAttackTime) * dStartAmplitude;
			else if ((dLifeTime > dAttackTime) && (dLifeTime <= dAttackTime + dDecayTime))
				dAmplitude = ((dLifeTime - dAttackTime) / dDecayTime) * (dSustainAmplitude - dStartAmplitude) + dStartAmplitude;
			else if (dLifeTime > dAttackTime + dDecayTime)
				dAmplitude = dSustainAmplitude;
		}
		else
		{
			dAmplitude = (dTime - dTriggerOffTime) / dReleaseTime * (0.0 - dSustainAmplitude) + dSustainAmplitude;
		}
		if (dAmplitude < 0.0001)
			dAmplitude = 0.0;
		return dAmplitude;
	}
	void NoteOn(double dTimeOn)
	{
		dTriggerOnTime = dTimeOn;
		bNoteOn = true;
	}
	void NoteOff(double dTimeOff)
	{
		dTriggerOffTime = dTimeOff;
		bNoteOn = false;
	}
};
sEnvelopeADSR envelop;
// Global synthesizer variables
atomic<double> dFrequencyOutput = 0.0;			// dominant output frequency of instrument, i.e. the note
double dOctaveBaseFrequency = 110.0; // A2		// frequency of octave represented by keyboard
double d12thRootOf2 = pow(2.0, 1.0 / 12.0);		// assuming western 12 notes per ocatve
double w(double dHertz)
{
	return dHertz * 2.0 * PI;
}
double osc(double dHertz, double dTime, int nType)
{
	switch (nType)
	{
	case 0: // sin wave
		return sin(w(dHertz)*dTime);
	case 1: //Square wave
		return sin(w(dHertz)*dTime) > 0.0 ? 1.0 : -1.0;
	case 2: // triangle wave
		return asin(sin(w(dHertz)*dTime)) * 2.0 / PI;
	case 3: // Saw analog
	{
		double dOutput = 0.0;
		for (double n = 1.0; n < 100; n++)
		{
			dOutput += (sin(n*w(dHertz)*dTime)) / n;
		}
		return dOutput * 2.0/PI;
	}
	case 4: // Saw mathematical
		return (2.0 / PI) * (dHertz * PI * fmod(dTime, 1.0 / dHertz) - (PI / 2.0));
	case 5:
		return 2.0 * ((double)rand() / (double)RAND_MAX) - 1.0;
	default:
		return 0.0;
	}
}
// Function used by olcNoiseMaker to generate sound waves
// Returns amplitude (-1.0 to +1.0) as a function of time
double MakeNoise(double dTime)
{
	double dOutput = envelop.GetAmplitude(dTime) * 
		(
			+osc(dFrequencyOutput*0.5, dTime, 4)
			+osc(dFrequencyOutput*1.5, dTime, 3)
		);
	return dOutput * 0.5; // Master Volume
}



int main()
{
	// Shameless self-promotion
	wcout << "www.OneLoneCoder.com - Synthesizer Part 1" << endl << "Single Sine Wave Oscillator, No Polyphony" << endl << endl;

	// Get all sound hardware
	vector<wstring> devices = olcNoiseMaker<short>::Enumerate();

	// Display findings
	for (auto d : devices) wcout << "Found Output Device: " << d << endl;
	wcout << "Using Device: " << devices[0] << endl;

	// Display a keyboard
	wcout << endl <<
		"|   |   |   |   |   | |   |   |   |   | |   | |   |   |   |" << endl <<
		"|   | S |   |   | F | | G |   |   | J | | K | | L |   |   |" << endl <<
		"|   |___|   |   |___| |___|   |   |___| |___| |___|   |   |__" << endl <<
		"|     |     |     |     |     |     |     |     |     |     |" << endl <<
		"|  Z  |  X  |  C  |  V  |  B  |  N  |  M  |  ,  |  .  |  /  |" << endl <<
		"|_____|_____|_____|_____|_____|_____|_____|_____|_____|_____|" << endl << endl;

	// Create sound machine!!
	olcNoiseMaker<short> sound(devices[0], 44100, 1, 8, 512);

	// Link noise function with sound machine
	sound.SetUserFunction(MakeNoise);

	// Sit in loop, capturing keyboard state changes and modify
	// synthesizer output accordingly
	int nCurrentKey = -1;
	bool bKeyPressed = false;
	while (1)
	{
		bKeyPressed = false;
		for (int k = 0; k < 16; k++)
		{
			if (GetAsyncKeyState((unsigned char)("ZSXCFVGBNJMK\xbcL\xbe\xbf"[k])) & 0x8000)
			{
				if (nCurrentKey != k)
				{
					envelop.NoteOn(sound.GetTime());
					dFrequencyOutput = dOctaveBaseFrequency * pow(d12thRootOf2, k);
					wcout << "\rNote On : " << sound.GetTime() << "s " << dFrequencyOutput << "Hz";
					nCurrentKey = k;
				}

				bKeyPressed = true;
			}
		}

		if (!bKeyPressed)
		{
			if (nCurrentKey != -1)
			{
				envelop.NoteOff(sound.GetTime());
				wcout << "\rNote Off: " << sound.GetTime() << "s                        ";
				nCurrentKey = -1;
			}
		}
	}

	return 0;
}
