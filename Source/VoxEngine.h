#pragma once
// VoxFXDeck voice engine (plain C++, no JUCE so it can be tested on its own).
//
// Signal flow (mono, the voice is summed to mono first):
//   1. STFT analysis (Hann, 4x overlap)
//   2. spectral envelope (true-envelope via cepstrum) = the "formants" of the voice
//   3. excitation = spectrum / envelope  -> pitch shifted with a phase vocoder
//      (optional second "layer" voice, e.g. an octave down for demons)
//   4. or: a synthetic carrier (buzz / whisper) instead of the voice's own excitation (robot, ghost)
//   5. envelope stretched up/down = formant shift, re-applied to the excitation
//   6. time-domain colour: growl, ring mod, metal comb, drive, bit crush, filters, nasal peak, echo
//   7. auto level + dry/wet mix (dry is delayed by the STFT latency so they stay in phase)

#include <vector>
#include <complex>
#include <cmath>
#include <atomic>
#include <array>
#include <algorithm>
#include <cstdint>

namespace vox
{
constexpr float pi = 3.14159265358979f;
constexpr float twoPi = 6.28318530717959f;
constexpr int numEnvPoints = 96;
constexpr float envLoHz = 70.0f, envHiHz = 12000.0f;

//==============================================================================
struct Character
{
    const char* name;
    const char* tag;
    float pitch, formant;          // semitones
    float layerSemis, layerLevel;  // second voice, relative to the main pitch
    float synth, noise;            // synth = how much the voice's own buzz is replaced by a synthetic one; noise = whisper part of it
    float vibRate, vibDepth;       // Hz, semitones
    float ringHz, ringMix;
    float growlHz, growl;
    float drive, crush;
    float hpHz, lpHz;
    float peakHz, peakDb;          // nasal / body resonance
    float combHz, combFb;          // metallic resonance
    float echoMs, echoFb, echoMix;
};

//                 name        tag          pitch form  layer lvl   synth noise vibHz vibSt ringHz ring  grHz  growl drive crush  hp     lp      pkHz   pkDb combHz fb    echoMs fb    mix
inline const Character characters[] = {
    { "Robot",    "MACHINE",    0,    0,    0,   0,    1,    0.06f, 0,  0,     0,    0,    0,    0,    0.15f, 0.10f, 120,  9000,   1200,  2,   0,    0,     0,    0,    0    },
    { "Cyborg",   "MACHINE",    0,   -1,    0,   0,    0,    0,     0,  0,     60,   0.55f,0,    0,    0.25f, 0.05f, 150,  8000,   2000,  3,   180,  0.55f, 0,    0,    0    },
    { "Alien",    "SCI-FI",     2,   -5,    0,   0,    0,    0,     7,  0.6f,  420,  0.30f,0,    0,    0.10f, 0,     100,  10000,  0,     0,   0,    0,     90,   0.30f,0.20f},
    { "Radio",    "SCI-FI",     0,    0,    0,   0,    0,    0,     0,  0,     0,    0,    0,    0,    0.50f, 0.35f, 450,  3200,   1500,  4,   0,    0,     0,    0,    0    },
    { "Monster",  "CREATURE",  -7,   -5,  -12,  0.35f, 0,    0,     0,  0,     0,    0,    45,   0.60f,0.50f, 0,     40,   7000,   250,   3,   0,    0,     0,    0,    0    },
    { "Demon",    "CREATURE",  -5,   -3,  -12,  0.70f, 0,    0,     0,  0,     0,    0,    30,   0.35f,0.45f, 0,     30,   9000,   300,   4,   0,    0,     70,   0.40f,0.20f},
    { "Giant",    "CREATURE", -12,   -6,    0,   0,    0,    0,     0,  0,     0,    0,    0,    0,    0.10f, 0,     30,   9000,   150,   3,   0,    0,     120,  0.25f,0.15f},
    { "Goblin",   "CREATURE",   4,    5,    0,   0,    0,    0,     0,  0,     0,    0,    55,   0.25f,0.30f, 0,     120,  10000,  1800,  6,   0,    0,     0,    0,    0    },
    { "Ghost",    "SPOOKY",     2,    1,    0,   0,    1,    1,     4,  0.2f,  0,    0,    0,    0,    0,     0,     250,  7000,   0,     0,   0,    0,     260,  0.55f,0.45f},
    { "Zombie",   "SPOOKY",    -4,   -2,    0,   0,    0.35f,1,     0,  0,     0,    0,    22,   0.70f,0.40f, 0,     60,   5000,   0,     0,   0,    0,     0,    0,    0    },
    { "Chipmunk", "ANIMAL",    12,    7,    0,   0,    0,    0,     0,  0,     0,    0,    0,    0,    0,     0,     100,  16000,  0,     0,   0,    0,     0,    0,    0    },
    { "Duck",     "ANIMAL",     5,    9,    0,   0,    0,    0,     0,  0,     0,    0,    90,   0.45f,0.45f, 0,     400,  10000,  1600,  9,   0,    0,     0,    0,    0    },
    { "Cat",      "ANIMAL",    12,    5,    0,   0,    0,    0,     6,  0.35f, 0,    0,    0,    0,    0.10f, 0,     300,  12000,  2500,  5,   0,    0,     0,    0,    0    },
    { "Dog",      "ANIMAL",    -2,   -3,    0,   0,    0,    0,     0,  0,     0,    0,    35,   0.55f,0.55f, 0,     120,  6000,   700,   5,   0,    0,     0,    0,    0    },
    { "Elephant", "ANIMAL",     7,   -3,   -5,  0.40f, 0,    0,     5,  0.3f,  0,    0,    0,    0,    0.70f, 0,     200,  6000,   1100,  8,   0,    0,     0,    0,    0    },
    { "Insect",   "ANIMAL",    12,   12,    0,   0,    0,    0,     12, 0.4f,  220,  0.60f,0,    0,    0.10f, 0,     800,  14000,  0,     0,   0,    0,     0,    0,    0    },
};
constexpr int numCharacters = (int) (sizeof (characters) / sizeof (characters[0]));

//==============================================================================
struct Settings
{
    int character = 0;
    float amount = 1.0f;      // 0..1  how far toward the character
    float pitch = 0.0f;       // semitones added to the character's pitch (robot: the note it sings)
    float formant = 0.0f;     // semitones added to the character's formant shift
    float grit = 1.0f;        // 0..2  scales growl / drive / crush / ring
    float mix = 1.0f;         // 0..1
    float outputDb = 0.0f;
    bool fxOn = true;
    bool autoLevel = true;
};

struct Meters
{
    std::atomic<float> inDb { -100.0f }, outDb { -100.0f };
    std::array<std::atomic<float>, numEnvPoints> inEnv {}, outEnv {};   // dB, relative
    std::atomic<float> pitchSt { 0.0f }, formantSt { 0.0f }, robotHz { 0.0f }, synth { 0.0f };
    std::atomic<int> frames { 0 };
};

//==============================================================================
class FFT
{
public:
    void init (int size)
    {
        n = size;
        tw.resize ((size_t) n / 2);
        for (int k = 0; k < n / 2; ++k)
            tw[(size_t) k] = std::polar (1.0f, -twoPi * (float) k / (float) n);
        rev.resize ((size_t) n);
        int bits = 0;
        while ((1 << bits) < n) ++bits;
        for (int i = 0; i < n; ++i)
        {
            int r = 0;
            for (int b = 0; b < bits; ++b)
                if (i & (1 << b)) r |= 1 << (bits - 1 - b);
            rev[(size_t) i] = r;
        }
    }

    // unnormalised in both directions
    void run (std::complex<float>* x, bool inverse) const
    {
        for (int i = 0; i < n; ++i)
            if (i < rev[(size_t) i]) std::swap (x[i], x[rev[(size_t) i]]);
        for (int len = 2; len <= n; len <<= 1)
        {
            const int half = len >> 1, step = n / len;
            for (int i = 0; i < n; i += len)
                for (int j = 0; j < half; ++j)
                {
                    auto w = tw[(size_t) (j * step)];
                    if (inverse) w = std::conj (w);
                    const auto u = x[i + j], v = x[i + j + half] * w;
                    x[i + j] = u + v;
                    x[i + j + half] = u - v;
                }
        }
    }

private:
    int n = 0;
    std::vector<std::complex<float>> tw;
    std::vector<int> rev;
};

//==============================================================================
struct Biquad
{
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
    float process (float x)
    {
        const float y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }
    void reset() { z1 = z2 = 0; }
    void set (float nb0, float nb1, float nb2, float a0, float na1, float na2)
    {
        b0 = nb0 / a0; b1 = nb1 / a0; b2 = nb2 / a0; a1 = na1 / a0; a2 = na2 / a0;
    }
    void highpass (float sr, float f, float q = 0.7071f)
    {
        const float w = twoPi * std::clamp (f, 10.0f, sr * 0.45f) / sr, c = std::cos (w), al = std::sin (w) / (2 * q);
        set ((1 + c) * 0.5f, -(1 + c), (1 + c) * 0.5f, 1 + al, -2 * c, 1 - al);
    }
    void lowpass (float sr, float f, float q = 0.7071f)
    {
        const float w = twoPi * std::clamp (f, 20.0f, sr * 0.45f) / sr, c = std::cos (w), al = std::sin (w) / (2 * q);
        set ((1 - c) * 0.5f, 1 - c, (1 - c) * 0.5f, 1 + al, -2 * c, 1 - al);
    }
    void peak (float sr, float f, float db, float q)
    {
        const float A = std::pow (10.0f, db / 40.0f);
        const float w = twoPi * std::clamp (f, 20.0f, sr * 0.45f) / sr, c = std::cos (w), al = std::sin (w) / (2 * q);
        set (1 + al * A, -2 * c, 1 - al * A, 1 + al / A, -2 * c, 1 - al / A);
    }
};

//==============================================================================
class Engine
{
public:
    // the values that actually drive the sound (smoothed toward the chosen character)
    struct Live
    {
        float pitch = 0, formant = 0, layerSemis = 0, layerLevel = 0, synth = 0, noise = 0,
              vibRate = 0, vibDepth = 0, ringHz = 100, ringMix = 0, growlHz = 40, growl = 0,
              drive = 0, crush = 0, hpHz = 20, lpHz = 20000, peakHz = 1000, peakDb = 0,
              combHz = 200, combFb = 0, echoMs = 100, echoFb = 0, echoMix = 0, robotHz = 130.8f;
    };

    void prepare (double sampleRate, int /*maxBlock*/)
    {
        sr = (float) sampleRate;
        N = sr <= 50000.0f ? 2048 : (sr <= 100000.0f ? 4096 : 8192);
        half = N / 2;
        hop = N / osamp;
        latency = N - hop;      // fifo bookkeeping
        fullLatency = N;        // what the host sees: wet output lags the input by one full frame
        fft.init (N);

        win.assign ((size_t) N, 0.0f);
        double sumW2 = 0;
        for (int i = 0; i < N; ++i)
        {
            win[(size_t) i] = 0.5f - 0.5f * std::cos (twoPi * (float) i / (float) N);
            sumW2 += (double) win[(size_t) i] * win[(size_t) i];
        }
        olaScale = (float) (1.0 / ((double) N * (sumW2 / hop)));
        rmsRef = (float) ((double) N * sumW2 * 0.5);   // half-spectrum energy of a unit-RMS signal

        auto z = [] (std::vector<float>& v, int n) { v.assign ((size_t) n, 0.0f); };
        z (inFifo, N); z (outFifo, N); z (outAccum, 2 * N);
        z (mag, half + 1); z (freq, half + 1); z (lastPhase, half + 1);
        z (logMag, half + 1); z (envLog, half + 1); z (work, half + 1); z (newEnv, half + 1);
        z (synMag, half + 1); z (synFreq, half + 1); z (synMag2, half + 1); z (synFreq2, half + 1);
        z (sumPhase, half + 1); z (sumPhase2, half + 1); z (carPhase, half + 1);
        z (carMag, half + 1); z (carFreq, half + 1);
        buf.assign ((size_t) N, {});
        cep.assign ((size_t) N, {});
        outSpec.assign ((size_t) half + 1, {});
        tmpSpec.assign ((size_t) half + 1, {});

        lifter = std::clamp ((int) std::lround (sr * 0.0011f), 20, 200);
        rover = latency;

        // envelope display bins
        for (int p = 0; p < numEnvPoints; ++p)
        {
            const float f = envLoHz * std::pow (envHiHz / envLoHz, (float) p / (float) (numEnvPoints - 1));
            envBin[(size_t) p] = f / (sr / (float) N);
        }

        dryDelay = fullLatency;
        dryL.assign ((size_t) dryDelay, 0.0f);
        dryR.assign ((size_t) dryDelay, 0.0f);
        dryPos = 0;

        combBuf.assign ((size_t) (sr * 0.06f) + 4, 0.0f);
        echoBuf.assign ((size_t) (sr * 1.1f) + 4, 0.0f);
        combPos = echoPos = 0;
        hp.reset(); hp2.reset(); lp.reset(); lp2.reset(); pk.reset();

        live = Live();
        first = true;
        vibPhase = growlPhase = ringPhase = 0;
        holdCount = 0; holdValue = 0;
        inPow = wetPow = 0; autoGain = 1; fxMix = 1; outGain = 1;
    }

    int getLatency() const { return fullLatency; }

    void process (float* L, float* R, int n, const Settings& s, Meters& m)
    {
        updateLive (s, n);
        updateFilters();

        const float blockA = 1.0f - std::exp (-1.0f / (0.3f * sr));
        const float fxTarget = s.fxOn ? 1.0f : 0.0f;
        const float outTarget = std::pow (10.0f, s.outputDb / 20.0f);
        const float smoothA = 1.0f - std::exp (-1.0f / (0.02f * sr));
        float inPeak = 0, outPeak = 0;

        for (int i = 0; i < n; ++i)
        {
            const float xl = L[i], xr = R != nullptr ? R[i] : xl;
            const float mono = 0.5f * (xl + xr);

            // ---- spectral stage (STFT with latency N - hop)
            inFifo[(size_t) rover] = mono;
            float wet = outFifo[(size_t) (rover - latency)];
            if (++rover >= N)
            {
                rover = latency;
                processFrame (m);
            }

            // ---- time-domain colour
            wet = colour (wet);

            // ---- dry, delayed to line up with the wet signal
            const float dl = dryL[(size_t) dryPos], dr = dryR[(size_t) dryPos];
            dryL[(size_t) dryPos] = xl;
            dryR[(size_t) dryPos] = xr;
            if (++dryPos >= dryDelay) dryPos = 0;

            // ---- auto level: wet as loud as the input
            inPow += blockA * (mono * mono - inPow);
            wetPow += blockA * (wet * wet - wetPow);
            if (s.autoLevel)
            {
                if (inPow > 1.0e-9f && wetPow > 1.0e-12f)
                {
                    const float want = std::clamp (std::sqrt (inPow / wetPow), 0.125f, 8.0f);
                    autoGain += 0.0005f * (want - autoGain);
                }
            }
            else autoGain += 0.002f * (1.0f - autoGain);
            wet *= autoGain;

            fxMix += smoothA * (fxTarget - fxMix);
            outGain += smoothA * (outTarget - outGain);
            const float mix = s.mix * fxMix;

            float ol = (dl * (1.0f - mix) + wet * mix) * outGain;
            float orr = (dr * (1.0f - mix) + wet * mix) * outGain;
            if (! std::isfinite (ol) || ! std::isfinite (orr)) { ol = orr = 0.0f; resetState(); }

            L[i] = ol;
            if (R != nullptr) R[i] = orr;
            inPeak = std::max (inPeak, std::abs (mono));
            outPeak = std::max (outPeak, std::max (std::abs (ol), std::abs (orr)));
        }

        auto toDb = [] (float v) { return v > 1.0e-5f ? 20.0f * std::log10 (v) : -100.0f; };
        auto fall = [] (std::atomic<float>& a, float v) { a.store (std::max (v, a.load() - 1.5f)); };
        fall (m.inDb, toDb (inPeak));
        fall (m.outDb, toDb (outPeak));
        m.pitchSt.store (live.pitch);
        m.formantSt.store (live.formant);
        m.robotHz.store (live.robotHz);
        m.synth.store (live.synth);
    }

    const Live& getLive() const { return live; }

private:
    //==========================================================================
    static float lerp (float a, float b, float t) { return a + (b - a) * t; }

    Live targetFor (const Settings& s) const
    {
        const auto& c = characters[std::clamp (s.character, 0, numCharacters - 1)];
        const float a = std::clamp (s.amount, 0.0f, 1.0f), g = std::clamp (s.grit, 0.0f, 2.0f);
        Live t;
        t.pitch = std::clamp (c.pitch * a + s.pitch, -24.0f, 24.0f);
        t.formant = std::clamp (c.formant * a + s.formant, -18.0f, 18.0f);
        t.layerSemis = c.layerSemis;
        t.layerLevel = c.layerLevel * a;
        t.synth = c.synth * a;
        t.noise = c.noise;
        t.vibRate = c.vibRate;
        t.vibDepth = c.vibDepth * a;
        t.ringHz = c.ringHz;
        t.ringMix = std::min (1.0f, c.ringMix * a * g);
        t.growlHz = c.growlHz;
        t.growl = std::min (0.95f, c.growl * a * g);
        t.drive = std::min (1.6f, c.drive * a * g);
        t.crush = std::min (1.0f, c.crush * a * g);
        t.hpHz = std::exp (lerp (std::log (20.0f), std::log (c.hpHz), a));
        t.lpHz = std::exp (lerp (std::log (20000.0f), std::log (c.lpHz), a));
        t.peakHz = c.peakHz;
        t.peakDb = c.peakDb * a;
        t.combHz = c.combHz;
        t.combFb = c.combFb * a;
        t.echoMs = c.echoMs;
        t.echoFb = c.echoFb;
        t.echoMix = c.echoMix * a;
        // robot note: C3 + pitch (the character's own pitch is part of its note)
        t.robotHz = 130.81f * std::pow (2.0f, (c.pitch + s.pitch) / 12.0f);
        return t;
    }

    void updateLive (const Settings& s, int n)
    {
        const Live t = targetFor (s);
        if (first) { live = t; first = false; fixZeroFreqs (live); return; }

        const float a = 1.0f - std::exp (-(float) n / (0.06f * sr));
        auto sm = [a] (float& v, float target) { v += a * (target - v); };
        // frequency-type values: when the new character doesn't use them (0), keep the old value and only fade the amount
        auto smf = [a] (float& v, float target) { if (target > 0.0f) v *= std::pow (target / v, a); };
        sm (live.pitch, t.pitch);       sm (live.formant, t.formant);
        sm (live.layerLevel, t.layerLevel);
        if (t.layerLevel > 0.0f) sm (live.layerSemis, t.layerSemis);
        sm (live.synth, t.synth);       if (t.synth > 0.0f) sm (live.noise, t.noise);
        smf (live.vibRate, t.vibRate);  sm (live.vibDepth, t.vibDepth);
        smf (live.ringHz, t.ringHz);    sm (live.ringMix, t.ringMix);
        smf (live.growlHz, t.growlHz);  sm (live.growl, t.growl);
        sm (live.drive, t.drive);       sm (live.crush, t.crush);
        smf (live.hpHz, t.hpHz);        smf (live.lpHz, t.lpHz);
        smf (live.peakHz, t.peakHz);    sm (live.peakDb, t.peakDb);
        smf (live.combHz, t.combHz);    sm (live.combFb, t.combFb);
        smf (live.echoMs, t.echoMs);    sm (live.echoMix, t.echoMix);
        if (t.echoMix > 0.0f) sm (live.echoFb, t.echoFb); else live.echoFb *= (1.0f - a);
        smf (live.robotHz, t.robotHz);
    }

    static void fixZeroFreqs (Live& l)
    {
        if (l.vibRate <= 0) l.vibRate = 5;
        if (l.ringHz <= 0) l.ringHz = 100;
        if (l.growlHz <= 0) l.growlHz = 40;
        if (l.peakHz <= 0) l.peakHz = 1000;
        if (l.combHz <= 0) l.combHz = 200;
        if (l.echoMs <= 0) l.echoMs = 100;
    }

    void updateFilters()
    {
        hp.highpass (sr, live.hpHz);
        hp2.highpass (sr, live.hpHz);
        lp.lowpass (sr, live.lpHz);
        lp2.lowpass (sr, live.lpHz);
        pk.peak (sr, live.peakHz, live.peakDb, 2.2f);
    }

    void resetState()
    {
        std::fill (outAccum.begin(), outAccum.end(), 0.0f);
        std::fill (outFifo.begin(), outFifo.end(), 0.0f);
        std::fill (combBuf.begin(), combBuf.end(), 0.0f);
        std::fill (echoBuf.begin(), echoBuf.end(), 0.0f);
        hp.reset(); hp2.reset(); lp.reset(); lp2.reset(); pk.reset();
        autoGain = 1.0f;
    }

    float rnd() // -1..1
    {
        seed ^= seed << 13; seed ^= seed >> 17; seed ^= seed << 5;
        return (float) (seed & 0xffffff) / 8388608.0f - 1.0f;
    }

    static float wrapPhase (float p) { return p - twoPi * std::floor ((p + pi) / twoPi); }

    //==========================================================================
    float colour (float y)
    {
        // growl: rough amplitude flutter at a low rate (jittered so it sounds organic, not like a tremolo)
        if (live.growl > 0.001f)
        {
            growlJit += 0.002f * (rnd() - growlJit);
            growlPhase += live.growlHz * (1.0f + 1.5f * growlJit) / sr;
            growlPhase -= std::floor (growlPhase);
            const float mm = 0.5f + 0.5f * std::sin (twoPi * growlPhase);
            y *= 1.0f - live.growl * mm * mm;
        }

        // ring modulator
        if (live.ringMix > 0.001f)
        {
            ringPhase += live.ringHz / sr;
            ringPhase -= std::floor (ringPhase);
            y = y * (1.0f - live.ringMix) + 1.4f * y * std::sin (twoPi * ringPhase) * live.ringMix;
        }

        // metallic comb
        if (live.combFb > 0.001f)
        {
            const float d = std::clamp (sr / live.combHz, 2.0f, (float) combBuf.size() - 3.0f);
            const float v = readFrac (combBuf, combPos, d);
            const float w = y + live.combFb * v;
            combBuf[(size_t) combPos] = w;
            if (++combPos >= (int) combBuf.size()) combPos = 0;
            const float cm = std::min (1.0f, live.combFb * 1.4f);
            y = y * (1.0f - cm) + w * (1.0f - live.combFb) * 1.6f * cm;
        }

        // drive
        if (live.drive > 0.001f)
        {
            const float g = 1.0f + live.drive * 12.0f;
            y = std::tanh (g * y) / std::pow (g, 0.7f);
        }

        // bit crush + sample-rate reduction
        if (live.crush > 0.001f)
        {
            const int hold = 1 + (int) (live.crush * 10.0f * sr / 44100.0f);
            if (++holdCount >= hold) { holdCount = 0; holdValue = y; }
            const float q = std::pow (2.0f, 15.0f - live.crush * 11.0f);
            y = std::round (holdValue * q) / q;
        }

        // filters
        if (live.hpHz > 20.5f) y = hp2.process (hp.process (y));
        if (live.lpHz < 19900.0f) y = lp2.process (lp.process (y));
        if (std::abs (live.peakDb) > 0.05f) y = pk.process (y);

        // echo
        if (live.echoMix > 0.001f || live.echoFb > 0.001f)
        {
            const float d = std::clamp (live.echoMs * 0.001f * sr, 2.0f, (float) echoBuf.size() - 3.0f);
            const float v = readFrac (echoBuf, echoPos, d);
            echoBuf[(size_t) echoPos] = y + v * live.echoFb;
            if (++echoPos >= (int) echoBuf.size()) echoPos = 0;
            y += v * live.echoMix;
        }
        return y;
    }

    static float readFrac (const std::vector<float>& b, int writePos, float delay)
    {
        const int size = (int) b.size();
        float rp = (float) writePos - delay;
        while (rp < 0) rp += (float) size;
        const int i0 = (int) rp;
        const float f = rp - (float) i0;
        const int i1 = (i0 + 1) % size;
        return b[(size_t) i0] + f * (b[(size_t) i1] - b[(size_t) i0]);
    }

    //==========================================================================
    // spectral envelope in natural-log magnitude ("true envelope": cepstral smoothing that rides the harmonic peaks)
    void computeEnvelope()
    {
        for (int k = 0; k <= half; ++k) work[(size_t) k] = logMag[(size_t) k];
        const float invN = 1.0f / (float) N;
        for (int it = 0; it < 3; ++it)
        {
            for (int k = 0; k <= half; ++k) cep[(size_t) k] = { work[(size_t) k], 0.0f };
            for (int k = 1; k < half; ++k) cep[(size_t) (N - k)] = cep[(size_t) k];
            fft.run (cep.data(), true);
            for (int q = 0; q < N; ++q)
            {
                const int qq = q <= half ? q : N - q;
                float w = 0.0f;
                if (qq < lifter) w = 1.0f;
                else if (qq < lifter + 8) w = 0.5f + 0.5f * std::cos (pi * (float) (qq - lifter) / 8.0f);
                cep[(size_t) q] = { cep[(size_t) q].real() * invN * w, 0.0f };
            }
            fft.run (cep.data(), false);
            for (int k = 0; k <= half; ++k)
            {
                envLog[(size_t) k] = cep[(size_t) k].real();
                work[(size_t) k] = std::max (logMag[(size_t) k], envLog[(size_t) k]);
            }
        }
    }

    static void shiftBins (const std::vector<float>& exc, const std::vector<float>& fr, float ratio, int half,
                           std::vector<float>& outMag, std::vector<float>& outFreq)
    {
        std::fill (outMag.begin(), outMag.end(), 0.0f);
        std::fill (outFreq.begin(), outFreq.end(), 0.0f);
        for (int k = 0; k <= half; ++k)
        {
            const int idx = (int) std::lround ((float) k * ratio);
            if (idx > half) break;
            if (idx < 0) continue;
            outMag[(size_t) idx] += exc[(size_t) k];
            outFreq[(size_t) idx] = fr[(size_t) k] * ratio;
        }
    }

    // adds polar(mag * env, phase accumulated from freq) * gain into dst, returns energy added
    float synth (const std::vector<float>& mg, const std::vector<float>& fr, std::vector<float>& acc, float* energy)
    {
        float e = 0.0f;
        for (int k = 0; k <= half; ++k)
        {
            acc[(size_t) k] = wrapPhase (acc[(size_t) k] + twoPi * fr[(size_t) k] / (float) osamp);
            const float a = mg[(size_t) k] * newEnv[(size_t) k];
            tmpSpec[(size_t) k] = std::polar (a, acc[(size_t) k]);
            e += a * a;
        }
        if (energy) *energy = e;
        return e;
    }

    void addTmp (float gain)
    {
        for (int k = 0; k <= half; ++k) outSpec[(size_t) k] += tmpSpec[(size_t) k] * gain;
    }

    void processFrame (Meters& m)
    {
        const float expct = twoPi * (float) hop / (float) N;
        const float binHz = sr / (float) N;

        // ---- analysis
        for (int k = 0; k < N; ++k) buf[(size_t) k] = { inFifo[(size_t) k] * win[(size_t) k], 0.0f };
        fft.run (buf.data(), false);
        float energyIn = 0.0f;
        for (int k = 0; k <= half; ++k)
        {
            const float re = buf[(size_t) k].real(), im = buf[(size_t) k].imag();
            const float mg = std::sqrt (re * re + im * im);
            const float ph = std::atan2 (im, re);
            float d = ph - lastPhase[(size_t) k];
            lastPhase[(size_t) k] = ph;
            d = wrapPhase (d - (float) k * expct);
            mag[(size_t) k] = mg;
            freq[(size_t) k] = (float) k + d * (float) osamp / twoPi;
            logMag[(size_t) k] = std::log (mg + 1.0e-6f);
            energyIn += mg * mg;
        }
        const float rmsDb = 10.0f * std::log10 (energyIn / rmsRef + 1.0e-12f);

        computeEnvelope();

        // ---- pitch: main voice ratio with vibrato
        vibPhase += twoPi * live.vibRate * (float) hop / sr;
        if (vibPhase > twoPi) vibPhase -= twoPi;
        const float vib = live.vibDepth * std::sin (vibPhase);
        const float ratio = std::pow (2.0f, (live.pitch + vib) / 12.0f);
        const float ratio2 = std::pow (2.0f, (live.pitch + live.layerSemis + vib) / 12.0f);
        const float fRatio = std::pow (2.0f, live.formant / 12.0f);

        // ---- excitation (flattened spectrum)
        for (int k = 0; k <= half; ++k) work[(size_t) k] = mag[(size_t) k] * std::exp (-envLog[(size_t) k]);

        // ---- formant shift = stretch the envelope
        for (int k = 0; k <= half; ++k)
        {
            const float src = std::min ((float) k / fRatio, (float) half);
            const int i0 = (int) src;
            const int i1 = std::min (i0 + 1, half);
            const float f = src - (float) i0;
            newEnv[(size_t) k] = std::exp (envLog[(size_t) i0] + f * (envLog[(size_t) i1] - envLog[(size_t) i0]));
        }

        std::fill (outSpec.begin(), outSpec.end(), std::complex<float> {});
        const float voiceAmt = 1.0f - std::clamp (live.synth, 0.0f, 1.0f);

        // ---- your own voice, shifted
        if (voiceAmt > 0.001f)
        {
            float e = 0.0f;
            if (std::abs (ratio - 1.0f) < 1.0e-4f)
            {
                // no pitch change: keep the voice's own phases (clean, no phase-vocoder smear)
                for (int k = 0; k <= half; ++k)
                {
                    const float a = work[(size_t) k] * newEnv[(size_t) k];
                    tmpSpec[(size_t) k] = std::polar (a, lastPhase[(size_t) k]);
                    sumPhase[(size_t) k] = lastPhase[(size_t) k];
                    e += a * a;
                }
            }
            else
            {
                shiftBins (work, freq, ratio, half, synMag, synFreq);
                synth (synMag, synFreq, sumPhase, &e);
            }
            addTmp (voiceAmt * (e > 1.0e-20f ? std::sqrt (energyIn / e) : 0.0f));

            if (live.layerLevel > 0.001f)
            {
                shiftBins (work, freq, ratio2, half, synMag2, synFreq2);
                synth (synMag2, synFreq2, sumPhase2, &e);
                addTmp (voiceAmt * live.layerLevel * (e > 1.0e-20f ? std::sqrt (energyIn / e) : 0.0f));
            }
        }

        // ---- synthetic carrier (robot buzz / whisper), shaped by your formants
        if (live.synth > 0.001f)
        {
            const float gate = std::clamp ((rmsDb + 62.0f) / 14.0f, 0.0f, 1.0f);
            const float buzzAmt = 1.0f - live.noise, noiseAmt = live.noise;
            if (buzzAmt > 0.001f)
            {
                std::fill (carMag.begin(), carMag.end(), 0.0f);
                std::fill (carFreq.begin(), carFreq.end(), 0.0f);
                const float f0 = live.robotHz * std::pow (2.0f, vib / 12.0f) / binHz;
                for (int h = 1; ; ++h)
                {
                    const float fb = f0 * (float) h;
                    const int idx = (int) std::lround (fb);
                    if (idx >= half) break;
                    carMag[(size_t) idx] += 1.0f;
                    carFreq[(size_t) idx] = fb;
                }
                float e = 0.0f;
                synth (carMag, carFreq, carPhase, &e);
                addTmp (live.synth * buzzAmt * gate * (e > 1.0e-20f ? std::sqrt (energyIn / e) : 0.0f));
            }
            if (noiseAmt > 0.001f)
            {
                float e = 0.0f;
                for (int k = 0; k <= half; ++k)
                {
                    const float a = newEnv[(size_t) k];
                    tmpSpec[(size_t) k] = std::polar (a, pi * rnd());
                    e += a * a;
                }
                addTmp (live.synth * noiseAmt * gate * (e > 1.0e-20f ? std::sqrt (energyIn / e) : 0.0f));
            }
        }

        // ---- synthesis + overlap-add
        outSpec[0] = { outSpec[0].real(), 0.0f };
        outSpec[(size_t) half] = { outSpec[(size_t) half].real(), 0.0f };
        for (int k = 0; k <= half; ++k) buf[(size_t) k] = outSpec[(size_t) k];
        for (int k = 1; k < half; ++k) buf[(size_t) (N - k)] = std::conj (outSpec[(size_t) k]);
        fft.run (buf.data(), true);
        for (int k = 0; k < N; ++k) outAccum[(size_t) k] += buf[(size_t) k].real() * win[(size_t) k] * olaScale;
        for (int k = 0; k < hop; ++k) outFifo[(size_t) k] = outAccum[(size_t) k];
        std::copy (outAccum.begin() + hop, outAccum.begin() + hop + N, outAccum.begin());
        for (int k = 0; k < latency; ++k) inFifo[(size_t) k] = inFifo[(size_t) (k + hop)];

        // ---- display: input envelope and the shifted one (dB)
        if (rmsDb > -70.0f)
        {
            const float toDb = 20.0f / std::log (10.0f);
            const float ref = std::log (std::sqrt (rmsRef)) ;
            for (int p = 0; p < numEnvPoints; ++p)
            {
                const float b = std::min (envBin[(size_t) p], (float) half);
                const int i0 = (int) b, i1 = std::min (i0 + 1, half);
                const float f = b - (float) i0;
                const float lin = envLog[(size_t) i0] + f * (envLog[(size_t) i1] - envLog[(size_t) i0]);
                const float shifted = std::log (newEnv[(size_t) i0] + f * (newEnv[(size_t) i1] - newEnv[(size_t) i0]) + 1.0e-9f);
                m.inEnv[(size_t) p].store ((lin - ref) * toDb);
                m.outEnv[(size_t) p].store ((shifted - ref) * toDb);
            }
            m.frames.fetch_add (1);
        }
    }

    //==========================================================================
    float sr = 44100.0f;
    static constexpr int osamp = 4;
    int N = 2048, half = 1024, hop = 512, latency = 1536, fullLatency = 2048, rover = 1536, lifter = 48;
    float olaScale = 1.0f, rmsRef = 1.0f;
    FFT fft;
    std::vector<float> win, inFifo, outFifo, outAccum;
    std::vector<float> mag, freq, lastPhase, logMag, envLog, work, newEnv;
    std::vector<float> synMag, synFreq, synMag2, synFreq2, sumPhase, sumPhase2, carPhase, carMag, carFreq;
    std::vector<std::complex<float>> buf, cep, outSpec, tmpSpec;
    std::array<float, numEnvPoints> envBin {};

    std::vector<float> dryL, dryR;
    int dryDelay = 0, dryPos = 0;
    std::vector<float> combBuf, echoBuf;
    int combPos = 0, echoPos = 0;
    Biquad hp, hp2, lp, lp2, pk;

    Live live;
    bool first = true;
    float vibPhase = 0, growlPhase = 0, growlJit = 0, ringPhase = 0;
    int holdCount = 0;
    float holdValue = 0;
    float inPow = 0, wetPow = 0, autoGain = 1, fxMix = 1, outGain = 1;
    uint32_t seed = 0x1234567u;
};
} // namespace vox
