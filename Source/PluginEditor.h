#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

// same colour scheme as StageDeck
namespace Pal
{
    const juce::Colour bgTop { 0xff22303f }, bgBottom { 0xff111a23 }, panel { 0xff0e161f }, panelEdge { 0xff2a3a4b },
        text { 0xffe9eef3 }, dim { 0xff8394a6 }, faint { 0xff3b4d60 }, blue { 0xff5ab4e8 }, gold { 0xfff2c14e },
        knobTop { 0xff3c4c5e }, knobBottom { 0xff17212b }, badge { 0xff0d151d };
}

juce::Font uiFont (float h, bool bold = false);

//==============================================================================
class DeckLook : public juce::LookAndFeel_V4
{
public:
    DeckLook();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float start, float end, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool over, bool down) override;
    juce::Font getPopupMenuFont() override;
    float uiScale = 1.0f;
};

//==============================================================================
class Knob : public juce::Component, private juce::Slider::Listener
{
public:
    Knob (VoxFXDeckProcessor&, const juce::String& paramId, const juce::String& title,
          const juce::String& lo, const juce::String& hi, bool big = false);
    ~Knob() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseEnter (const juce::MouseEvent&) override { repaint(); }
    void mouseExit (const juce::MouseEvent&) override { repaint(); }
    void mouseUp (const juce::MouseEvent&) override { repaint(); }

    juce::Slider slider;
private:
    void sliderValueChanged (juce::Slider*) override { repaint(); }
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    juce::String title, lo, hi;
    bool big;
};

//==============================================================================
class CharacterGrid : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit CharacterGrid (VoxFXDeckProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void update();
    float uiScale = 1.0f;
private:
    juce::Rectangle<float> tileBounds (int index) const;
    int tileAt (juce::Point<float>) const;
    VoxFXDeckProcessor& proc;
    int hovered = -1, shown = -1;
};

//==============================================================================
class ShapePanel : public juce::Component
{
public:
    explicit ShapePanel (VoxFXDeckProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
    void update();
    float uiScale = 1.0f;
private:
    VoxFXDeckProcessor& proc;
    float inEnv[vox::numEnvPoints] {}, outEnv[vox::numEnvPoints] {};
    float top = -20.0f, inLvl = -100, outLvl = -100;
    int lastFrames = -1, idleTicks = 1000;
};

//==============================================================================
class VoxFXDeckEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit VoxFXDeckEditor (VoxFXDeckProcessor&);
    ~VoxFXDeckEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    VoxFXDeckProcessor& proc;
    DeckLook look;
    juce::TooltipWindow tooltips { this, 700 };

    Knob pitch, formant, grit, amount, mix, output;
    CharacterGrid grid;
    ShapePanel shape;
    juce::ToggleButton fxOn { "FX On" }, autoLevel { "Auto level" }, midiOn { "MIDI keys" };
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> buttonAttachments;

    juce::Rectangle<int> headerArea, knobArea, middleArea, bottomArea;
    juce::Rectangle<float> statusPill;
    juce::String status;
    bool statusWarn = false;
    int lastNoteOns = 0, midiSeenTicks = 0;
    float sc = 1.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VoxFXDeckEditor)
};
