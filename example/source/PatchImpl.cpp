#include "Patch.h"

#include <algorithm>
#include <cmath>
#include <span>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

class DelayLine
{
  public:
    void init(float* buffer, size_t size)
    {
        m_buffer = buffer;
        m_size = size;
        m_writePos = 0;
        if (m_buffer)
        {
            for (size_t i = 0; i < m_size; ++i)
            {
                m_buffer[i] = 0.0f;
            }
        }
    }

    void write(float sample)
    {
        if (!m_buffer) return;
        m_buffer[m_writePos] = sample;
        m_writePos = (m_writePos + 1) % m_size;
    }

    float read(float delaySamples) const
    {
        if (!m_buffer) return 0.0f;
        float readPos = static_cast<float>(m_writePos) - delaySamples;
        while (readPos < 0.0f)
        {
            readPos += static_cast<float>(m_size);
        }

        size_t pos1 = static_cast<size_t>(readPos) % m_size;
        size_t pos2 = (pos1 + 1) % m_size;
        float frac = readPos - std::floor(readPos);

        return m_buffer[pos1] * (1.0f - frac) + m_buffer[pos2] * frac;
    }

  private:
    float* m_buffer = nullptr;
    size_t m_size = 0;
    size_t m_writePos = 0;
};

class OnePoleLP
{
  public:
    void setCutoff(float cutoffHz, float sampleRate)
    {
        m_alpha = 1.0f - std::exp(-2.0f * M_PI * cutoffHz / sampleRate);
    }

    float process(float input)
    {
        m_state += m_alpha * (input - m_state);
        return m_state;
    }

  private:
    float m_alpha = 0.1f;
    float m_state = 0.0f;
};

class PatchImpl : public Patch
{
  public:
    void init() override
    {
        m_currentBaseMode = ChorusMode::kI;
        m_modeIplusII = false;
        m_lfoPhase = 0.0f;
        m_knobMix = 0.5f;
        m_knobTone = 0.5f;
        m_knobWidth = 0.5f;
        m_noiseState = 12345;
    }

    void setWorkingBuffer(std::span<float, kWorkingBufferSize> buffer) override
    {
        m_delayLineL.init(&buffer[0], 1024);
        m_delayLineR.init(&buffer[1024], 1024);
    }

    void processAudio(std::span<float> audioBufferLeft, std::span<float> audioBufferRight) override
    {
        const float sampleRate = static_cast<float>(kSampleRate);
        
        ChorusMode activeMode = m_modeIplusII ? ChorusMode::kIplusII : m_currentBaseMode;
        
        float rate = 0.513f;
        float depth = 1.0f;
        
        switch (activeMode)
        {
            case ChorusMode::kI:
                rate = 0.513f;
                depth = 1.0f;
                break;
            case ChorusMode::kII:
                rate = 0.863f;
                depth = 1.0f;
                break;
            case ChorusMode::kIplusII:
                rate = 9.75f;
                depth = 0.2f;
                break;
        }

        float mix = getMix(m_knobMix);
        
        float cutoff = 9000.0f;
        if (m_knobTone < 0.4f) cutoff = 6000.0f + (m_knobTone / 0.4f) * 3000.0f;
        else if (m_knobTone > 0.6f) cutoff = 9000.0f + ((m_knobTone - 0.6f) / 0.4f) * 3000.0f;
        
        m_filterL.setCutoff(cutoff, sampleRate);
        m_filterR.setCutoff(cutoff, sampleRate);
        m_inputLP.setCutoff(10000.0f, sampleRate);
        
        const float minDelaySamples = 0.00166f * sampleRate;
        const float maxDelaySamples = 0.00535f * sampleRate;
        const float delayRange = maxDelaySamples - minDelaySamples;

        for (size_t i = 0; i < audioBufferLeft.size(); ++i)
        {
            float input = (audioBufferLeft[i] + audioBufferRight[i]) * 0.5f;
            
            // Subtle noise
            input += getNoise() * 0.00005f;
            
            input = m_inputLP.process(input);

            // Triangle LFO: 4 * abs(phase - 0.5) - 1
            float lfo = 4.0f * std::abs(m_lfoPhase - 0.5f) - 1.0f;
            lfo *= depth;

            m_lfoPhase += rate / sampleRate;
            if (m_lfoPhase >= 1.0f) m_lfoPhase -= 1.0f;

            float leftMod = lfo;
            float rightMod = lfo * (1.0f - m_knobWidth * 4.0f);
            rightMod = std::clamp(rightMod, -1.0f, 1.0f);

            float delayL = minDelaySamples + delayRange * ((leftMod + 1.0f) * 0.5f);
            float delayR = minDelaySamples + delayRange * ((rightMod + 1.0f) * 0.5f);

            m_delayLineL.write(input);
            m_delayLineR.write(input);

            float wetL = m_delayLineL.read(delayL);
            float wetR = m_delayLineR.read(delayR);
            
            wetL = m_filterL.process(wetL);
            wetR = m_filterR.process(wetR);
            
            // Subtle saturation
            wetL = std::tanh(wetL * 1.05f);
            wetR = std::tanh(wetR * 1.05f);

            audioBufferLeft[i] = input * (1.0f - mix) + wetL * mix;
            audioBufferRight[i] = input * (1.0f - mix) + wetR * mix;
        }
    }

    ParameterMetadata getParameterMetadata(int /* paramIdx */) override
    {
        return ParameterMetadata{ 0.0f, 1.0f, 0.5f };
    }

    void setParamValue(int idx, float value) override
    {
        if (idx == 0) m_knobMix = value;
        else if (idx == 1) m_knobTone = value;
        else if (idx == 2) m_knobWidth = value;
    }

    void handleAction(int idx) override
    {
        if (idx == static_cast<int>(endless::ActionId::kLeftFootSwitchPress))
        {
            m_modeIplusII = false;
            m_currentBaseMode = (m_currentBaseMode == ChorusMode::kI) ? ChorusMode::kII : ChorusMode::kI;
        }
        else if (idx == static_cast<int>(endless::ActionId::kLeftFootSwitchHold))
        {
            m_modeIplusII = !m_modeIplusII;
        }
    }

    Color getStateLedColor() override
    {
        if (m_modeIplusII) return Color::kBlue;
        return (m_currentBaseMode == ChorusMode::kI) ? Color::kDarkRed : Color::kDarkLime;
    }

  private:
    float getMix(float knob) const
    {
        if (knob < 0.95f) return 0.5f;
        return 0.5f + ((knob - 0.95f) / 0.05f) * 0.5f;
    }

    float getNoise()
    {
        m_noiseState = m_noiseState * 1664525 + 1013904223;
        return (static_cast<float>(m_noiseState) / 4294967296.0f) * 2.0f - 1.0f;
    }

    enum class ChorusMode { kI, kII, kIplusII };
    ChorusMode m_currentBaseMode = ChorusMode::kI;
    bool m_modeIplusII = false;

    float m_lfoPhase = 0.0f;
    float m_knobMix = 0.5f;
    float m_knobTone = 0.5f;
    float m_knobWidth = 0.5f;
    uint32_t m_noiseState = 12345;

    DelayLine m_delayLineL;
    DelayLine m_delayLineR;
    
    OnePoleLP m_inputLP;
    OnePoleLP m_filterL;
    OnePoleLP m_filterR;
};

static PatchImpl patch;

Patch* Patch::getInstance()
{
    return &patch;
}
