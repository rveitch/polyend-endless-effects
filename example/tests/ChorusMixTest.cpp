#include "../source/PatchImpl.cpp"
#include <array>
#include <bit>
#include <cstdio>
#include <cstdlib>

int main()
{
    static std::array<float, Patch::kWorkingBufferSize> wetBuffer{};
    static std::array<float, Patch::kWorkingBufferSize> testBuffer{};
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
            for (std::size_t i = 0; i < left.size(); i += 1)
            {
                const bool exactDry = expectedWet == 0.0f;
                const bool validLeft = exactDry
                    ? std::bit_cast<uint32_t>(left[i]) == std::bit_cast<uint32_t>(originalLeft[i])
                    : std::abs(left[i] - (originalLeft[i] * (1.0f - expectedWet) + wetLeft[i] * expectedWet)) < 0.000001f;
                const bool validRight = exactDry
                    ? std::bit_cast<uint32_t>(right[i]) == std::bit_cast<uint32_t>(originalRight[i])
                    : std::abs(right[i] - (originalRight[i] * (1.0f - expectedWet) + wetRight[i] * expectedWet)) < 0.000001f;
                if (!validLeft || !validRight)
                {
                    std::fprintf(stderr, "FAIL knob %.3f block %d sample %zu: dry preservation or blend\n", knob, block, i);
                    return EXIT_FAILURE;
                }
            }
        }
    }
    std::puts("PASS: exact dry through 5%, continuous blends, independent dry channels, wet state advances");
}
