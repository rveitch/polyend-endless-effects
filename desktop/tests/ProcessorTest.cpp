#include "Processor.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>

void require(bool condition, const char* message)
{
    if (!condition) { std::fprintf(stderr, "FAIL: %s\n", message); std::exit(EXIT_FAILURE); }
}
void set(EndlessProcessor& processor, const char* name, float value)
{
    for (auto* parameter : processor.getParameters())
        if (parameter->getName(100) == name) { parameter->setValueNotifyingHost(value); return; }
    require(false, name);
}
juce::AudioBuffer<float> input(int frames)
{
    juce::AudioBuffer<float> audio(2, frames);
    for (int i = 0; i < frames; i += 1) {
        audio.setSample(0, i, 0.2f * std::sin(static_cast<float>(i) * 0.17f));
        audio.setSample(1, i, 0.15f * std::cos(static_cast<float>(i) * 0.13f));
    }
    return audio;
}
void equal(const juce::AudioBuffer<float>& a, const juce::AudioBuffer<float>& b, const char* message)
{
    for (int ch = 0; ch < 2; ch += 1)
        for (int i = 0; i < a.getNumSamples(); i += 1)
            require(a.getSample(ch, i) == b.getSample(ch, i), message);
}
int main()
{
    juce::ScopedJuceInitialiser_GUI initialize;
    juce::ScopedNoDenormals noDenormals;
    juce::MidiBuffer midi;
    EndlessProcessor a, b;
    a.prepareToPlay(48000, 7); b.prepareToPlay(48000, 127);
    set(a, "Mix", 1); set(b, "Mix", 1);
    auto reference = endlessDesktop::createPatch();
    std::vector<float> storage(Patch::kWorkingBufferSize);
    reference->setWorkingBuffer(std::span<float, Patch::kWorkingBufferSize>(storage.data(), storage.size()));
    reference->init(); reference->setParamValue(0, 1); reference->setParamValue(1, 0.5f); reference->setParamValue(2, 0.5f);
    for (int block = 0; block < 100; block += 1) {
        auto audio = input(127); auto expected = input(127); auto second = input(127);
        a.processBlock(audio, midi); b.processBlock(second, midi);
        reference->processAudio({expected.getWritePointer(0), 127}, {expected.getWritePointer(1), 127});
        equal(audio, expected, "chunked callback parity"); equal(audio, second, "independent processors");
    }
    set(a, "I+II", 1); set(a, "Tone", 0.9f);
    auto changed = input(127); auto original = input(127); auto expected = input(127);
    a.processBlock(changed, midi); b.processBlock(original, midi);
    reference->processAudio({expected.getWritePointer(0),127}, {expected.getWritePointer(1),127});
    equal(original, expected, "changing one instance must not affect another");
    for (const auto mode : {0, 1, 2, 1, 0, 2}) {
        a.prepareToPlay(48000, 64);
        reference->setWorkingBuffer(std::span<float, Patch::kWorkingBufferSize>(storage.data(), storage.size()));
        reference->init(); reference->setParamValue(0, 1); reference->setParamValue(2, 0.5f);
        set(a, "Tone", 0.5f); reference->setParamValue(1, 0.5f);
        set(a, "Base mode", mode == 1 ? 1.0f : 0.0f); set(a, "I+II", mode == 2 ? 1.0f : 0.0f);
        if (mode == 1) reference->handleAction(0);
        if (mode == 2) reference->handleAction(1);
        for (int block = 0; block < 20; block += 1) {
            auto actual = input(127); auto modeExpected = input(127);
            a.processBlock(actual, midi);
            reference->processAudio({modeExpected.getWritePointer(0),127}, {modeExpected.getWritePointer(1),127});
            equal(actual, modeExpected, "mode action reference parity");
        }
    }
    juce::MemoryBlock saved; a.getStateInformation(saved);
    EndlessProcessor restored; restored.setStateInformation(saved.getData(), static_cast<int>(saved.getSize()));
    restored.prepareToPlay(48000, 64);
    require(restored.getParameters()[1]->getValue() == a.getParameters()[1]->getValue(), "saved Tone");
    require(restored.getParameters()[4]->getValue() == 1, "saved combined mode");
    a.prepareToPlay(48000, 64); auto first = input(257); auto second = input(257);
    a.processBlock(first, midi); restored.processBlock(second, midi); equal(first, second, "saved state and reset parity");
    set(a, "Bypass", 1); auto bypassed = input(257); auto dry = input(257);
    a.processBlock(bypassed, midi); equal(bypassed, dry, "exact bypass");
    a.prepareToPlay(44100, 128); set(a, "Bypass", 0); auto unsupported = input(128); auto raw = input(128);
    a.processBlock(unsupported, midi); equal(unsupported, raw, "unsupported rate stays dry");
    require(!a.supportedRate(), "unsupported rate warning");
    const char invalid[] = "<wrong/>";
    restored.setStateInformation(invalid, sizeof(invalid));
    require(restored.getParameters()[4]->getValue() == 1, "invalid state preserved");
    auto xml = juce::AudioProcessor::getXmlFromBinary(saved.getData(), static_cast<int>(saved.getSize()));
    require(xml != nullptr, "state XML");
    auto malformed = juce::ValueTree::fromXml(*xml);
    for (auto child : malformed) if (child.getProperty("id").toString() == "knob1") child.setProperty("value", 0.1f, nullptr);
    malformed.getChild(0).setProperty("value", 500.0f, nullptr);
    juce::MemoryBlock invalidState;
    juce::AudioProcessor::copyXmlToBinary(*malformed.createXml(), invalidState);
    restored.setStateInformation(invalidState.getData(), static_cast<int>(invalidState.getSize()));
    require(restored.getParameters()[1]->getValue() == a.getParameters()[1]->getValue(), "out-of-range state rejected atomically");
    malformed = juce::ValueTree::fromXml(*xml);
    malformed.getChild(0).setProperty("id", "knob1", nullptr);
    for (auto child : malformed) if (child.getProperty("id").toString() == "knob1") child.setProperty("value", 0.1f, nullptr);
    juce::AudioProcessor::copyXmlToBinary(*malformed.createXml(), invalidState);
    restored.setStateInformation(invalidState.getData(), static_cast<int>(invalidState.getSize()));
    require(restored.getParameters()[1]->getValue() == a.getParameters()[1]->getValue(), "duplicate state rejected");
    EndlessProcessor trimmed, unity;
    set(trimmed, "Output trim", 1.0f); // +6 dB in the -6..+6 dB range.
    trimmed.prepareToPlay(48000, 64); unity.prepareToPlay(48000, 64);
    auto boosted = input(257); auto plain = input(257);
    trimmed.processBlock(boosted, midi); unity.processBlock(plain, midi);
    const float boost = juce::Decibels::decibelsToGain(6.0f);
    for (int ch = 0; ch < 2; ch += 1)
        for (int i = 0; i < 257; i += 1)
            require(std::abs(boosted.getSample(ch, i) - plain.getSample(ch, i) * boost) < 1.0e-7f, "trim after complete blend");
    set(trimmed, "Bypass", 1); auto trimmedBypass = input(257); auto bypassReference = input(257);
    trimmed.processBlock(trimmedBypass, midi); equal(trimmedBypass, bypassReference, "trim excluded from parameter bypass");
    set(trimmed, "Bypass", 0); trimmedBypass = input(257);
    trimmed.processBlockBypassed(trimmedBypass, midi); equal(trimmedBypass, bypassReference, "trim excluded from host bypass");
    juce::MemoryBlock trimState; trimmed.getStateInformation(trimState);
    EndlessProcessor trimRestored; trimRestored.setStateInformation(trimState.getData(), static_cast<int>(trimState.getSize()));
    require(trimRestored.getParameters()[6]->getValue() == 1.0f, "trim saved and restored");
    auto oldXml = juce::AudioProcessor::getXmlFromBinary(trimState.getData(), static_cast<int>(trimState.getSize()));
    auto oldState = juce::ValueTree::fromXml(*oldXml);
    oldState.setProperty("schemaVersion", 1, nullptr);
    for (int i = oldState.getNumChildren() - 1; i >= 0; i -= 1)
        if (oldState.getChild(i).getProperty("id").toString() == "outputTrim") oldState.removeChild(i, nullptr);
    juce::AudioProcessor::copyXmlToBinary(*oldState.createXml(), trimState);
    trimRestored.setStateInformation(trimState.getData(), static_cast<int>(trimState.getSize()));
    require(trimRestored.getParameters()[6]->getValue() == 0.5f, "legacy preset resets trim to zero dB");
    trimmed.prepareToPlay(44100, 64); auto trimUnsupported = input(257);
    trimmed.processBlock(trimUnsupported, midi); equal(trimUnsupported, bypassReference, "trim excluded from unsupported rate");
    EndlessProcessor ramped;
    set(ramped, "Mix", 0); ramped.prepareToPlay(48000, 127); set(ramped, "Output trim", 1);
    juce::AudioBuffer<float> constant(2, 1200);
    for (int ch = 0; ch < 2; ch += 1) for (int i = 0; i < 1200; i += 1) constant.setSample(ch, i, 0.1f);
    ramped.processBlock(constant, midi);
    require(constant.getSample(0, 0) > 0.1f && constant.getSample(0, 0) < 0.101f, "trim ramp starts without gain jump");
    require(std::abs(constant.getSample(0, 1199) - 0.1f * boost) < 1.0e-7f, "trim ramp reaches target");
    for (int i = 1; i < 1200; i += 1) {
        require(constant.getSample(0, i) >= constant.getSample(0, i-1), "trim ramp monotonic");
        require(constant.getSample(0, i) == constant.getSample(1, i), "same trim on both channels");
    }
    std::puts("PASS: processor parity, isolation, modes/state/reset, oversized callbacks, bypass and sample-rate guard");
    return EXIT_SUCCESS;
}
