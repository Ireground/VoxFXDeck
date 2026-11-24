#include "PluginEditor.h"

using namespace juce;

Font uiFont (float h, bool bold)
{
    auto o = FontOptions (h);
    return Font (bold ? o.withStyle ("Bold") : o);
}

static const char* const characterInfo[] = {
    "Monotone robot: your words sung on one note. Set the note with PITCH.",
    "Half human, half machine: ring modulator and a metallic ring.",
    "Wobbly, warped alien voice with a sci-fi ring mod and a short echo.",
    "Old radio / walkie-talkie: narrow, crunchy and lo-fi.",
    "Deep, growling monster with an extra voice an octave down.",
    "Demon: pitched down, an octave-down layer and a dark echo.",
    "Huge giant: an octave down with a big body.",
    "Small, nasty, nasal goblin.",
    "Breathy ghost whisper with a long haunted echo.",
    "Rotten, raspy, half-whispered zombie groan.",
    "The classic sped-up chipmunk voice.",
    "Raspy, nasal cartoon duck.",
    "High, wobbly, meow-ish cat.",
    "Gruff, growly dog.",
    "Brassy, trumpeting elephant call.",
    "Tiny buzzing insect.",
};

static Colour tagColour (const String& tag)
{
    if (tag == "MACHINE")  return Pal::blue;
    if (tag == "SCI-FI")   return Colour (0xff4fd1c5);
    if (tag == "CREATURE") return Pal::gold;
    if (tag == "SPOOKY")   return Colour (0xffa99cf0);
    return Colour (0xff86d28a); // ANIMAL
}

static String noteName (float hz)
{
    static const char* names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    const int midi = roundToInt (69.0f + 12.0f * std::log2 (jmax (1.0f, hz) / 440.0f));
    return String (names[((midi % 12) + 12) % 12]) + String (midi / 12 - 1);
}

//==============================================================================
DeckLook::DeckLook()
{
    setColour (PopupMenu::backgroundColourId, Pal::panel);
    setColour (PopupMenu::textColourId, Pal::text);
    setColour (PopupMenu::highlightedBackgroundColourId, Pal::blue.withAlpha (0.35f));
    setColour (PopupMenu::highlightedTextColourId, Colours::white);
    setColour (ToggleButton::textColourId, Pal::dim);
    setColour (TooltipWindow::backgroundColourId, Pal::panel);
    setColour (TooltipWindow::textColourId, Pal::text);
    setColour (TooltipWindow::outlineColourId, Pal::panelEdge);
}

void DeckLook::drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos, float start, float end, Slider& s)
{
    const bool big = (bool) s.getProperties()["big"];
    const bool bipolar = (bool) s.getProperties()["bipolar"];
    auto b = Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
    const float r = jmin (b.getWidth(), b.getHeight()) * 0.5f * 0.78f;
    const auto c = b.getCentre();
    const float angle = start + pos * (end - start);

    g.setColour (Pal::faint);
    const int ticks = big ? 21 : 11;
    for (int i = 0; i < ticks; ++i)
    {
        const float a = start + (float) i / (float) (ticks - 1) * (end - start);
        const auto p = c.getPointOnCircumference (r * 1.2f, a);
        g.fillEllipse (p.x - 1.2f * uiScale, p.y - 1.2f * uiScale, 2.4f * uiScale, 2.4f * uiScale);
    }

    {
        const float from = bipolar ? (start + end) * 0.5f : start;
        Path arc;
        arc.addCentredArc (c.x, c.y, r * 1.08f, r * 1.08f, 0.0f, jmin (from, angle), jmax (from, angle), true);
        g.setColour (Pal::blue.withAlpha (s.isMouseOverOrDragging() ? 1.0f : 0.75f));
        g.strokePath (arc, PathStrokeType (2.0f * uiScale, PathStrokeType::curved, PathStrokeType::rounded));
    }

    g.setColour (Colours::black.withAlpha (0.5f));
    g.fillEllipse (c.x - r, c.y - r + 2.0f * uiScale, r * 2, r * 2);
    ColourGradient grad (Pal::knobTop, c.x, c.y - r, Pal::knobBottom, c.x, c.y + r, false);
    g.setGradientFill (grad);
    g.fillEllipse (c.x - r, c.y - r, r * 2, r * 2);
    g.setColour (Colours::white.withAlpha (0.12f));
    g.drawEllipse (c.x - r + 0.5f, c.y - r + 0.5f, r * 2 - 1, r * 2 - 1, 1.0f);
    if (big)
    {
        const float ri = r * 0.82f;
        g.setColour (Colour (0xff131c26));
        g.fillEllipse (c.x - ri, c.y - ri, ri * 2, ri * 2);
        g.setColour (Pal::text);
        g.setFont (uiFont (r * 0.30f, true));
        g.drawText (s.getTextFromValue (s.getValue()), Rectangle<float> (c.x - ri, c.y - r * 0.2f, ri * 2, r * 0.4f), Justification::centred);
    }

    const auto p1 = c.getPointOnCircumference (big ? r * 0.86f : r * 0.3f, angle);
    const auto p2 = c.getPointOnCircumference (r * 0.95f, angle);
    g.setColour (Colours::white);
    g.drawLine ({ p1, p2 }, (big ? 3.0f : 2.4f) * uiScale);
}

void DeckLook::drawToggleButton (Graphics& g, ToggleButton& b, bool over, bool)
{
    auto r = b.getLocalBounds().toFloat();
    const float h = 14.0f * uiScale, w = 26.0f * uiScale;
    auto sw = Rectangle<float> (r.getX(), r.getCentreY() - h * 0.5f, w, h);
    const bool on = b.getToggleState();
    g.setColour (on ? Pal::blue.withAlpha (0.85f) : Pal::faint);
    g.fillRoundedRectangle (sw, h * 0.5f);
    const float d = h - 4.0f;
    g.setColour (Colours::white);
    g.fillEllipse (on ? sw.getRight() - d - 2.0f : sw.getX() + 2.0f, sw.getY() + 2.0f, d, d);
    g.setColour (over ? Pal::text : Pal::dim);
    g.setFont (uiFont (12.0f * uiScale, true));
    g.drawText (b.getButtonText().toUpperCase(), r.withTrimmedLeft (w + 8.0f * uiScale), Justification::centredLeft);
}

Font DeckLook::getPopupMenuFont() { return uiFont (14.0f * uiScale); }

//==============================================================================
Knob::Knob (VoxFXDeckProcessor& p, const String& paramId, const String& t, const String& l, const String& h, bool isBig)
    : title (t), lo (l), hi (h), big (isBig)
{
    slider.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
    slider.setRotaryParameters (MathConstants<float>::pi * 1.2f, MathConstants<float>::pi * 2.8f, true);
    slider.getProperties().set ("big", big);
    slider.setMouseDragSensitivity (big ? 260 : 200);
    addAndMakeVisible (slider);
    attachment = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (p.apvts, paramId, slider);
    if (auto* param = p.apvts.getParameter (paramId))
        slider.setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));
    slider.addListener (this);
    slider.addMouseListener (this, false);
}

Knob::~Knob() { slider.removeListener (this); }

void Knob::resized()
{
    auto b = getLocalBounds();
    const int labelH = jmax (14, getHeight() / 6);
    b.removeFromBottom (labelH);
    slider.setBounds (b);
}

void Knob::paint (Graphics& g)
{
    const int labelH = jmax (14, getHeight() / 6);
    auto lab = getLocalBounds().removeFromBottom (labelH).toFloat();
    const bool active = slider.isMouseOverOrDragging();
    const float fh = (float) labelH * 0.72f;

    g.setFont (uiFont (fh, true));
    if (active && ! big)
    {
        g.setColour (Pal::blue);
        g.drawText (slider.getTextFromValue (slider.getValue()), lab, Justification::centred);
    }
    else
    {
        g.setColour (Pal::text);
        g.drawText (title, lab, Justification::centred);
    }

    auto sb = slider.getBounds().toFloat();
    const float r = jmin (sb.getWidth(), sb.getHeight()) * 0.5f * 0.78f;
    const auto c = sb.getCentre();
    g.setColour (Pal::dim);
    g.setFont (uiFont (fh * 0.72f, true));
    const float yy = c.y + r * 1.0f;
    g.drawText (lo, Rectangle<float> (c.x - r * 2.2f, yy, r * 1.6f, fh), Justification::centredRight);
    g.drawText (hi, Rectangle<float> (c.x + r * 0.6f, yy, r * 1.6f, fh), Justification::centredLeft);
}

//==============================================================================
static void drawPanel (Graphics& g, Rectangle<float> r, float sc, const String& title)
{
    g.setColour (Pal::panel);
    g.fillRoundedRectangle (r, 8.0f * sc);
    g.setColour (Pal::panelEdge);
    g.drawRoundedRectangle (r.reduced (0.5f), 8.0f * sc, 1.0f);
    g.setColour (Pal::dim);
    g.setFont (uiFont (11.0f * sc, true));
    g.drawText (title, r.reduced (14 * sc, 0).withHeight (30 * sc), Justification::centredLeft);
}

// small icon per family
static void drawGlyph (Graphics& g, const String& tag, Rectangle<float> r, Colour col, float sc)
{
    const float s = jmin (r.getWidth(), r.getHeight());
    r = r.withSizeKeepingCentre (s, s);
    const auto c = r.getCentre();
    const float lw = jmax (1.2f, 1.6f * sc);
    g.setColour (col);

    if (tag == "MACHINE")       // robot head
    {
        auto head = Rectangle<float> (s * 0.64f, s * 0.5f).withCentre ({ c.x, c.y + s * 0.1f });
        g.drawRoundedRectangle (head, s * 0.08f, lw);
        g.drawLine (c.x, head.getY(), c.x, head.getY() - s * 0.16f, lw);
        g.fillEllipse (c.x - s * 0.05f, head.getY() - s * 0.24f, s * 0.1f, s * 0.1f);
        g.fillEllipse (head.getX() + s * 0.12f, head.getY() + s * 0.13f, s * 0.11f, s * 0.11f);
        g.fillEllipse (head.getRight() - s * 0.23f, head.getY() + s * 0.13f, s * 0.11f, s * 0.11f);
        g.drawLine (head.getX() + s * 0.16f, head.getBottom() - s * 0.12f, head.getRight() - s * 0.16f, head.getBottom() - s * 0.12f, lw);
    }
    else if (tag == "SCI-FI")   // saucer
    {
        g.drawEllipse (Rectangle<float> (s * 0.86f, s * 0.26f).withCentre ({ c.x, c.y + s * 0.06f }), lw);
        Path dome;
        dome.addCentredArc (c.x, c.y - s * 0.02f, s * 0.2f, s * 0.2f, 0.0f, -MathConstants<float>::halfPi, MathConstants<float>::halfPi, true);
        g.strokePath (dome, PathStrokeType (lw));
        for (int i = -1; i <= 1; ++i)
            g.fillEllipse (c.x + (float) i * s * 0.22f - s * 0.035f, c.y + s * 0.03f, s * 0.07f, s * 0.07f);
    }
    else if (tag == "CREATURE") // horned head
    {
        const float hr = s * 0.27f;
        g.drawEllipse (c.x - hr, c.y - hr + s * 0.1f, hr * 2, hr * 2, lw);
        Path horns;
        horns.startNewSubPath (c.x - hr * 0.75f, c.y - hr * 0.35f);
        horns.quadraticTo (c.x - hr * 1.3f, c.y - hr * 1.2f, c.x - hr * 0.9f, c.y - hr * 1.75f);
        horns.startNewSubPath (c.x + hr * 0.75f, c.y - hr * 0.35f);
        horns.quadraticTo (c.x + hr * 1.3f, c.y - hr * 1.2f, c.x + hr * 0.9f, c.y - hr * 1.75f);
        g.strokePath (horns, PathStrokeType (lw, PathStrokeType::curved, PathStrokeType::rounded));
        g.fillEllipse (c.x - hr * 0.5f, c.y + s * 0.04f, hr * 0.3f, hr * 0.3f);
        g.fillEllipse (c.x + hr * 0.2f, c.y + s * 0.04f, hr * 0.3f, hr * 0.3f);
    }
    else if (tag == "SPOOKY")   // ghost
    {
        const float w = s * 0.56f, top = c.y - s * 0.34f, bottom = c.y + s * 0.34f;
        Path p;
        p.startNewSubPath (c.x - w * 0.5f, bottom);
        p.lineTo (c.x - w * 0.5f, top + w * 0.5f);
        p.addCentredArc (c.x, top + w * 0.5f, w * 0.5f, w * 0.5f, 0.0f, -MathConstants<float>::halfPi, MathConstants<float>::halfPi);
        p.lineTo (c.x + w * 0.5f, bottom);
        for (int i = 0; i < 3; ++i)
        {
            const float x0 = c.x + w * 0.5f - (float) i * w / 3.0f;
            p.quadraticTo (x0 - w / 6.0f, bottom - s * 0.1f, x0 - w / 3.0f, bottom);
        }
        g.strokePath (p, PathStrokeType (lw, PathStrokeType::curved, PathStrokeType::rounded));
        g.fillEllipse (c.x - w * 0.24f, c.y - s * 0.08f, s * 0.08f, s * 0.1f);
        g.fillEllipse (c.x + w * 0.24f - s * 0.08f, c.y - s * 0.08f, s * 0.08f, s * 0.1f);
    }
    else                        // paw
    {
        g.fillEllipse (Rectangle<float> (s * 0.38f, s * 0.3f).withCentre ({ c.x, c.y + s * 0.16f }));
        g.fillEllipse (Rectangle<float> (s * 0.14f, s * 0.18f).withCentre ({ c.x - s * 0.24f, c.y - s * 0.06f }));
        g.fillEllipse (Rectangle<float> (s * 0.14f, s * 0.18f).withCentre ({ c.x - s * 0.08f, c.y - s * 0.2f }));
        g.fillEllipse (Rectangle<float> (s * 0.14f, s * 0.18f).withCentre ({ c.x + s * 0.08f, c.y - s * 0.2f }));
        g.fillEllipse (Rectangle<float> (s * 0.14f, s * 0.18f).withCentre ({ c.x + s * 0.24f, c.y - s * 0.06f }));
    }
}

//==============================================================================
Rectangle<float> CharacterGrid::tileBounds (int index) const
{
    const float sc = uiScale;
    auto r = getLocalBounds().toFloat().reduced (12 * sc, 0);
    r.removeFromTop (32 * sc);
    r.removeFromBottom (12 * sc);
    const int cols = 4, rows = (vox::numCharacters + cols - 1) / cols;
    const float gap = 8 * sc;
    const float w = (r.getWidth() - gap * (float) (cols - 1)) / (float) cols;
    const float h = (r.getHeight() - gap * (float) (rows - 1)) / (float) rows;
    const int col = index % cols, row = index / cols;
    return { r.getX() + (float) col * (w + gap), r.getY() + (float) row * (h + gap), w, h };
}

int CharacterGrid::tileAt (Point<float> p) const
{
    for (int i = 0; i < vox::numCharacters; ++i)
        if (tileBounds (i).contains (p)) return i;
    return -1;
}

void CharacterGrid::mouseMove (const MouseEvent& e)
{
    const int t = tileAt (e.position);
    if (t != hovered)
    {
        hovered = t;
        setTooltip (t >= 0 ? String (characterInfo[t]) : String());
        setMouseCursor (t >= 0 ? MouseCursor::PointingHandCursor : MouseCursor::NormalCursor);
        repaint();
    }
}

void CharacterGrid::mouseExit (const MouseEvent&)
{
    hovered = -1;
    repaint();
}

void CharacterGrid::mouseDown (const MouseEvent& e)
{
    const int t = tileAt (e.position);
    if (t >= 0)
    {
        proc.selectCharacter (t);
        update();
    }
}

void CharacterGrid::update()
{
    const int c = proc.getCharacter();
    if (c != shown) { shown = c; repaint(); }
}

void CharacterGrid::paint (Graphics& g)
{
    const float sc = uiScale;
    auto r = getLocalBounds().toFloat();
    drawPanel (g, r, sc, "CHARACTER");

    g.setColour (Pal::dim.withAlpha (0.8f));
    g.setFont (uiFont (10.0f * sc, true));
    g.drawText ("CLICK TO CHOOSE", r.reduced (14 * sc, 0).withHeight (30 * sc), Justification::centredRight);

    const int sel = proc.getCharacter();
    for (int i = 0; i < vox::numCharacters; ++i)
    {
        const auto& ch = vox::characters[i];
        const String tag (ch.tag);
        const Colour tc = tagColour (tag);
        auto t = tileBounds (i);
        const bool isSel = i == sel, isHover = i == hovered;
        const float rad = 6.0f * sc;

        g.setColour (isSel ? Pal::blue.withAlpha (0.16f) : (isHover ? Colour (0xff1a2633) : Colour (0xff121c27)));
        g.fillRoundedRectangle (t, rad);
        g.setColour (isSel ? Pal::blue : (isHover ? Pal::blue.withAlpha (0.5f) : Pal::panelEdge));
        g.drawRoundedRectangle (t.reduced (0.5f), rad, isSel ? 1.6f * sc : 1.0f);

        // family stripe
        g.setColour (tc.withAlpha (isSel ? 0.95f : 0.55f));
        g.fillRoundedRectangle (t.getX() + 6 * sc, t.getY() + 6 * sc, 3 * sc, t.getHeight() - 12 * sc, 1.5f * sc);

        auto inner = t.reduced (14 * sc, 6 * sc);
        auto icon = inner.removeFromLeft (jmin (inner.getHeight(), 32 * sc));
        drawGlyph (g, tag, icon.reduced (2 * sc), tc.withAlpha (isSel ? 1.0f : 0.7f), sc);
        inner.removeFromLeft (7 * sc);

        const float nameH = jmin (14.0f * sc, inner.getHeight() * 0.42f);
        auto nameR = inner.withHeight (inner.getHeight() * 0.58f);
        g.setColour (isSel ? Colours::white : Pal::text);
        g.setFont (uiFont (nameH, true));
        g.drawFittedText (String (ch.name).toUpperCase(), nameR.toNearestInt(), Justification::bottomLeft, 1, 0.8f);
        g.setColour (isSel ? tc : Pal::dim);
        g.setFont (uiFont (jmin (10.0f * sc, nameH * 0.7f), true));
        g.drawText (tag, inner.withTrimmedTop (inner.getHeight() * 0.6f), Justification::topLeft);
    }
}

//==============================================================================
void ShapePanel::update()
{
    auto& m = proc.meters;
    const int f = m.frames.load();
    if (f != lastFrames)
    {
        lastFrames = f;
        idleTicks = 0;
        float mx = -200.0f;
        for (int i = 0; i < vox::numEnvPoints; ++i)
        {
            inEnv[i] += 0.35f * (m.inEnv[(size_t) i].load() - inEnv[i]);
            outEnv[i] += 0.35f * (m.outEnv[(size_t) i].load() - outEnv[i]);
            mx = jmax (mx, inEnv[i], outEnv[i]);
        }
        top += 0.1f * ((mx + 6.0f) - top);
    }
    else ++idleTicks;

    inLvl = m.inDb.load();
    outLvl = m.outDb.load();
    repaint();
}

void ShapePanel::paint (Graphics& g)
{
    const float sc = uiScale;
    auto r = getLocalBounds().toFloat();
    drawPanel (g, r, sc, "VOICE SHAPE");

    // legend
    {
        auto lg = r.reduced (14 * sc, 0).withHeight (30 * sc);
        g.setFont (uiFont (10.0f * sc, true));
        auto item = [&] (const String& txt, Colour col)
        {
            const float tw = GlyphArrangement::getStringWidth (uiFont (10.0f * sc, true), txt);
            g.setColour (Pal::dim);
            g.drawText (txt, lg.removeFromRight (tw), Justification::centredRight);
            auto sw = lg.removeFromRight (16 * sc);
            g.setColour (col);
            g.fillRect (sw.getX() + 2 * sc, sw.getCentreY() - 1, 10 * sc, 2.0f);
            lg.removeFromRight (12 * sc);
        };
        item ("AFTER", Pal::blue);
        item ("YOUR VOICE", Pal::dim);
    }

    auto inner = r.reduced (14 * sc, 0);
    inner.removeFromTop (32 * sc);
    auto readouts = inner.removeFromBottom (150 * sc);
    auto plot = inner.withTrimmedBottom (18 * sc);

    g.setColour (Pal::badge);
    g.fillRoundedRectangle (plot, 4 * sc);

    const float range = 54.0f;
    auto xOf = [&] (float hz) { return plot.getX() + plot.getWidth() * std::log (hz / vox::envLoHz) / std::log (vox::envHiHz / vox::envLoHz); };
    auto yOf = [&] (float db) { return plot.getY() + plot.getHeight() * jlimit (0.0f, 1.0f, (top - db) / range); };

    // grid
    g.setFont (uiFont (9.5f * sc, true));
    for (float hz : { 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f })
    {
        const float x = xOf (hz);
        g.setColour (Pal::faint.withAlpha (0.45f));
        g.drawVerticalLine ((int) x, plot.getY(), plot.getBottom());
        g.setColour (Pal::dim);
        g.drawText (hz >= 1000 ? String ((int) (hz / 1000)) + "k" : String ((int) hz),
                    Rectangle<float> (x - 20 * sc, plot.getBottom() + 2 * sc, 40 * sc, 14 * sc), Justification::centred);
    }

    const bool waiting = idleTicks > 45;
    auto curve = [&] (const float* v, bool closed)
    {
        Path p;
        for (int i = 0; i < vox::numEnvPoints; ++i)
        {
            const float x = plot.getX() + plot.getWidth() * (float) i / (float) (vox::numEnvPoints - 1);
            const float y = yOf (v[i]);
            if (i == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
        }
        if (closed)
        {
            p.lineTo (plot.getRight(), plot.getBottom());
            p.lineTo (plot.getX(), plot.getBottom());
            p.closeSubPath();
        }
        return p;
    };

    {
        Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (plot.toNearestInt());
        const float alpha = waiting ? 0.3f : 1.0f;
        g.setColour (Pal::blue.withAlpha (0.15f * alpha));
        g.fillPath (curve (outEnv, true));
        g.setColour (Pal::dim.withAlpha (0.8f * alpha));
        g.strokePath (curve (inEnv, false), PathStrokeType (1.4f * sc, PathStrokeType::curved, PathStrokeType::rounded));
        g.setColour (Pal::blue.withAlpha (alpha));
        g.strokePath (curve (outEnv, false), PathStrokeType (2.0f * sc, PathStrokeType::curved, PathStrokeType::rounded));
    }
    if (waiting)
    {
        g.setColour (Pal::dim);
        g.setFont (uiFont (11.0f * sc, true));
        g.drawText ("WAITING FOR VOICE", plot, Justification::centred);
    }

    // readouts
    readouts.removeFromTop (10 * sc);
    auto& m = proc.meters;
    auto row = [&] (const String& label, const String& value, Colour col)
    {
        auto rr = readouts.removeFromTop (22 * sc);
        g.setColour (Pal::dim);
        g.setFont (uiFont (11.0f * sc, true));
        g.drawText (label, rr, Justification::centredLeft);
        g.setColour (col);
        g.setFont (uiFont (14.0f * sc, true));
        g.drawText (value, rr, Justification::centredRight);
    };
    auto st = [] (float v) { return (v > 0.04f ? "+" : "") + String (v, 1) + " st"; };
    const float synth = m.synth.load();
    const float pitchSt = m.pitchSt.load(), formantSt = m.formantSt.load();

    String source;
    const int ch = proc.getCharacter();
    const float noise = vox::characters[ch].noise;
    if (synth > 0.6f) source = noise > 0.5f ? "WHISPER" : "ROBOT  " + noteName (m.robotHz.load());
    else if (synth > 0.05f) source = noise > 0.5f ? "VOICE + BREATH" : "VOICE + ROBOT";
    else source = "YOUR VOICE";

    row ("SOUND SOURCE", source, synth > 0.6f ? Pal::gold : Pal::text);
    row ("PITCH", synth > 0.6f ? String ("-") : st (pitchSt), std::abs (pitchSt) > 0.05f ? Pal::blue : Pal::text);
    row ("FORMANT", st (formantSt), std::abs (formantSt) > 0.05f ? Pal::blue : Pal::text);
    row ("LATENCY", String (proc.getLatencyMs(), 0) + " ms (auto-compensated)", Pal::dim);

    // level meters
    readouts.removeFromTop (6 * sc);
    auto meter = [&] (const String& label, float db, Colour col)
    {
        auto rr = readouts.removeFromTop (18 * sc);
        g.setColour (Pal::dim);
        g.setFont (uiFont (10.0f * sc, true));
        g.drawText (label, rr.removeFromLeft (34 * sc), Justification::centredLeft);
        auto val = rr.removeFromRight (52 * sc);
        g.setColour (db > -0.1f ? Pal::gold : Pal::text);
        g.drawText (db < -90 ? String ("-inf") : String (db, 1), val, Justification::centredRight);
        auto bar = rr.withSizeKeepingCentre (rr.getWidth() - 8 * sc, 6 * sc);
        g.setColour (Pal::badge);
        g.fillRoundedRectangle (bar, 3 * sc);
        const float frac = jlimit (0.0f, 1.0f, (db + 60.0f) / 60.0f);
        g.setColour (db > -0.1f ? Pal::gold : col);
        g.fillRoundedRectangle (bar.withWidth (bar.getWidth() * frac), 3 * sc);
    };
    meter ("IN", inLvl, Pal::dim);
    meter ("OUT", outLvl, Pal::blue);
}

//==============================================================================
VoxFXDeckEditor::VoxFXDeckEditor (VoxFXDeckProcessor& p)
    : AudioProcessorEditor (p), proc (p),
      pitch (p, ParamIDs::pitch, "PITCH", "-12", "+12"),
      formant (p, ParamIDs::formant, "FORMANT", "LOW", "HIGH"),
      grit (p, ParamIDs::grit, "GRIT", "CLEAN", "DIRTY"),
      amount (p, ParamIDs::amount, "AMOUNT", "0%", "100%", true),
      mix (p, ParamIDs::mix, "MIX", "DRY", "WET"),
      output (p, ParamIDs::output, "OUTPUT", "-24", "+12"),
      grid (p), shape (p)
{
    setLookAndFeel (&look);
    for (auto* k : { &pitch, &formant, &grit, &amount, &mix, &output })
        addAndMakeVisible (k);
    pitch.slider.getProperties().set ("bipolar", true);
    formant.slider.getProperties().set ("bipolar", true);
    addAndMakeVisible (grid);
    addAndMakeVisible (shape);

    pitch.slider.setTooltip ("Moves the pitch up or down on top of the character, in semitones. For ROBOT this sets the note it sings.");
    formant.slider.setTooltip ("Changes the size of the 'throat': LOW = bigger, darker creature, HIGH = smaller, cartoonier.");
    grit.slider.setTooltip ("How rough it gets: scales the growl, distortion, crunch and ring modulation of the character.");
    amount.slider.setTooltip ("How far toward the character. 0% = your normal voice, 100% = the full effect.");
    mix.slider.setTooltip ("Blend of your original voice and the effect.");
    output.slider.setTooltip ("Output level.");
    fxOn.setTooltip ("Turn the effect on/off to compare.");
    autoLevel.setTooltip ("Keeps the effect as loud as your original voice, whatever character you pick.");

    for (auto [btn, id] : { std::pair<ToggleButton*, const String*> { &fxOn, &ParamIDs::fxOn }, { &autoLevel, &ParamIDs::autoLevel } })
    {
        addAndMakeVisible (btn);
        buttonAttachments.push_back (std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, *id, *btn));
    }

    // read the remembered size first: setResizeLimits() triggers resized(), which stores the size
    const int savedW = jlimit (780, 2400, proc.editorWidth), savedH = jlimit (500, 1540, proc.editorHeight);
    setResizable (true, true);
    setResizeLimits (780, 500, 2400, 1540);
    setSize (savedW, savedH);
    startTimerHz (30);
}

VoxFXDeckEditor::~VoxFXDeckEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void VoxFXDeckEditor::timerCallback()
{
    grid.update();
    shape.update();

    const auto& ch = vox::characters[proc.getCharacter()];
    String s = String (ch.name).toUpperCase() + "  |  " + ch.tag;
    bool warn = false;
    if (proc.getParam (ParamIDs::fxOn) < 0.5f) { s = "FX OFF"; warn = true; }
    if (s != status || warn != statusWarn) { status = s; statusWarn = warn; repaint (statusPill.toNearestInt().expanded (2)); }
}

void VoxFXDeckEditor::paint (Graphics& g)
{
    g.setGradientFill (ColourGradient (Pal::bgTop, 0, 0, Pal::bgBottom, 0, (float) getHeight(), false));
    g.fillAll();

    auto h = headerArea.toFloat().reduced (16 * sc, 0);
    const String a = "VoxFX", b = "Deck";
    const float aw = GlyphArrangement::getStringWidth (uiFont (22.0f * sc, true), a);
    g.setFont (uiFont (22.0f * sc, true));
    g.setColour (Pal::text);
    g.drawText (a, h, Justification::centredLeft);
    g.setColour (Pal::blue);
    g.drawText (b, h.withTrimmedLeft (aw), Justification::centredLeft);
    const float tx = aw + GlyphArrangement::getStringWidth (uiFont (22.0f * sc, true), b) + 14 * sc;
    g.setColour (Pal::dim);
    g.setFont (uiFont (11.0f * sc, true));
    g.drawText ("VOICE EFFECTS", h.withTrimmedLeft (tx), Justification::centredLeft);

    // status pill
    g.setColour (statusWarn ? Pal::gold : Pal::badge);
    g.fillRoundedRectangle (statusPill, statusPill.getHeight() * 0.5f);
    g.setColour (statusWarn ? Pal::gold : Pal::panelEdge);
    g.drawRoundedRectangle (statusPill.reduced (0.5f), statusPill.getHeight() * 0.5f, 1.0f);
    g.setColour (statusWarn ? Pal::badge : Pal::blue);
    g.setFont (uiFont (11.0f * sc, true));
    g.drawText (status, statusPill, Justification::centred);

    g.setColour (Colours::black.withAlpha (0.35f));
    g.drawHorizontalLine (headerArea.getBottom(), 0.0f, (float) getWidth());

    auto bb = bottomArea.toFloat().reduced (16 * sc, 0);
    g.setColour (Pal::dim.withAlpha (0.8f));
    g.setFont (uiFont (10.5f * sc, true));
    g.drawText ("DOUBLE-CLICK A KNOB TO RESET IT", bb, Justification::centredRight);
}

void VoxFXDeckEditor::resized()
{
    proc.editorWidth = getWidth();
    proc.editorHeight = getHeight();
    sc = jmin ((float) getWidth() / 1000.0f, (float) getHeight() / 640.0f);
    look.uiScale = sc;
    grid.uiScale = sc;
    shape.uiScale = sc;

    auto r = getLocalBounds();
    headerArea = r.removeFromTop ((int) (46 * sc));
    bottomArea = r.removeFromBottom ((int) (40 * sc));
    knobArea = r.removeFromBottom ((int) (132 * sc));
    middleArea = r.reduced ((int) (12 * sc), (int) (8 * sc));

    const float pw = 200 * sc, ph = 22 * sc;
    statusPill = { (float) headerArea.getRight() - 16 * sc - pw, (float) headerArea.getCentreY() - ph * 0.5f, pw, ph };

    auto m = middleArea;
    shape.setBounds (m.removeFromRight ((int) (330 * sc)));
    m.removeFromRight ((int) (10 * sc));
    grid.setBounds (m);

    // knob row: 3 | big | 2
    auto k = knobArea.reduced ((int) (20 * sc), (int) (4 * sc));
    const int bigW = (int) (150 * sc);
    const int small = (k.getWidth() - bigW) / 5;
    for (auto* kn : { &pitch, &formant, &grit })
        kn->setBounds (k.removeFromLeft (small).reduced ((int) (4 * sc), (int) (6 * sc)));
    amount.setBounds (k.removeFromLeft (bigW).withSizeKeepingCentre (bigW, k.getHeight() + (int) (6 * sc)));
    for (auto* kn : { &mix, &output })
        kn->setBounds (k.removeFromLeft (small).reduced ((int) (4 * sc), (int) (6 * sc)));

    auto b = bottomArea.reduced ((int) (16 * sc), (int) (8 * sc));
    for (auto* t : { &fxOn, &autoLevel })
    {
        t->setBounds (b.removeFromLeft ((int) (130 * sc)));
        b.removeFromLeft ((int) (6 * sc));
    }
}
