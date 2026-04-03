#include "Patch.h"
/**
 * Roland Juno-60 style chorus effect implementation for Polyend Endless SDK.
 * 
 * Core Design Principles:
 * - Shared triangle LFO between left and right delay lines.
 * - Stereo width achieved via phase inversion of the LFO on the right channel.
 * - Fixed modes (I, II, and I+II) with specific rates and depths.
 * - Authentic delay range: ~1.66 ms to ~5.35 ms.
 */

#include <algorithm>
#include <cmath>
#include <span>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

/**
 * Simple delay line with linear interpolation.
 */
class DelayLine
{
  public:
    /**
     * Initializes the delay line with an external buffer.
     */
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

    /**
     * Reads a sample from the delay line with fractional delay using linear interpolation.
     */
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
        
        // Determine the active chorus mode based on base mode and I+II toggle.
        ChorusMode activeMode = m_modeIplusII ? ChorusMode::kIplusII : m_currentBaseMode;
        
        // Juno-60 hardware-matched LFO rates and depths.
        float rate = 0.513f;
        float depth = 1.0f;
        
        switch (activeMode)
        {
            case ChorusMode::kI:
                rate = 0.513f;  // ~0.513 Hz
                depth = 1.0f;
                break;
            case ChorusMode::kII:
                rate = 0.863f;  // ~0.863 Hz
                depth = 1.0f;
                break;
            case ChorusMode::kIplusII:
                rate = 9.75f;   // ~9.75 Hz
                depth = 0.2f;   // Reduced depth for I+II mode
                break;
        }

        // Authentic Mix mapping: 50/50 mix until 0.95, then ramps to 100% wet.
        float mix = getMix(m_knobMix);
        
        // Tone control: Dead zone between 0.4 and 0.6, scaling LP cutoff from 6kHz to 12kHz.
        float cutoff = 9000.0f;
        if (m_knobTone < 0.4f) cutoff = 6000.0f + (m_knobTone / 0.4f) * 3000.0f;
        else if (m_knobTone > 0.6f) cutoff = 9000.0f + ((m_knobTone - 0.6f) / 0.4f) * 3000.0f;
        
        m_filterL.setCutoff(cutoff, sampleRate);
        m_filterR.setCutoff(cutoff, sampleRate);
        
        // Fixed input pre-filtering at ~10kHz.
        m_inputLP.setCutoff(10000.0f, sampleRate);
        
        // Hardware delay constraints: ~1.66ms to ~5.35ms.
        const float minDelaySamples = 0.00166f * sampleRate;
        const float maxDelaySamples = 0.00535f * sampleRate;
        const float delayRange = maxDelaySamples - minDelaySamples;

        for (size_t i = 0; i < audioBufferLeft.size(); ++i)
        {
            // Mono input summing (authentic Juno-60 chorus input).
            float input = (audioBufferLeft[i] + audioBufferRight[i]) * 0.5f;
            
            // Subtle analog noise for character.
            input += getNoise() * 0.00005f;
            
            // Input signal conditioning.
            input = m_inputLP.process(input);

            // Shared Triangle LFO: 4 * abs(phase - 0.5) - 1
            float lfo = 4.0f * std::abs(m_lfoPhase - 0.5f) - 1.0f;
            lfo *= depth;

            m_lfoPhase += rate / sampleRate;
            if (m_lfoPhase >= 1.0f) m_lfoPhase -= 1.0f;

            // Stereo Width: Left channel uses LFO directly, Right channel is inverted.
            // Width knob controls the amount of inversion/phase-offset.
            float leftMod = lfo;
            float rightMod = lfo * (1.0f - m_knobWidth * 4.0f);
            rightMod = std::clamp(rightMod, -1.0f, 1.0f);

            // Map LFO to delay range.
            float delayL = minDelaySamples + delayRange * ((leftMod + 1.0f) * 0.5f);
            float delayR = minDelaySamples + delayRange * ((rightMod + 1.0f) * 0.5f);

            m_delayLineL.write(input);
            m_delayLineR.write(input);

            // Read from delay lines with linear interpolation.
            float wetL = m_delayLineL.read(delayL);
            float wetR = m_delayLineR.read(delayR);
            
            // Apply Tone control LP filters.
            wetL = m_filterL.process(wetL);
            wetR = m_filterR.process(wetR);
            
            // Subtle saturation (tanh) for analog warmth.
            wetL = std::tanh(wetL * 1.05f);
            wetR = std::tanh(wetR * 1.05f);

            // Final dry/wet mix.
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
        // Toggle between Mode I and Mode II on short press.
        if (idx == static_cast<int>(endless::ActionId::kLeftFootSwitchPress))
        {
            m_modeIplusII = false;
            m_currentBaseMode = (m_currentBaseMode == ChorusMode::kI) ? ChorusMode::kII : ChorusMode::kI;
        }
        // Engage/Toggle Mode I+II on long press (SDK hold).
        else if (idx == static_cast<int>(endless::ActionId::kLeftFootSwitchHold))
        {
            m_modeIplusII = !m_modeIplusII;
        }
    }

    Color getStateLedColor() override
    {
        // LED indication: Blue (I+II), Dark Red (I), Dark Lime (II).
        if (m_modeIplusII) return Color::kBlue;
        return (m_currentBaseMode == ChorusMode::kI) ? Color::kDarkRed : Color::kDarkLime;
    }

  private:
    /**
     * Helper to compute the authentic Mix value.
     */
    float getMix(float knob) const
    {
        if (knob < 0.95f) return 0.5f;
        return 0.5f + ((knob - 0.95f) / 0.05f) * 0.5f;
    }

    /**
     * Simple linear congruential generator for subtle noise.
     */
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
            float input = (audioBufferLeft[i] + audioBufferRight[i]) * 0.5f;
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
    float triangleFromPhase(float phase) const
    {
        return 4.0f * std::abs(phase - 0.5f) - 1.0f;
    }

    float getMix(float knob) const
    {
        if (knob < 0.95f) return 0.5f;
        return 0.5f + ((knob - 0.95f) / 0.05f) * 0.5f;
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