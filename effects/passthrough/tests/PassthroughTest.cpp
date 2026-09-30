#include "../../../vendor/FxPatchSDK/source/Patch.h"

#include <array>
#include <cassert>
#include <cstring>
#include <cstdio>

int main()
{
    Patch* const patch = Patch::getInstance();
    static std::array<float, Patch::kWorkingBufferSize> workingBuffer{};
    patch->setWorkingBuffer(workingBuffer);
    patch->init();
    const std::array<float, 8> leftOriginal{0.0f, -0.0f, 1.0f, -1.0f, 0.25f, -0.5f, 0.001f, 0.75f};
    const std::array<float, 8> rightOriginal{-0.5f, 0.125f, 0.0f, -0.0f, -1.0f, 1.0f, 0.8f, -0.9f};
    for (int action = 0; action < 2; action += 1)
    {
        patch->handleAction(action);
        for (int parameter = 0; parameter < endless::kParams; parameter += 1)
        {
            const auto metadata = patch->getParameterMetadata(parameter);
            assert(metadata.minValue <= metadata.defaultValue);
            assert(metadata.defaultValue <= metadata.maxValue);
            for (const float value : {0.0f, 0.5f, 1.0f})
            {
                patch->setParamValue(parameter, value);
                for (const std::size_t length : {0u, 1u, 3u, 8u})
                {
                    auto left = leftOriginal;
                    auto right = rightOriginal;
                    patch->processAudio(std::span<float>(left.data(), length),
                                        std::span<float>(right.data(), length));
                    assert(std::memcmp(left.data(), leftOriginal.data(), sizeof(left)) == 0);
                    assert(std::memcmp(right.data(), rightOriginal.data(), sizeof(right)) == 0);
                    assert(patch->getStateLedColor() == Patch::Color::kDimWhite);
                }
            }
        }
    }
    std::puts("PASS: channels preserved bit-for-bit across controls and block lengths");
}
