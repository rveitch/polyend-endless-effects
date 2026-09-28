#include "Patch.h"
/**
 * Roland Juno-60 style chorus effect implementation for Polyend Endless SDK.
 *
 * This version keeps the control surface simple while correcting several issues:
 * - More audible and mode-distinct chorus behavior.
 * - A functional tone control with a center dead zone.
 * - A sane stereo width control that adjusts stereo image instead of breaking LFO polarity.
 * - A more Juno-like I+II mode using combined slow/fast modulation instead of a ~10 Hz chop.
 */

#include <algorithm>
#include <cmath>
#include <span>
#include <cstdint>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

/**
 * Simple delay line with linear interpolation.
 */
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

/**
 * One-pole low-pass filter for tone control and signal conditioning.
 */
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
        m_lfoPhaseI = 0.0f;
        m_lfoPhaseII = 0.0f;
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
        const ChorusMode activeMode = m_modeIplusII ? ChorusMode::kIplusII : m_currentBaseMode;

        const float mix = getMix(m_knobMix);
        const float wetCutoff = getToneCutoff(m_knobTone);
        const float stereoAmount = getStereoAmount(m_knobWidth);

        m_filterL.setCutoff(wetCutoff, sampleRate);
        m_filterR.setCutoff(wetCutoff, sampleRate);
        m_inputLP.setCutoff(12000.0f, sampleRate);

        const float minDelaySamples = 0.00166f * sampleRate;
        const float maxDelaySamples = 0.00535f * sampleRate;

        float rateI = 0.513f;
        float rateII = 0.863f;
        float centerDelayMs = 3.50f;
        float modDepthMs = 1.10f;

        switch (activeMode)
        {
            case ChorusMode::kI:
                centerDelayMs = 3.70f;
                modDepthMs = 0.95f;
                break;
            case ChorusMode::kII:
                centerDelayMs = 3.35f;
                modDepthMs = 1.45f;
                break;
            case ChorusMode::kIplusII:
                centerDelayMs = 3.50f;
                modDepthMs = 1.25f;
                break;
        }

        for (size_t i = 0; i < audioBufferLeft.size(); ++i)
        {
            const float dryLeft = audioBufferLeft[i];
            const float dryRight = audioBufferRight[i];
            float input = (dryLeft + dryRight) * 0.5f;
            input += getNoise() * 0.00005f;
            input = m_inputLP.process(input);

            const float lfoI = triangleFromPhase(m_lfoPhaseI);
            m_lfoPhaseI += rateI / sampleRate;
            if (m_lfoPhaseI >= 1.0f) m_lfoPhaseI -= 1.0f;

            float mod = lfoI;

            if (activeMode == ChorusMode::kII)
            {
                mod = triangleFromPhase(m_lfoPhaseII);
                m_lfoPhaseII += rateII / sampleRate;
                if (m_lfoPhaseII >= 1.0f) m_lfoPhaseII -= 1.0f;
            }
            else if (activeMode == ChorusMode::kIplusII)
            {
                const float lfoII = triangleFromPhase(m_lfoPhaseII);
                m_lfoPhaseII += rateII / sampleRate;
                if (m_lfoPhaseII >= 1.0f) m_lfoPhaseII -= 1.0f;

                mod = std::clamp((lfoI * 0.6f) + (lfoII * 0.4f), -1.0f, 1.0f);
            }

            const float centerDelaySamples = (centerDelayMs * 0.001f) * sampleRate;
            const float modDepthSamples = (modDepthMs * 0.001f) * sampleRate;

            const float delayL = std::clamp(centerDelaySamples + modDepthSamples * mod, minDelaySamples, maxDelaySamples);
            const float delayR = std::clamp(centerDelaySamples - modDepthSamples * mod, minDelaySamples, maxDelaySamples);

            m_delayLineL.write(input);
            m_delayLineR.write(input);

            float wetL = m_delayLineL.read(delayL);
            float wetR = m_delayLineR.read(delayR);

            wetL = m_filterL.process(wetL);
            wetR = m_filterR.process(wetR);

            wetL = std::tanh(wetL * 1.2f) * 1.15f;
            wetR = std::tanh(wetR * 1.2f) * 1.15f;

            const float wetMid = 0.5f * (wetL + wetR);
            wetL = wetMid + (wetL - wetMid) * stereoAmount;
            wetR = wetMid + (wetR - wetMid) * stereoAmount;

            // Keep the wet state running even in the dry dead zone.
            // Leave original samples untouched there, including signed zero.
            if (mix > 0.0f)
            {
                audioBufferLeft[i] = dryLeft * (1.0f - mix) + wetL * mix;
                audioBufferRight[i] = dryRight * (1.0f - mix) + wetR * mix;
            }
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
    float triangleFromPhase(float phase) const
    {
        return 4.0f * std::abs(phase - 0.5f) - 1.0f;
    }

    float getMix(float knob) const
    {
        // Temporary diagnostic mapping, not the final Juno control design.
        const float position = std::clamp(knob, 0.0f, 1.0f);
        if (position <= 0.05f) return 0.0f;
        if (position < 0.5f) return (position - 0.05f) * (0.5f / 0.45f);
        return position;
    }

    float getToneCutoff(float knob) const
    {
        if (knob >= 0.4f && knob <= 0.6f)
        {
            return 8500.0f;
        }

        if (knob < 0.4f)
        {
            const float t = knob / 0.4f;
            return 4500.0f + t * 4000.0f;
        }

        const float t = (knob - 0.6f) / 0.4f;
        return 8500.0f + t * 3500.0f;
    }

    float getStereoAmount(float knob) const
    {
        if (knob <= 0.4f)
        {
            return knob / 0.4f;
        }

        if (knob < 0.6f)
        {
            return 1.0f;
        }

        return 1.0f + ((knob - 0.6f) / 0.4f) * 0.25f;
    }

    float getNoise()
    {
        m_noiseState = m_noiseState * 1664525u + 1013904223u;
        return (static_cast<float>(m_noiseState) / 4294967296.0f) * 2.0f - 1.0f;
    }

    enum class ChorusMode { kI, kII, kIplusII };

    ChorusMode m_currentBaseMode = ChorusMode::kI;
    bool m_modeIplusII = false;

    float m_lfoPhaseI = 0.0f;
    float m_lfoPhaseII = 0.0f;
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