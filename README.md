# VoxFXDeck

Simple voice special effects. Put it on a vocal track, click a character, done.

## Characters
| Row | Characters |
|---|---|
| 1 – Vocoders | **Robot** (monotone, one note), **Talkbox** (Daft Punk-style synth singing your melody), **Vocoder** (classic, wide), **Prism** (Kanye / Bon Iver stacked harmonies), **Hard Tune** (T-Pain / Kanye Auto-Tune) |
| 2 | **Cyborg**, **Alien**, **Radio**, **Ghost**, **Zombie** |
| 3 | **Monster**, **Demon**, **Giant**, **Goblin**, **Chipmunk** |
| 4 | **Duck**, **Cat**, **Dog**, **Elephant**, **Insect** |

The character is a normal parameter, so you can automate it to switch voices during a song.

## Playing it with MIDI (top row)
Turn on **MIDI KEYS**, then send MIDI to the plugin:
- **Easiest in REAPER:** put a MIDI item (your chords/melody) on the *same track* as the vocal. REAPER sends it to the plugin.
- Or send MIDI from another track (Routing → MIDI send to the vocal track, audio off).
- Or play live: record-arm the vocal track with your MIDI keyboard as input and monitoring on.

What the keys do:
- **Robot / Talkbox / Vocoder / Prism:** the synth plays the notes you hold (chords work, up to 8 notes, sustain pedal and pitch bend too). No keys = silent, except consonants (s, t, k) which always come through so the words stay clear.
- **Hard Tune:** your voice snaps to the notes you hold instead of the nearest semitone.

Without MIDI KEYS: Robot sings one note (set with PITCH), Talkbox and Vocoder follow your melody (snapped to notes), Prism adds a fifth and octaves on top of your melody, Hard Tune snaps to the nearest semitone.

## Knobs
| Knob | What it does |
|---|---|
| **AMOUNT** (big) | How far toward the character. 0% = your normal voice, 100% = full effect. On **Hard Tune**, lower = softer, more natural tuning. |
| **PITCH** | Up/down in semitones, on top of the character. On **Robot** it sets the note the robot sings. On the vocoders it transposes the synth. |
| **FORMANT** | The size of the "throat": LOW = bigger, darker creature; HIGH = smaller, cartoonier. |
| **GRIT** | How rough: scales the growl, distortion, crunch and ring modulation. |
| **MIX** | Original voice vs effect. |
| **OUTPUT** | Output level. |

## Switches
- **FX ON**: compare with your original voice.
- **AUTO LEVEL**: keeps the effect as loud as your original voice, whichever character you pick.
- **MIDI KEYS**: play the top row with MIDI notes (see above). The header turns gold if MIDI is arriving while it's off.

## The display
**Voice shape** shows the resonances (formants) of your voice in grey and the changed ones in blue, plus what's making the sound (your voice, robot note, synth note, harmonies, keys held, tuned note), the note you're singing, the pitch and formant shift, and in/out levels.

## Notes
- Latency is about 43 ms (2048 samples at 44.1/48 kHz). REAPER compensates it automatically, so the effect stays in time. Fine for mixing; not meant for live monitoring.
- Mono or stereo tracks. The voice is processed in mono (as voices are), the dry part keeps its stereo.
- The window is resizable and remembers its size.

## Build
GitHub Actions builds the Windows VST3 on every push: **Actions → latest run → VoxFXDeck-VST3-Windows**. Copy `VoxFXDeck.vst3` to `C:\Program Files\Common Files\VST3`.
