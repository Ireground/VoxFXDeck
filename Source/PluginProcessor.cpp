#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;

static String pct (float v, int) { return String (roundToInt (v * 100.0f)) + "%"; }
static String semis (float v, int) { return (v > 0.04f ? "+" : "") + String (v, 1) + " st"; }

AudioProcessorValueTreeState::ParameterLayout VoxFXDeckProcessor::createLayout()
{
    std::vector<std::unique_ptr<RangedAudioParameter>> p;
    using F = AudioParameterFloat;
    using A = AudioParameterFloatAttributes;

    StringArray names;
    for (auto& c : vox::characters) names.add (c.name);
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::character, 1 }, "Character", names, 0));

    p.push_back (std::make_unique<F> (ParameterID { ParamIDs::amount, 1 }, "Amount",
        NormalisableRange<float> (0.0f, 1.0f, 0.01f), 1.0f, A().withStringFromValueFunction (pct)));

    p.push_back (std::make_unique<F> (ParameterID { ParamIDs::pitch, 1 }, "Pitch",
        NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f, A().withStringFromValueFunction (semis)));

    p.push_back (std::make_unique<F> (ParameterID { ParamIDs::formant, 1 }, "Formant",
        NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f, A().withStringFromValueFunction (semis)));

    p.push_back (std::make_unique<F> (ParameterID { ParamIDs::grit, 1 }, "Grit",
        NormalisableRange<float> (0.0f, 2.0f, 0.01f), 1.0f, A().withStringFromValueFunction (pct)));

    p.push_back (std::make_unique<F> (ParameterID { ParamIDs::mix, 1 }, "Mix",
        NormalisableRange<float> (0.0f, 1.0f, 0.01f), 1.0f, A().withStringFromValueFunction (pct)));

    p.push_back (std::make_unique<F> (ParameterID { ParamIDs::output, 1 }, "Output",
        NormalisableRange<float> (-24.0f, 12.0f, 0.1f), 0.0f,
        A().withStringFromValueFunction ([] (float v, int) { return (v > 0.04f ? "+" : "") + String (v, 1) + " dB"; })));

    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { ParamIDs::fxOn, 1 }, "FX On", true));
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { ParamIDs::autoLevel, 1 }, "Auto Level", true));

    return { p.begin(), p.end() };
}

VoxFXDeckProcessor::VoxFXDeckProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "VoxFXDeck", createLayout())
{
}

void VoxFXDeckProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    engine.prepare (sampleRate, samplesPerBlock);
    setLatencySamples (engine.getLatency());
}

bool VoxFXDeckProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in = layouts.getMainInputChannelSet(), out = layouts.getMainOutputChannelSet();
    if (out != AudioChannelSet::mono() && out != AudioChannelSet::stereo()) return false;
    return in == out;
}

void VoxFXDeckProcessor::selectCharacter (int index)
{
    if (auto* prm = apvts.getParameter (ParamIDs::character))
    {
        prm->beginChangeGesture();
        prm->setValueNotifyingHost (prm->convertTo0to1 ((float) jlimit (0, vox::numCharacters - 1, index)));
        prm->endChangeGesture();
    }
}

void VoxFXDeckProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer&)
{
    ScopedNoDenormals noDenormals;
    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());
    if (buffer.getNumChannels() < 1) return;

    vox::Settings st;
    st.character = getCharacter();
    st.amount    = getParam (ParamIDs::amount);
    st.pitch     = getParam (ParamIDs::pitch);
    st.formant   = getParam (ParamIDs::formant);
    st.grit      = getParam (ParamIDs::grit);
    st.mix       = getParam (ParamIDs::mix);
    st.outputDb  = getParam (ParamIDs::output);
    st.fxOn      = getParam (ParamIDs::fxOn) > 0.5f;
    st.autoLevel = getParam (ParamIDs::autoLevel) > 0.5f;

    float* r = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;
    engine.process (buffer.getWritePointer (0), r, buffer.getNumSamples(), st, meters);
}

AudioProcessorEditor* VoxFXDeckProcessor::createEditor() { return new VoxFXDeckEditor (*this); }

void VoxFXDeckProcessor::getStateInformation (MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("editorW", editorWidth, nullptr);
    state.setProperty ("editorH", editorHeight, nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void VoxFXDeckProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto state = ValueTree::fromXml (*xml);
            editorWidth = (int) state.getProperty ("editorW", editorWidth);
            editorHeight = (int) state.getProperty ("editorH", editorHeight);
            apvts.replaceState (state);
        }
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new VoxFXDeckProcessor(); }
