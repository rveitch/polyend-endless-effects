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
    std::puts("PASS: processor parity, isolation, modes/state/reset, oversized callbacks, bypass and sample-rate guard");
    return EXIT_SUCCESS;
}
