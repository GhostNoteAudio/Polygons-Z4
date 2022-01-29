#pragma once

#include "Polygons.h"
#include "ParameterZ4.h"
#include "ControllerZ4.h"
#include "Utils.h"

namespace Z4
{
    const int PRESET_COUNT = 7;

    float BufferInL[BUFFER_SIZE];
    float BufferInR[BUFFER_SIZE];
    float BufferOutL[BUFFER_SIZE];
    float BufferOutR[BUFFER_SIZE];

    Controller controller(SAMPLERATE);
    PolyOS os;

    uint16_t Presets[Parameter::COUNT * PRESET_COUNT];\
    uint8_t currentPreset;

    void loadPreset(int number);
    void storePreset(int number);

    const char* ParameterNames[Parameter::COUNT];
    
    void setNames()
    {
        ParameterNames[Parameter::Decay] = "Decay";
        ParameterNames[Parameter::SizeEarly] = "Early Size";
        ParameterNames[Parameter::SizeLate] = "Late Size";
        ParameterNames[Parameter::Diffuse] = "Diffusion";
        ParameterNames[Parameter::LowCutPre] = "Low-pass";
        ParameterNames[Parameter::HighCutPre] = "Hi-pass";
        ParameterNames[Parameter::Modulate] = "Modulation";
        ParameterNames[Parameter::Mix] = "Mix";
        ParameterNames[Parameter::EarlyStages] = "Bloom";
        ParameterNames[Parameter::Interpolation] = "Quality";
        ParameterNames[Parameter::Shimmer] = "Shimmer";
        ParameterNames[Parameter::InputMode] = "Input Mode";
        ParameterNames[Parameter::LowCutPost] = "Lows";
        ParameterNames[Parameter::HighCutPost] = "Highs";
        ParameterNames[Parameter::InGain] = "In Gain";
        ParameterNames[Parameter::OutGain] = "Out Gain";
    }

    void RegisterParams()
    {
        os.Register(Parameter::Decay,           1023, Polygons::ControlMode::Encoded, 0, 4);
        os.Register(Parameter::SizeEarly,       1023, Polygons::ControlMode::Encoded, 1, 4);
        os.Register(Parameter::SizeLate,        1023, Polygons::ControlMode::Encoded, 2, 4);
        os.Register(Parameter::Diffuse,         1023, Polygons::ControlMode::Encoded, 3, 4);

        os.Register(Parameter::LowCutPre,       1023, Polygons::ControlMode::Encoded, 4, 4);
        os.Register(Parameter::HighCutPre,      1023, Polygons::ControlMode::Encoded, 5, 4);
        os.Register(Parameter::Modulate,        1023, Polygons::ControlMode::Encoded, 6, 4);
        os.Register(Parameter::Mix,             1023, Polygons::ControlMode::Encoded, 7, 4);


        os.Register(Parameter::EarlyStages,     1023, Polygons::ControlMode::Encoded, 8, 4);
        os.Register(Parameter::Interpolation,   8, Polygons::ControlMode::Encoded, 9, 1);
        os.Register(Parameter::Shimmer,         8, Polygons::ControlMode::Encoded, 10, 1);
        os.Register(Parameter::InputMode,       8, Polygons::ControlMode::Encoded, 11, 1);
        
        os.Register(Parameter::LowCutPost,      1023, Polygons::ControlMode::Encoded, 12, 4);
        os.Register(Parameter::HighCutPost,     1023, Polygons::ControlMode::Encoded, 13, 4);
        os.Register(Parameter::InGain,          1023, Polygons::ControlMode::Encoded, 14, 4);
        os.Register(Parameter::OutGain,         1023, Polygons::ControlMode::Encoded, 15, 4);

        os.Register(Parameter::Active,          1, Polygons::ControlMode::DigitalToggle, 9, 0);
        os.Register(Parameter::Freeze,          1, Polygons::ControlMode::Digital, 10, 0);
    }

    inline void getPageName(int page, char* dest)
    {
        if (page == 0)
            strcpy(dest, "Primary");
        else if (page == 1)
            strcpy(dest, "Secondary");
        else if (page == 7)
            strcpy(dest, "Store");
        else
            strcpy(dest, "");
    }

    inline void getParameterName(int paramId, char* dest)
    {
        if (paramId >= 0)
            strcpy(dest, ParameterNames[paramId]);
        else
            strcpy(dest, "");
    }

    inline void getParameterDisplay(int paramId, char* dest)
    {
        double val = controller.GetScaledParameter(paramId);
       
        if (paramId == Parameter::Decay)
            sprintf(dest, "%.2fs", val);
        else if (paramId == Parameter::SizeEarly || paramId == Parameter::SizeLate || paramId == Parameter::Diffuse || paramId == Parameter::Modulate || paramId == Parameter::Mix)
            sprintf(dest, "%.0f%%", val*100);
        else if (paramId == Parameter::LowCutPre || paramId == Parameter::HighCutPre || paramId == Parameter::LowCutPost || paramId == Parameter::HighCutPost)
        {
            if (val < 1000)
                sprintf(dest, "%.0fHz", val);
            else
                sprintf(dest, "%.1fKHz", val/1000);
        }
        else if (paramId == Parameter::EarlyStages)
            sprintf(dest, "%d", (int)val);
        else if (paramId == Parameter::Interpolation)
        {
            if (val > 0.5)
                strcpy(dest, "High");
            else
                strcpy(dest, "Low");
        }
        else if (paramId == Parameter::Shimmer)
        {
            if (val < 0.5)
                strcpy(dest, "Off");
            else
                strcpy(dest, "On");
        }
        else if (paramId == Parameter::InputMode)
        {
            if ((int)val == 0)
                strcpy(dest, "Stereo");
            else if ((int)val == 1)
                strcpy(dest, "Left");
            else
                strcpy(dest, "Right");
        }
        else if (paramId == Parameter::InGain || paramId == Parameter::OutGain)
        {
            sprintf(dest, "%.1fdB", val);
        }
        else
        {
            sprintf(dest, "%.2f", val);
        }
    }

    inline void setPresetLed()
    {
        // changes the different RGB colours of the LED to indicate preset number
        Polygons::pushDigital(0, (currentPreset + 1) & 0b001);
        Polygons::pushDigital(1, (currentPreset + 1) & 0b010);
        Polygons::pushDigital(2, (currentPreset + 1) & 0b100);
    }

    inline void setActiveFreezeLeds()
    {
        bool active = controller.GetScaledParameter(Parameter::Active) == 1;
        bool freeze = controller.GetScaledParameter(Parameter::Freeze) == 1;
        // Uses the Red LED to indicate active and freeze states
        Polygons::pushDigital(3, active);
        Polygons::pushDigital(6, freeze);
    }


    inline void setIOConfig()
    {
        int gainIn = (int8_t)controller.GetScaledParameter(Parameter::InGain) * 2;
        int gainOut = (int8_t)controller.GetScaledParameter(Parameter::OutGain);
        Polygons::codec.analogInGain(gainIn, gainIn);
        Polygons::codec.lineOutGain(gainOut, gainOut, false);
        Polygons::codec.headphoneGain(gainOut, gainOut, false);
    }

    inline bool handleUpdate(Polygons::ParameterUpdate* update)
    {
        if (update->Type == Polygons::MessageType::Digital && update->Index == 7)
        {
            storePreset(currentPreset);
            return true;
        }
        else if (update->Type == Polygons::MessageType::Digital && update->Index == 8 && update->Value > 0)
        {
            loadPreset((currentPreset + 1) % PRESET_COUNT);
            return true;
        }

       return false;
    }

    inline void setParameter(uint8_t paramId, uint16_t value)
    {
        controller.SetParameter(paramId, (uint16_t)value);
        setActiveFreezeLeds();
        if (paramId == Parameter::InGain || paramId == Parameter::OutGain)
            setIOConfig();
    }
    
    void audioCallback(int32_t** inputs, int32_t** outputs)
    {
        float scaler = (float)(1.0 / (double)SAMPLE_32_MAX);
        for (size_t i = 0; i < AUDIO_BLOCK_SAMPLES; i++)
        {
            BufferInL[i] = inputs[0][i] * scaler;
            BufferInR[i] = inputs[1][i] * scaler;
        }

        float* ins[2] = {BufferInL, BufferInR};
        float* outs[2] = {BufferOutL, BufferOutR};
        controller.Process(ins, outs, AUDIO_BLOCK_SAMPLES);

        for (size_t i = 0; i < AUDIO_BLOCK_SAMPLES; i++)
        {
            outputs[0][i] = (int)(BufferOutL[i] * SAMPLE_32_MAX);
            outputs[1][i] = (int)(BufferOutR[i] * SAMPLE_32_MAX);
        }
    }

    void loadPreset(int number)
    {
        currentPreset = number;
        auto preset = &Presets[number * Parameter::COUNT];
        for (size_t i = 0; i < Parameter::COUNT; i++)
        {
            if (i != Parameter::Active && i != Parameter::Freeze)
            {
                controller.SetParameter(i, preset[i]);
                os.Parameters[i].Value = preset[i];
            }
        }
        setPresetLed();
        setIOConfig();
    }

    void storePreset(int number)
    {
        auto preset = &Presets[number * Parameter::COUNT];
        auto rawParams = controller.GetAllParameters();
        for (size_t i = 0; i < Parameter::COUNT; i++)
        {
            preset[i] = rawParams[i];
        }
        bool ok = Storage::WriteFile("Z4/presets.bin", (uint8_t*)Presets, sizeof(Presets));
        if (ok)
            os.menu.setMessage("Preset Stored", 1000);
        else
            os.menu.setMessage("Error writing preset!", 1000);
    }

    inline void LoadPresetsSD()
    {
        if (Storage::FileExists("Z4/presets.bin"))
        {
            Serial.println("Reading presets from SD Card...");
            Storage::ReadFile("Z4/presets.bin", (uint8_t*)Presets, sizeof(Presets));
            Serial.println("Done reading presets");
        }
        else
        {
            Serial.println("No presets are present, writing to SD Card...");
            Storage::CreateFolder("Z4");
            Storage::WriteFile("Z4/presets.bin", (uint8_t*)Presets, sizeof(Presets));
            Serial.println("Done writing presets");
        }
    }

    inline void start()
    {
        Serial.println("Starting up - waiting for controller signal...");
        //os.waitForControllerSignal();
        setNames();
        RegisterParams();

       for (size_t i = 0; i < Parameter::COUNT * PRESET_COUNT; i++)
           Presets[i] = 0;

        os.HandleUpdateCallback = handleUpdate;
        os.SetParameterCallback = setParameter;
        os.PageCount = 8;
        os.menu.getPageName = getPageName;
        os.menu.getParameterName = getParameterName;
        os.menu.getParameterDisplay = getParameterDisplay;
        
        LoadPresetsSD();
        loadPreset(0);
        controller.SetParameter(Parameter::Active, 1);
        setActiveFreezeLeds();
        i2sAudioCallback = audioCallback;
    }

    inline void loop()
    {
        os.loop();
    }
}
