
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
		bool active;

		uint16_t parameters[Parameter::COUNT];

	public:
		Controller(int samplerate) : Reverb(samplerate) 
		{
			this->samplerate = samplerate;
			inputMode = InputMode::Left;
			inGain = 1.0;
			outGain = 1.0;
			active = true;
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
				case Parameter::Shimmer:			return (int)(P(param, 64) * 5.999);
				case Parameter::InputMode:			return (int)(P(param, 8) * 2.999);

				case Parameter::LowCutPost:			return 200 + Polygons::Response4Oct(P(param)) * 15800;
				case Parameter::HighCutPost:		return 20 + Polygons::Response4Oct(P(param)) * 1980;
				case Parameter::InGain:				return (int)(P(param) * 40) / 2.0; // 0.5db increments
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
			if (param == Parameter::Active)
			{
				active = value == 0 ? false : true;
			}
			else if (param == Parameter::InGain)
				inGain = DB2gain(scaled);
			else if (param == Parameter::OutGain)
				outGain = DB2gain(scaled);
		}

		void Process(float** inputs, float** outputs, int bufferSize)
		{
			// inGain applied to ADC programmable amplifier
			//Gain(inputs[0], inGain, bufferSize);
			//Gain(inputs[1], inGain, bufferSize);
			
			auto a = Buffers::Request();
			auto b = Buffers::Request();
			float* tempInputs[2] = {a.Ptr, b.Ptr};

			if (inputMode == InputMode::Left)
			{
				Copy(tempInputs[0], inputs[0], bufferSize);
				Copy(tempInputs[1], inputs[0], bufferSize);
			}
			else if (inputMode == InputMode::Right)
			{
				Copy(tempInputs[0], inputs[1], bufferSize);
				Copy(tempInputs[1], inputs[1], bufferSize);
			}
			else
			{
				Copy(tempInputs[0], inputs[0], bufferSize);
				Copy(tempInputs[1], inputs[1], bufferSize);
			}

			Reverb.Process(tempInputs, outputs, bufferSize);
			
			Gain(outputs[0], outGain, bufferSize);
			Gain(outputs[1], outGain, bufferSize);

			// We still need to do all the processing above even if effect is not active
			// otherwise we get frozen buffers and stale data when we turn it back on
			if (!active)
			{
				Copy(outputs[0], inputs[0], bufferSize);
				Copy(outputs[1], inputs[1], bufferSize);
			}
		}
		
	private:
		double P(int para, int maxVal=1023)
		{
			auto idx = (int)para;
			return idx >= 0 && idx < Parameter::COUNT ? (parameters[idx] / (double)maxVal) : 0.0;
		}
	};
}
