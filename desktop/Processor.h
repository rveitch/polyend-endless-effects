#pragma once
#include "PatchFactory.h"
#include <juce_audio_utils/juce_audio_utils.h>
#include <array>
#include <atomic>
#include <vector>

class EndlessProcessor final : public juce::AudioProcessor
{
public:
    EndlessProcessor();
    void prepareToPlay(double sampleRate, int maximumBlockSize) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlockBypassed(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorParameter* getBypassParameter() const override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override;
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.1; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;
    bool supportedRate() const { return rateOk.load(std::memory_order_relaxed); }
private:
    void process(juce::AudioBuffer<float>&, bool bypass);
    endlessDesktop::PatchHandle patch;
    juce::AudioProcessorValueTreeState parameters;
    std::vector<float> workingBuffer;
    std::array<std::vector<float>, 2> dry;
    std::array<std::atomic<float>*, 3> knobs{};
    std::atomic<float>* baseParameter{};
    std::atomic<float>* combinedParameter{};
    std::atomic<float>* bypassParameter{};
    std::atomic<float>* trimParameter{};
    juce::SmoothedValue<float> outputGain;
    std::atomic<bool> rateOk{false};
    int baseMode = 0;
    bool combinedMode = false;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EndlessProcessor)
};
