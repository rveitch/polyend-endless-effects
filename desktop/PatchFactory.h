#pragma once
#include "../vendor/FxPatchSDK/source/Patch.h"
#include <memory>
namespace endlessDesktop
{
struct PatchDeleter
{
    void (*destroy)(Patch*);
    void operator()(Patch* patch) const { if (patch) destroy(patch); }
};
using PatchHandle = std::unique_ptr<Patch, PatchDeleter>;
PatchHandle createPatch();
}
