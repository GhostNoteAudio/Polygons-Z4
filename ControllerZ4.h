
#pragma once

#include "Constants.h"
#include "ParameterZ4.h"
#include "Utils.h"
#include "Z4Rev.h"

namespace Z4
{
	enum class InputMode
	{
		Stereo = 0,
		Left = 1,
		Right = 2,
	};

	class Controller
	{
	private:
		Z4Rev Reverb;
		int samplerate;
		InputMode inputMode;
		float inGain;
		float outGain;

		uint16_t parameters[Parameter::COUNT];

	public:
		Controller(int samplerate) : Reverb(samplerate) 
		{
			this->samplerate = samplerate;
			inputMode = InputMode::Left;
			inGain = 1.0;
			outGain = 1.0;
		}

		int GetSamplerate()
		{
			return samplerate;
		}

		uint16_t* GetAllParameters()
		{
			return parameters;
			
		}

		double GetScaledParameter(int param)
		{
			switch (param)
			{
				case Parameter::Decay:				return Polygons::Response2Dec(P(param)) * 30;
				case Parameter::SizeEarly:			return 0.1 + P(param) * 0.9;
				case Parameter::SizeLate:			return 0.1 + P(param) * 0.9;
				case Parameter::Diffuse:			return P(param);

				case Parameter::LowCutPre:			return 200 + Polygons::Response4Oct(P(param)) * 15800;
				case Parameter::HighCutPre:			return 20 + Polygons::Response4Oct(P(param)) * 1980;
				case Parameter::Modulate:			return P(param);
				case Parameter::Mix:				return P(param);


				case Parameter::EarlyStages:		return (int)(1 + P(param) * 11.99);
				case Parameter::Interpolation:		return (int)(P(param, 8));
				case Parameter::Shimmer:			return (int)(P(param, 8) >= 0.5);
				case Parameter::InputMode:			return (int)(P(param, 8) * 2.999);

				case Parameter::LowCutPost:			return 200 + Polygons::Response4Oct(P(param)) * 15800;
				case Parameter::HighCutPost:		return 20 + Polygons::Response4Oct(P(param)) * 1980;
				case Parameter::InGain:				return -20 + P(param) * 40;
				case Parameter::OutGain:			return -20 + P(param) * 40;
			}
			return parameters[param];
		}

		void SetParameter(int param, uint16_t value)
		{
			parameters[param] = value;
			auto scaled = GetScaledParameter(param);
			Reverb.SetParameter(param, scaled);

			if (param == Parameter::InputMode)
			{
				inputMode = (InputMode)(int)scaled;
				Serial.print("Input mode: ");
				Serial.println((int)inputMode);
			}
			else if (param == Parameter::InGain)
				inGain = DB2gain(scaled);
			else if (param == Parameter::OutGain)
				outGain = DB2gain(scaled);
		}

		void Process(float** inputs, float** outputs, int bufferSize)
		{
			
			Gain(inputs[0], inGain, bufferSize);
			Gain(inputs[1], inGain, bufferSize);
			
			if (inputMode == InputMode::Left)
				Copy(inputs[1], inputs[0], bufferSize);
			else if (inputMode == InputMode::Right)
				Copy(inputs[0], inputs[1], bufferSize);

			Reverb.Process(inputs, outputs, bufferSize);
			
			Gain(outputs[0], outGain, bufferSize);
			Gain(outputs[1], outGain, bufferSize);
		}
		
	private:
		double P(int para, int maxVal=1023)
		{
			auto idx = (int)para;
			return idx >= 0 && idx < Parameter::COUNT ? (parameters[idx] / (double)maxVal) : 0.0;
		}
	};
}
