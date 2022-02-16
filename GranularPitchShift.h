#pragma once
#include <math.h>
#include <stdlib.h>
#include "Utils.h"

using namespace Polygons;

namespace Z4
{
    class Grain
    {
    public:
        int start = 0;
        int length = 0;
        float speed = 1;
        float pos = 0;
        bool active = true;
    
        inline Grain()
        {
            this->start = 0;
            this->length = 0;
            this->speed = 1;
            this->pos = 0;
            this->active = true;
        }

        inline Grain(int start, int length, float speed, float pos)
        {
            this->start = start;
            this->length = length;
            this->speed = speed;
            this->pos = pos;
            this->active = true;
        }

        inline float Window(float pos, int length)
        {
            // Creates a smooth, cosine-like window, based off a triangle
            float frac = pos / length;
            if (frac < 0)
                return 0;
            
            float tri = 0;
            if (frac <= 0.5)
                tri = frac * 2;
            else
                tri = 2 - frac*2;
            
            float c1 = tri * tri; // The edges
            float c2 = 1-(1-tri)*(1-tri); // the smooth bell in the middle
            float output = (1-tri)*c1 + tri * c2; // morph between them
            return output;
        }

        inline static float GetData(float* data, int dataSize, float pos)
        {
            int a = (int)floorf(pos);
            int b = (int)ceilf(pos);
            float frac = pos - a;
            return data[a%dataSize] * (1-frac) + data[b%dataSize] * frac;
        }

        inline int Process(int bufsize, float* data, int dataSize, float* output)
        {
            int i = 0;
            while (i < bufsize && active)
            {
                float window_val = Window(pos-start, length);
                output[i] = output[i] + GetData(data, dataSize, pos) * window_val;
                pos += speed;
                if (pos >= (start + length))
                    active = false;
                i += 1;
            }
            return i;
        }
    };

    template<int N>
    class GranularPitchShift
    {
        const static int GrainSize = 4000;
        const static int GrainCount = 6;
        float Buffer[N];
        Grain Grains[GrainCount];
        int Samplerate;
        int K;

    public:
        inline GranularPitchShift(int samplerate, float pitchShift)
        {
            ZeroBuffer(Buffer, N);

            for (int i = 0; i < GrainCount; i++)
            {
                Grains[i].start = GrainSize/GrainCount*i;
                Grains[i].length = GrainSize;
                Grains[i].speed = pitchShift;
                Grains[i].pos = 0;
            }

            K = 0;
            this->Samplerate = samplerate;
        }

        inline void Process(float* input, float* output, int bufSize)
        {
            for (int i = 0; i < bufSize; i++)
                Buffer[(K + i + GrainSize) % N] = input[i]; // writing into the buffer ahead of the read head, which tracks K, our current position
            
            ZeroBuffer(output, bufSize);

            for (int i=0; i<GrainCount; i++)
            {
                Grain* g = &Grains[i];
                int samples_processed = g->Process(bufSize, Buffer, N, &output[0]);
                if (!g->active)
                {
                    g->start = K + samples_processed + (rand()/(float)RAND_MAX) * 300;
                    g->pos = g->start;
                    g->length = (rand()/(float)RAND_MAX + 1) * GrainSize;
                    g->active = true;
                    if (samples_processed < bufSize)
                        g->Process(bufSize - samples_processed, Buffer, N, &output[samples_processed]);
                }
            }

            Gain(output, 1.0 / sqrtf(GrainCount/2.0), bufSize);
            K += bufSize;
        }
    };
}
