#include "Processor.h"
#include "EffectConfig.h"
#include <algorithm>
#include <cmath>
#include <set>

static juce::AudioProcessorValueTreeState::ParameterLayout makeLayout(Patch& patch)
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    for (int index = 0; index < 3; index += 1)
    {
        const auto metadata = patch.getParameterMetadata(index);
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"knob" + juce::String(index), 1},
            endlessDesktop::parameterNames[index], juce::NormalisableRange<float>{metadata.minValue, metadata.maxValue}, metadata.defaultValue));
    }
    layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"baseMode", 1}, "Base mode", juce::StringArray{"I", "II"}, 0));
    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"combined", 1}, "I+II", false));
    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"bypass", 1}, "Bypass", false));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"outputTrim", 1}, "Output trim",
        juce::NormalisableRange<float>{-6.0f, 6.0f}, 0.0f, juce::AudioParameterFloatAttributes{}.withLabel("dB")));
    return layout;
}
EndlessProcessor::EndlessProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      patch(endlessDesktop::createPatch()), parameters(*this, nullptr, "EndlessState", makeLayout(*patch))
{
    for (int index = 0; index < 3; index += 1) knobs[index] = parameters.getRawParameterValue("knob" + juce::String(index));
    baseParameter = parameters.getRawParameterValue("baseMode");
    combinedParameter = parameters.getRawParameterValue("combined");
    bypassParameter = parameters.getRawParameterValue("bypass");
    trimParameter = parameters.getRawParameterValue("outputTrim");
    parameters.state.setProperty("schemaVersion", 2, nullptr);
    parameters.state.setProperty("effect", endlessDesktop::effectName, nullptr);
}
void EndlessProcessor::prepareToPlay(double sampleRate, int maximumBlockSize)
{
    rateOk.store(sampleRate == Patch::kSampleRate, std::memory_order_relaxed);
    workingBuffer.assign(Patch::kWorkingBufferSize, 0.0f);
    for (auto& channel : dry) channel.resize(static_cast<std::size_t>(std::max(1, maximumBlockSize)));
    patch->setWorkingBuffer(std::span<float, Patch::kWorkingBufferSize>{workingBuffer.data(), workingBuffer.size()});
    patch->init(); baseMode = 0; combinedMode = false;
    outputGain.reset(Patch::kSampleRate, 0.02);
    outputGain.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(trimParameter->load(std::memory_order_relaxed)));
}
bool EndlessProcessor::isBusesLayoutSupported(const BusesLayout& layout) const
{
    return layout.getMainInputChannelSet() == juce::AudioChannelSet::stereo() &&
           layout.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}
void EndlessProcessor::processBlock(juce::AudioBuffer<float>& audio, juce::MidiBuffer&)
{
    process(audio, bypassParameter->load(std::memory_order_relaxed) >= 0.5f);
}
void EndlessProcessor::processBlockBypassed(juce::AudioBuffer<float>& audio, juce::MidiBuffer&)
{
    process(audio, true);
}
void EndlessProcessor::process(juce::AudioBuffer<float>& audio, bool bypass)
{
    juce::ScopedNoDenormals noDenormals;
    if (!supportedRate() || audio.getNumChannels() != 2 || dry[0].empty()) return;
    for (int index = 0; index < 3; index += 1) patch->setParamValue(index, knobs[index]->load(std::memory_order_relaxed));
    const int requestedBase = baseParameter->load(std::memory_order_relaxed) >= 0.5f ? 1 : 0;
    const bool requestedCombined = combinedParameter->load(std::memory_order_relaxed) >= 0.5f;
    if (requestedBase != baseMode) { patch->handleAction(0); baseMode = requestedBase; combinedMode = false; }
    if (requestedCombined != combinedMode) { patch->handleAction(1); combinedMode = requestedCombined; }
    outputGain.setTargetValue(juce::Decibels::decibelsToGain(trimParameter->load(std::memory_order_relaxed)));
    for (int offset = 0; offset < audio.getNumSamples();)
    {
        const int count = std::min(static_cast<int>(dry[0].size()), audio.getNumSamples() - offset);
        float* const left = audio.getWritePointer(0, offset);
        float* const right = audio.getWritePointer(1, offset);
        if (bypass) { std::copy_n(left, count, dry[0].data()); std::copy_n(right, count, dry[1].data()); }
        patch->processAudio({left, static_cast<std::size_t>(count)}, {right, static_cast<std::size_t>(count)});
        for (int sample = 0; sample < count; sample += 1) {
            const float gain = outputGain.getNextValue();
            if (!bypass) { left[sample] *= gain; right[sample] *= gain; }
        }
        if (bypass) { std::copy_n(dry[0].data(), count, left); std::copy_n(dry[1].data(), count, right); }
        offset += count;
    }
}
juce::AudioProcessorParameter* EndlessProcessor::getBypassParameter() const { return parameters.getParameter("bypass"); }
const juce::String EndlessProcessor::getName() const { return endlessDesktop::effectName; }
void EndlessProcessor::getStateInformation(juce::MemoryBlock& destination)
{
    const auto state = parameters.copyState();
    if (auto xml = state.createXml()) copyXmlToBinary(*xml, destination);
}
void EndlessProcessor::setStateInformation(const void* data, int size)
{
    if (size <= 0 || size > 1024 * 1024 || data == nullptr) return;
    const auto xml = getXmlFromBinary(data, size);
    if (!xml) return;
    auto state = juce::ValueTree::fromXml(*xml);
    const int version = static_cast<int>(state.getProperty("schemaVersion", 0));
    if (!state.hasType("EndlessState") || (version != 1 && version != 2)) return;
    const int expectedChildren = getParameters().size() - (version == 1 ? 1 : 0);
    if (state.getProperty("effect").toString() != endlessDesktop::effectName || state.getNumChildren() != expectedChildren) return;
    std::set<juce::String> ids;
    for (const auto& child : state)
    {
        const auto id = child.getProperty("id").toString();
        if (version == 1 && id == "outputTrim") return;
        const auto* parameter = parameters.getParameter(id);
        const float value = static_cast<float>(child.getProperty("value"));
        if (!child.hasType("PARAM") || !child.hasProperty("value") || parameter == nullptr || !std::isfinite(value) || !ids.insert(id).second) return;
        const auto range = parameter->getNormalisableRange();
        if (value < range.start || value > range.end || (range.interval > 0 && range.snapToLegalValue(value) != value)) return;
    }
    if (version == 1) {
        juce::ValueTree trim("PARAM");
        trim.setProperty("id", "outputTrim", nullptr);
        trim.setProperty("value", 0.0f, nullptr);
        state.appendChild(trim, nullptr);
        state.setProperty("schemaVersion", 2, nullptr);
    }
    parameters.replaceState(state);
}
class EndlessEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit EndlessEditor(EndlessProcessor& owner) : AudioProcessorEditor(owner), processor(owner), controls(owner)
    {
        addAndMakeVisible(controls); addAndMakeVisible(status);
        status.setJustificationType(juce::Justification::centred);
        setSize(controls.getWidth(), controls.getHeight() + 36);
        startTimerHz(5); timerCallback();
    }
    void resized() override { status.setBounds(0, 0, getWidth(), 36); controls.setBounds(0, 36, getWidth(), getHeight() - 36); }
private:
    void timerCallback() override
    {
        status.setText(processor.supportedRate() ? "Stereo 48 kHz audition" : "Set the project to 48 kHz. Audio is passing through.", juce::dontSendNotification);
    }
    EndlessProcessor& processor;
    juce::GenericAudioProcessorEditor controls;
    juce::Label status;
};
juce::AudioProcessorEditor* EndlessProcessor::createEditor() { return new EndlessEditor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new EndlessProcessor; }
