#include "../PatchImpl.cpp"
#include <array>
#include <cstdio>
#include <cstdlib>

int failures = 0;
void check(bool passed, const char* name)
{
    if (!passed) { std::fprintf(stderr, "FAIL: %s\n", name); failures += 1; }
}

float measure(float frequency, bool combined = false)
{
    static std::array<float, Patch::kWorkingBufferSize> buffer{};
    PatchImpl effect;
    effect.init(); effect.setWorkingBuffer(buffer); effect.setParamValue(0, 1.0f);
    if (combined) effect.handleAction(static_cast<int>(endless::ActionId::kLeftFootSwitchHold));
    double energy = 0.0;
    float difference = 0.0f;
    for (int block = 0; block < 1500; block += 1)
    {
        std::array<float, 64> left{}, right{};
        for (int i = 0; i < 64; i += 1)
            left[i] = right[i] = 0.1f * std::sin(2.0f * static_cast<float>(M_PI) * frequency * static_cast<float>(block * 64 + i) / 48000.0f);
        effect.processAudio(left, right);
        if (block > 300)
            for (int i = 0; i < 64; i += 1)
            {
                energy += static_cast<double>(left[i]) * left[i];
                difference = std::max(difference, std::abs(left[i] - right[i]));
            }
    }
    if (combined) check(difference > 0.001f, "slow combined mode has stereo wet movement");
    return static_cast<float>(std::sqrt(energy / (1199.0 * 64.0)));
}

void motionTest()
{
    ChorusMotion motion;
    float minimum[3] = {100.0f, 100.0f, 100.0f};
    float maximum[3]{};
    int crossings[3]{};
    float previous[3]{};
    int lastBlueCrossing = -1;
    for (int i = 0; i < 480000; i += 1)
    {
        for (int mode = 0; mode < 3; mode += 1)
        {
            const float value = motion.delayMs(mode, false);
            minimum[mode] = std::min(minimum[mode], value);
            maximum[mode] = std::max(maximum[mode], value);
            const float center = mode == 2 ? 3.455f : 3.505f;
            if (i > 0 && previous[mode] < center && value >= center) {
                crossings[mode] += 1;
                if (mode == 2) {
                    if (lastBlueCrossing >= 0)
                        check(std::abs(48000.0f / static_cast<float>(i - lastBlueCrossing) - 0.4f) < 0.0001f,
                              "blue measured cycle rate is 0.4 Hz");
                    lastBlueCrossing = i;
                }
            }
            previous[mode] = value;
            if (mode < 2)
                check(std::abs(2.0f * center - value - motion.delayMs(mode, true)) < 0.000002f, "red and green opposing stereo relationship");
            else {
                const float normalizedLeft = (value - center) / 2.135f;
                const float normalizedRight = (motion.delayMs(mode, true) - center) / 2.135f;
                check(std::abs(normalizedLeft * normalizedLeft + normalizedRight * normalizedRight - 1.0f) < 0.00001f,
                      "blue quarter-cycle stereo relationship");
            }
        }
        motion.advance();
    }
    for (int mode = 0; mode < 3; mode += 1)
    {
        check(std::abs(minimum[mode] - (mode == 2 ? 1.32f : 1.66f)) < 0.001f, "minimum delay");
        check(std::abs(maximum[mode] - (mode == 2 ? 5.59f : 5.35f)) < 0.001f, "maximum delay");
    }
    check(crossings[0] == 5 && crossings[1] == 8 && (crossings[2] == 3 || crossings[2] == 4), "modulation cycle counts over ten seconds");
}

void headroomAndTransitions(float inputPeak)
{
    static std::array<float, Patch::kWorkingBufferSize> buffer{};
    float peak = 0.0f;
    for (const float frequency : {80.0f, 320.0f, 1000.0f, 5000.0f, 18000.0f})
    {
        PatchImpl effect; effect.init(); effect.setWorkingBuffer(buffer); effect.setParamValue(0, 1.0f);
        for (int block = 0; block < 1500; block += 1)
        {
            if (block % 71 == 0) effect.handleAction(static_cast<int>(endless::ActionId::kLeftFootSwitchHold));
            if (block % 127 == 0) effect.handleAction(static_cast<int>(endless::ActionId::kLeftFootSwitchPress));
            if (block % 53 == 0) effect.setParamValue(1, block % 2 ? 0.0f : 1.0f);
            effect.setParamValue(2, block % 2 ? 0.0f : 1.0f);
            std::array<float, 64> left{}, right{};
            for (int i = 0; i < 64; i += 1)
            {
                const float value = std::sin(2.0f * static_cast<float>(M_PI) * frequency * static_cast<float>(block * 64 + i) / 48000.0f);
                left[i] = right[i] = value >= 0.0f ? inputPeak : -inputPeak;
            }
            effect.processAudio(left, right);
            for (int i = 0; i < 64; i += 1)
            {
                check(std::isfinite(left[i]) && std::isfinite(right[i]), "finite rapid-transition output");
                peak = std::max(peak, std::max(std::abs(left[i]), std::abs(right[i])));
            }
        }
    }
    std::printf("Rapid-transition input %.2f square-wave output peak: %.6f\n", inputPeak, peak);
    if (inputPeak <= 0.5f)
        check(peak <= 1.0f, "headroom with six dB input allowance and rapid changes");
    else {
        // The selected linear boost intentionally trades full-scale input
        // headroom for output level. Keep this limitation explicitly tested.
        check(peak > 1.0f && peak < 1.72f, "known hot-input overload stays finite and bounded by linear correction");
    }
    PatchImpl effect; effect.init(); effect.setWorkingBuffer(buffer); effect.setParamValue(0, 1.0f);
    std::array<float, 64> silence{}; auto right = silence;
    effect.processAudio(silence, right);
    for (int i = 0; i < 64; i += 1) check(silence[i] == 0.0f && right[i] == 0.0f, "no added noise");
}

void blockPartitionTest()
{
    static std::array<float, Patch::kWorkingBufferSize> firstBuffer{}, secondBuffer{};
    PatchImpl first, second; first.init(); second.init();
    first.setWorkingBuffer(firstBuffer); second.setWorkingBuffer(secondBuffer);
    first.setParamValue(0, 1.0f); second.setParamValue(0, 1.0f);
    for (int block = 0; block < 100; block += 1)
    {
        if (block % 13 == 0)
        {
            first.handleAction(static_cast<int>(endless::ActionId::kLeftFootSwitchHold));
            second.handleAction(static_cast<int>(endless::ActionId::kLeftFootSwitchHold));
            first.setParamValue(1, block % 2 ? 0.0f : 1.0f);
            second.setParamValue(1, block % 2 ? 0.0f : 1.0f);
        }
        std::array<float, 64> left{}, right{};
        for (int i = 0; i < 64; i += 1)
            left[i] = right[i] = 0.2f * std::sin(static_cast<float>(block * 64 + i) * 0.137f);
        auto otherLeft = left; auto otherRight = right;
        first.processAudio(left, right);
        for (int offset = 0; offset < 64; offset += 8)
            second.processAudio(std::span<float>(otherLeft).subspan(offset, 8), std::span<float>(otherRight).subspan(offset, 8));
        for (int i = 0; i < 64; i += 1)
            check(left[i] == otherLeft[i] && right[i] == otherRight[i], "callback partition independence during transitions");
    }
}

// Compare a switching render against continuously running pure-mode renders.
// Interrupt the first fade before completion, then check the exact endpoint.
void modeFadeTest()
{
    static std::array<std::array<float, Patch::kWorkingBufferSize>, 4> buffers{};
    std::array<PatchImpl, 4> effects;
    for (int index = 0; index < 4; index += 1)
    {
        effects[index].init(); effects[index].setWorkingBuffer(buffers[index]);
        effects[index].setParamValue(0, 1.0f);
    }
    effects[2].handleAction(static_cast<int>(endless::ActionId::kLeftFootSwitchPress));
    effects[3].handleAction(static_cast<int>(endless::ActionId::kLeftFootSwitchHold));
    float startingWeights[3] = {1.0f, 0.0f, 0.0f};
    int target = 0;
    int startSample = 0;
    for (int sample = 0; sample < 3500; sample += 1)
    {
        if (sample == 1200)
        {
            effects[0].handleAction(static_cast<int>(endless::ActionId::kLeftFootSwitchPress));
            target = 1; startSample = sample;
        }
        if (sample == 1500)
        {
            startingWeights[0] = 1.0f - 300.0f / 960.0f;
            startingWeights[1] = 300.0f / 960.0f;
            effects[0].handleAction(static_cast<int>(endless::ActionId::kLeftFootSwitchHold));
            target = 2; startSample = sample;
        }
        float values[4][2]{};
        for (int index = 0; index < 4; index += 1)
        {
            std::array<float, 1> left{0.2f * std::sin(static_cast<float>(sample) * 0.17f)};
            auto right = left;
            effects[index].processAudio(left, right);
            values[index][0] = left[0]; values[index][1] = right[0];
        }
        if (sample < 1200) continue;
        const float progress = std::min(static_cast<float>(sample - startSample + 1) / 960.0f, 1.0f);
        for (int channel = 0; channel < 2; channel += 1)
        {
            float expected = 0.0f;
            for (int mode = 0; mode < 3; mode += 1)
            {
                const float weight = startingWeights[mode] * (1.0f - progress) + (mode == target ? progress : 0.0f);
                expected += weight * values[mode + 1][channel];
            }
            check(std::abs(values[0][channel] - expected) < 0.000002f, "interrupted mode fade matches continuous references");
        }
    }
}

int main()
{
    modeFadeTest();
    blockPartitionTest();
    motionTest();
    headroomAndTransitions(0.5f);
    headroomAndTransitions(0.99f);
    std::array<float, 32> storage{}; DelayLine delay; delay.init(storage.data(), storage.size());
    for (int i = 0; i < 80; i += 1)
    {
        delay.write(i == 0 || i == 40 ? 1.0f : 0.0f);
        check(std::abs(delay.read(5.0f) - (i == 5 || i == 45 ? 1.0f : 0.0f)) < 0.000001f, "five-sample delay including wrap");
        check(std::abs(delay.read(5.5f) - (i == 5 || i == 6 || i == 45 || i == 46 ? 0.5f : 0.0f)) < 0.000001f, "fractional delay including wrap");
    }
    const float reference = measure(1000.0f, true);
    const float at5k = 20.0f * std::log10(measure(5000.0f, true) / reference);
    const float at10k = 20.0f * std::log10(measure(10000.0f, true) / reference);
    std::printf("Wet response relative to 1k: 5k %.2f dB, 10k %.2f dB\n", at5k, at10k);
    check(at5k <= -4.0f && at5k >= -7.0f, "vintage 5 kHz rolloff");
    check(at10k <= -18.0f && at10k >= -25.0f, "vintage 10 kHz rolloff including interpolation");
    if (!failures) std::puts("PASS: vintage candidate regressions");
    return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
