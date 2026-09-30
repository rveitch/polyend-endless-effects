#include "../../vendor/FxPatchSDK/source/Patch.h"

namespace
{
class Passthrough final : public Patch
{
  public:
    void init() override {}
    void setWorkingBuffer(std::span<float, kWorkingBufferSize> /* buffer */) override {}

    // The SDK processes in place. Leaving both buffers untouched is exact unity
    // gain, with no crossfeed, filtering, clipping, or added sample delay.
    void processAudio(std::span<float> /* left */, std::span<float> /* right */) override {}

    ParameterMetadata getParameterMetadata(int /* index */) override
    {
        return ParameterMetadata{0.0f, 1.0f, 0.5f};
    }

    void setParamValue(int /* index */, float /* value */) override {}
    void handleAction(int /* index */) override {}
    Color getStateLedColor() override { return Color::kDimWhite; }
};

#if !defined(ENDLESS_DESKTOP)
Passthrough patch;
#endif
}

#if defined(ENDLESS_DESKTOP)
#include "../../desktop/PatchFactory.h"
static void destroyDesktopPatch(Patch* instance)
{
    delete static_cast<Passthrough*>(instance);
}
endlessDesktop::PatchHandle endlessDesktop::createPatch()
{
    return PatchHandle{new Passthrough{}, PatchDeleter{destroyDesktopPatch}};
}
#else
Patch* Patch::getInstance()
{
    return &patch;
}

#endif
