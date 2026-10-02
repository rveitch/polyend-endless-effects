#include "../PatchImpl.cpp"
#include <array>
#include <bit>
#include <cstdio>
#include <cstdlib>

int main()
{
    static std::array<float, Patch::kWorkingBufferSize> wetBuffer{};
    static std::array<float, Patch::kWorkingBufferSize> testBuffer{};
    // Chosen post-blend correction: +4.7 dB at noon and above,
    // with a continuous approach from unity at the true-dry endpoint.
    const float correction = std::pow(10.0f, 4.7f / 20.0f);
    for (const auto& [knob, expectedWet] : std::array<std::pair<float, float>, 7>{{
             {0.0f, 0.0f}, {0.025f, 0.0f}, {0.05f, 0.0f},
             {0.275f, 0.25f}, {0.5f, 0.5f}, {0.75f, 0.75f}, {1.0f, 1.0f}}})
    {
        PatchImpl wet;
        PatchImpl test;
        wet.setWorkingBuffer(wetBuffer);
        test.setWorkingBuffer(testBuffer);
        wet.init();
        test.init();
        wet.setParamValue(0, 1.0f);
        test.setParamValue(0, knob);
        for (int block = 0; block < 40; block += 1)
        {
            std::array<float, 64> left{}, right{};
            for (std::size_t i = 0; i < left.size(); i += 1)
            {
                left[i] = 0.3f * std::sin(static_cast<float>(block * 64 + i) * 0.17f);
                right[i] = -0.2f * std::cos(static_cast<float>(block * 64 + i) * 0.31f);
            }
            left[0] = -0.0f;
            const auto originalLeft = left;
            const auto originalRight = right;
            auto wetLeft = left;
            auto wetRight = right;
            wet.processAudio(wetLeft, wetRight);
            test.processAudio(left, right);
            const float gain = 1.0f + (correction - 1.0f) * std::min(2.0f * expectedWet, 1.0f);
            for (std::size_t i = 0; i < left.size(); i += 1)
            {
                const bool exactDry = expectedWet == 0.0f;
                const bool validLeft = exactDry
                    ? std::bit_cast<uint32_t>(left[i]) == std::bit_cast<uint32_t>(originalLeft[i])
                    : std::abs(left[i] - (originalLeft[i] * (1.0f - expectedWet) + wetLeft[i] * expectedWet / correction) * gain) < 0.000001f;
                const bool validRight = exactDry
                    ? std::bit_cast<uint32_t>(right[i]) == std::bit_cast<uint32_t>(originalRight[i])
                    : std::abs(right[i] - (originalRight[i] * (1.0f - expectedWet) + wetRight[i] * expectedWet / correction) * gain) < 0.000001f;
                if (!validLeft || !validRight)
                {
                    std::fprintf(stderr, "FAIL knob %.3f block %d sample %zu: dry preservation or blend\n", knob, block, i);
                    return EXIT_FAILURE;
                }
            }
        }
    }
    // Fully wet must contain delayed audio, not the original dry impulse.
    PatchImpl endpoint;
    endpoint.setWorkingBuffer(testBuffer); endpoint.init(); endpoint.setParamValue(0, 1.0f);
    std::array<float, 600> impulseLeft{}, impulseRight{};
    impulseLeft[0] = impulseRight[0] = 0.2f;
    endpoint.processAudio(impulseLeft, impulseRight);
    float delayedPeak = 0.0f;
    for (std::size_t i = 1; i < impulseLeft.size(); i += 1)
        delayedPeak = std::max(delayedPeak, std::max(std::abs(impulseLeft[i]), std::abs(impulseRight[i])));
    if (impulseLeft[0] != 0.0f || impulseRight[0] != 0.0f || delayedPeak <= 0.00001f) {
        std::fprintf(stderr, "FAIL: full Mix must be delayed wet audio, not dry passthrough\n");
        return EXIT_FAILURE;
    }
    std::puts("PASS: exact dry through 5%, continuous blends, independent dry channels, wet state advances");
}
