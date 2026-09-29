#include "Patch.h"
/** Juno-60 candidate: recording-derived motion and provisional vintage wet filtering.
 * Filter coefficients and gain are engineering candidates, not a circuit-exact model.
 * Keep the diagnostic Mix mapping until hardware validation is complete.
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
            for (size_t i = 0; i < m_size; i += 1)
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

        float readPos = static_cast<float>(m_writePos) - 1.0f - delaySamples;
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

// Second-order Butterworth low-pass. Coefficients ramp over 20 ms after
// parameter changes; the first setup is immediate. All storage is inline.
class VintageLowPass
{
  public:
    void setCutoff(float frequency)
    {
        if (frequency == m_cutoff) return;
        m_cutoff = frequency;
        const float k = std::tan(static_cast<float>(M_PI) * frequency / 48000.0f);
        const float norm = 1.0f / (1.0f + 1.41421356237f * k + k * k);
        m_target[0] = k * k * norm;
        m_target[1] = 2.0f * m_target[0];
        m_target[2] = m_target[0];
        m_target[3] = 2.0f * (k * k - 1.0f) * norm;
        m_target[4] = (1.0f - 1.41421356237f * k + k * k) * norm;
        if (!m_initialized)
        {
            for (int i = 0; i < 5; i += 1) m_coeff[i] = m_target[i];
            m_initialized = true;
        }
        else m_remaining = 960;
    }

    float process(float input)
    {
        if (m_remaining > 0)
        {
            for (int i = 0; i < 5; i += 1)
                m_coeff[i] += (m_target[i] - m_coeff[i]) / static_cast<float>(m_remaining);
            m_remaining -= 1;
        }
        const float output = m_coeff[0] * input + m_z1;
        m_z1 = m_coeff[1] * input - m_coeff[3] * output + m_z2;
        m_z2 = m_coeff[2] * input - m_coeff[4] * output;
        return output;
    }
  private:
    float m_coeff[5]{};
    float m_target[5]{};
    float m_cutoff = -1.0f;
    float m_z1 = 0.0f;
    float m_z2 = 0.0f;
    int m_remaining = 0;
    bool m_initialized = false;
};

class ChorusMotion
{
  public:
    void advance()
    {
        for (int mode = 0; mode < 3; mode += 1)
        {
            m_phase[mode] += phaseIncrements[mode];
        }
    }
    float delayMs(int mode, bool right) const
    {
        const float phase = static_cast<float>(m_phase[mode]) * (1.0f / 4294967296.0f);
        if (mode == 2)
            return 3.5f + 0.2f * std::sin(2.0f * static_cast<float>(M_PI) * phase);
        const float triangle = 4.0f * std::abs(phase - 0.5f) - 1.0f;
        return 3.505f + (right ? -1.845f : 1.845f) * triangle;
    }
  private:
    // Unsigned wrap gives continuous phase without software double arithmetic.
    static constexpr uint32_t phaseIncrements[3] = {45902u, 77220u, 872415u};
    uint32_t m_phase[3]{};
};

class PatchImpl : public Patch
{
  public:
    void init() override
    {
        m_currentBaseMode = ChorusMode::kI;
        m_modeIplusII = false;
        m_motion = ChorusMotion{};
        m_inputLP = VintageLowPass{};
        for (int mode = 0; mode < 3; mode += 1)
        {
            m_filterL[mode] = VintageLowPass{};
            m_filterR[mode] = VintageLowPass{};
            m_weights[mode] = mode == 0 ? 1.0f : 0.0f;
        }
        m_targetMode = 0;
        m_transitionRemaining = 0;
        m_knobMix = m_knobTone = m_knobWidth = 0.5f;
    }

    void setWorkingBuffer(std::span<float, kWorkingBufferSize> buffer) override
    {
        m_delayLineL.init(&buffer[0], 1024);
        m_delayLineR.init(&buffer[1024], 1024);
    }

    void processAudio(std::span<float> audioBufferLeft, std::span<float> audioBufferRight) override
    {
        const int activeMode = m_modeIplusII ? 2 : static_cast<int>(m_currentBaseMode);
        if (activeMode != m_targetMode)
        {
            m_targetMode = activeMode;
            m_transitionRemaining = 960;
        }
        const float mix = getMix(m_knobMix);
        const float stereoAmount = getStereoAmount(m_knobWidth);
        m_inputLP.setCutoff(8000.0f);
        for (int mode = 0; mode < 3; mode += 1)
        {
            m_filterL[mode].setCutoff(getToneCutoff(m_knobTone));
            m_filterR[mode].setCutoff(getToneCutoff(m_knobTone));
        }
        for (size_t i = 0; i < audioBufferLeft.size(); i += 1)
        {
            const float dryLeft = audioBufferLeft[i];
            const float dryRight = audioBufferRight[i];
            const float input = m_inputLP.process((dryLeft + dryRight) * 0.5f);
            m_delayLineL.write(input);
            m_delayLineR.write(input);
            float wetL = 0.0f;
            float wetR = 0.0f;
            for (int mode = 0; mode < 3; mode += 1)
            {
                if (m_transitionRemaining > 0)
                {
                    const float target = mode == m_targetMode ? 1.0f : 0.0f;
                    m_weights[mode] += (target - m_weights[mode]) / static_cast<float>(m_transitionRemaining);
                }
                // Run all paths continuously so mode fades do not expose stale state.
                const float left = m_filterL[mode].process(m_delayLineL.read(m_motion.delayMs(mode, false) * 48.0f));
                const float right = m_filterR[mode].process(m_delayLineR.read(m_motion.delayMs(mode, true) * 48.0f));
                wetL += left * m_weights[mode];
                wetR += right * m_weights[mode];
            }
            if (m_transitionRemaining > 0) m_transitionRemaining -= 1;
            m_motion.advance();
            // Conservative filter headroom budget, not analog gain calibration.
            wetL *= 0.7f;
            wetR *= 0.7f;
            const float mid = (wetL + wetR) * 0.5f;
            wetL = mid + (wetL - mid) * stereoAmount;
            wetR = mid + (wetR - mid) * stereoAmount;
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
        if (!std::isfinite(value)) return;
        value = std::clamp(value, 0.0f, 1.0f);
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
        // Temporary diagnostic mapping, not the final Juno control design.
        const float position = std::clamp(knob, 0.0f, 1.0f);
        if (position <= 0.05f) return 0.0f;
        if (position < 0.5f) return (position - 0.05f) * (0.5f / 0.45f);
        return position;
    }

    float getToneCutoff(float knob) const
    {
        if (knob < 0.4f) return 3600.0f + (knob / 0.4f) * 900.0f;
        if (knob <= 0.6f) return 4500.0f;
        return 4500.0f + ((knob - 0.6f) / 0.4f) * 900.0f;
    }

    float getStereoAmount(float knob) const
    {
        return std::min(knob / 0.4f, 1.0f);
    }

    enum class ChorusMode { kI, kII, kIplusII };

    ChorusMode m_currentBaseMode = ChorusMode::kI;
    bool m_modeIplusII = false;

    ChorusMotion m_motion;
    float m_weights[3] = {1.0f, 0.0f, 0.0f};
    int m_targetMode = 0;
    int m_transitionRemaining = 0;
    float m_knobMix = 0.5f;
    float m_knobTone = 0.5f;
    float m_knobWidth = 0.5f;

    DelayLine m_delayLineL;
    DelayLine m_delayLineR;

    VintageLowPass m_inputLP;
    VintageLowPass m_filterL[3];
    VintageLowPass m_filterR[3];
};

static PatchImpl patch;

Patch* Patch::getInstance()
{
    return &patch;
}