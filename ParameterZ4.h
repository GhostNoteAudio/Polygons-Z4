#pragma once

namespace Z4
{
    class Parameter
    {
    public:
        static const int Decay = 0;
        static const int SizeEarly = 1;
        static const int SizeLate = 2;
        static const int Diffuse = 3;

        static const int LowCutPre = 4;
        static const int HighCutPre = 5;
        static const int Modulate = 6;
        static const int Mix = 7;

        static const int EarlyStages = 8;
        static const int Interpolation = 9;
        static const int Shimmer = 10;
        static const int InputMode = 11;

        static const int LowCutPost = 12;
        static const int HighCutPost = 13;
        static const int InGain = 14;
        static const int OutGain = 15;

        static const int Active = 16;
        static const int Freeze = 17;

        static const int COUNT = 18;
    };
}
