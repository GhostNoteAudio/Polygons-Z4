#pragma once

#include "Polygons.h"
#include "blocks/ModulatedDelay.h"
#include "blocks/ModulatedAllpass.h"
#include "Constants.h"
#include "blocks/Biquad.h"
#include "blocks/Bitcrusher.h"

using namespace Polygons;

namespace Z4
{
    class Z4Rev
    {
        const static int PRE_DIFFUSE_COUNT = 12;
        const static int ZCOUNT = 4;

        ModulatedDelay<FS_MAX/16, BUFFER_SIZE> Delay[ZCOUNT];
        ModulatedAllpass<FS_MAX/20, BUFFER_SIZE> PreDiffuser[PRE_DIFFUSE_COUNT];
        ModulatedAllpass<FS_MAX/20, BUFFER_SIZE> Diffuser[ZCOUNT*2];
        Biquad lpPre, lpPost, hpPre, hpPost;

        // Delay lengths in milliseconds, handpicked arbitrarily :)
        float PreDiffuserSizes[PRE_DIFFUSE_COUNT] = {56.797, 59.12, 65.1785, 67.324, 69.7954, 72.55, 75.6531, 80.804, 83.157, 86.45, 90.234, 96.194};
        float DiffuserSizes[ZCOUNT] = {70.312, 78.5123, 87.9312, 92.1576};
        float DelaySizes[ZCOUNT] = {73.459, 95.961, 104.1248, 117.934};

        // Modulation rates in Hz, I used sequential prime numbers scaled down
        float PreDiffuserModRate[PRE_DIFFUSE_COUNT] = {13*0.05, 17*0.05, 19*0.05, 23*0.05, 29*0.05};
        float DiffuserModRate[ZCOUNT] = {31*0.02, 37*0.02, 41*0.02, 43*0.02};
        float DelayModRate[ZCOUNT]    = {47*0.01, 53*0.01, 59*0.01, 61*0.01};

        int Samplerate;
        float Krt;
        float T60;
        float Wet;
        float Dry;
        float EarlySize;
        float DiffuserSize;
        float LateSize;
        float Modulation;
        float DiffuseFeedback;
        bool Interpolation;
        int EarlyStages;
        bool freeze;
        float smoothedFreeze;

    public:
        Z4Rev(int samplerate) : lpPre(Biquad::FilterType::LowPass, samplerate), lpPost(Biquad::FilterType::LowPass6db, samplerate),
                                hpPre(Biquad::FilterType::HighPass, samplerate), hpPost(Biquad::FilterType::HighPass6db, samplerate)
        {
            Samplerate = samplerate;
            Krt = 0.0;
            T60 = 5.0;
            Wet = 0.5;
            Dry = 1.0;
            EarlySize = 0.1;
            DiffuserSize = 1.0;
            LateSize = 0.1;
            Modulation = 0.2;
            DiffuseFeedback = 0.7;
            Interpolation = true;
            EarlyStages = 4;
            lpPre.Frequency = 20000;
            lpPost.Frequency = 16000;
            hpPre.Frequency = 20;
            hpPost.Frequency = 20;
            smoothedFreeze = 0;
            freeze = false;

            UpdateAll();
        }

        void SetParameter(int paramId, double value)
        {
            if (paramId == Parameter::Decay)
            {
                if (value < 0.1)
                    value = 0.1;
                T60 = value;
            }
            else if (paramId == Parameter::Diffuse)
            {
                DiffuseFeedback = 0.5 + (1 - value) * 0.49;
            }
            else if (paramId == Parameter::Interpolation)
            {
                Interpolation = (int)value;
            }
            else if (paramId == Parameter::Mix)
            {
                Wet = ClipF(value * 2, 0.0, 1.0);
                Dry = ClipF(2 - value * 2, 0, 1);
            }
            else if (paramId == Parameter::SizeEarly)
            {
                EarlySize = value; // 10-100%
            }
            else if (paramId == Parameter::SizeLate)
            {
                LateSize = value; // 10-100%
                DiffuserSize = 0.2 + value * 0.8;
            }
            else if (paramId == Parameter::Modulate)
            {
                Modulation = value;
            }
            else if (paramId == Parameter::EarlyStages)
            {
                EarlyStages = (int)value;
            }
            else if (paramId == Parameter::LowCutPre)
            {
                lpPre.Frequency = value;
            }
            else if (paramId == Parameter::LowCutPost)
            {
                lpPost.Frequency = value;
            }
            else if (paramId == Parameter::HighCutPre)
            {
                hpPre.Frequency = value;
            }
            else if (paramId == Parameter::HighCutPost)
            {
                hpPost.Frequency = value;
            }
            else if (paramId == Parameter::Freeze)
            {
				freeze = value > 0.5;
			}

            UpdateAll();
        }

        void UpdateAll()
        {
            lpPre.Update();
            lpPost.Update();
            hpPre.Update();
            hpPost.Update();

            const float IdealisedTimeConstant = 0.15 * std::sqrt(LateSize); // assumed tank round trip time
            auto tcToT60 = T60 / IdealisedTimeConstant;
            auto dbPerTc = -60 / tcToT60;
            Krt = std::pow(10, dbPerTc/20);

            for (size_t i = 0; i < PRE_DIFFUSE_COUNT; i++)
            {
                PreDiffuser[i].Feedback = 0.73;
                PreDiffuser[i].InterpolationEnabled = Interpolation;
                PreDiffuser[i].SampleDelay = (int)(PreDiffuserSizes[i] * 0.001 * EarlySize * Samplerate);
                PreDiffuser[i].ModRate = PreDiffuserModRate[i] / Samplerate;
                PreDiffuser[i].ModAmount = Modulation * 25;
            }

            for (size_t i = 0; i < ZCOUNT; i++)
            {
                Diffuser[i].Feedback = DiffuseFeedback;
                Diffuser[i].InterpolationEnabled = Interpolation;
                Diffuser[i].SampleDelay = (int)(DiffuserSizes[i] * 0.001 * DiffuserSize * Samplerate);
                Diffuser[i].ModRate = DiffuserModRate[i] / Samplerate;
                Diffuser[i].ModAmount = Modulation * 25;

                Delay[i].SampleDelay = (int)(DelaySizes[i] * 0.001 * LateSize * Samplerate);
                Delay[i].ModRate = DelayModRate[i] / Samplerate;
                Delay[i].ModAmount = Modulation * (i == 0 ? 200 : 25); // extra mod on the first delay
            }
        }

        void Process(float** inputs, float** outputs, int bufSize)
        {
            // Jumping straight between 100% feedback (freeze) and the selected feedback causes a click, needs to be smoothed
			smoothedFreeze = smoothedFreeze * 0.95 + (int)freeze * 0.05;
            float activeKrt = smoothedFreeze + (1-smoothedFreeze) * Krt;

            auto tb = Buffers::Request();
            auto buf = tb.Ptr;

            Copy(buf, inputs[0], bufSize);
            Mix(buf, inputs[1], 1.0, bufSize);
            lpPre.Process(buf, buf, bufSize);
            hpPre.Process(buf, buf, bufSize);

            float* preDiffIO = buf;
            for (size_t i = 0; i < PRE_DIFFUSE_COUNT; i++)
            {
                PreDiffuser[i].Process(preDiffIO, bufSize);
                preDiffIO = PreDiffuser[i].GetOutput();
            }

            preDiffIO = PreDiffuser[EarlyStages-1].GetOutput();

            // this compensates for fact that we take 4 output taps at full volume
            // It also reduces the max value pushed into the delay line, since that it stored as an integer and can clip
            Gain(preDiffIO, 0.5, bufSize); 

            for (size_t i = 0; i < ZCOUNT; i++)
            {
                Copy(buf, preDiffIO, bufSize);
                Mix(buf, Delay[(i - 1 + ZCOUNT) % ZCOUNT].GetOutput(), activeKrt, bufSize);
                if (i == 0)
                {
                    lpPost.Process(buf, buf, bufSize);
                    hpPost.Process(buf, buf, bufSize);
                }
                Diffuser[2*i].Process(buf, bufSize);
                Diffuser[2*i+1].Process(Diffuser[2*i].GetOutput(), bufSize);
                Delay[i].Process(Diffuser[2*i+1].GetOutput(), bufSize);
            }
            
            ZeroBuffer(outputs[0], bufSize);
            Mix(outputs[0], Delay[0].GetOutput(), Wet, bufSize);
            Mix(outputs[0], Delay[2].GetOutput(), Wet, bufSize);
            Mix(outputs[0], inputs[0], Dry, bufSize);

            ZeroBuffer(outputs[1], bufSize);
            Mix(outputs[1], Delay[1].GetOutput(), Wet, bufSize);
            Mix(outputs[1], Delay[3].GetOutput(), Wet, bufSize);
            Mix(outputs[1], inputs[1], Dry, bufSize);
        }
        
    };
}
