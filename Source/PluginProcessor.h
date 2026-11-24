#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "VoxEngine.h"

namespace ParamIDs
{
    static const juce::String character = "character", amount = "amount", pitch = "pitch", formant = "formant",
        grit = "grit", mix = "mix", output = "output", fxOn = "fxOn", autoLevel = "autoLevel";
}

class VoxFXDeckProcessor : public juce::AudioProcessor
{
public:
    VoxFXDeckProcessor();
    ~VoxFXDeckProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "VoxFXDeck"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    float getParam (const juce::String& id) const { return apvts.getRawParameterValue (id)->load(); }
    int getCharacter() const { return juce::jlimit (0, vox::numCharacters - 1, (int) getParam (ParamIDs::character)); }
    void selectCharacter (int index);
    double getLatencyMs() const { return currentSampleRate > 0 ? 1000.0 * getLatencySamples() / currentSampleRate : 0.0; }

    juce::AudioProcessorValueTreeState apvts;
    vox::Meters meters;

    // remembered editor size (saved with the project, survives closing/reopening the window)
    int editorWidth = 1000, editorHeight = 640;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

private:
    vox::Engine engine;
    double currentSampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VoxFXDeckProcessor)
};
