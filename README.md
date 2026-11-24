# VoxFXDeck

Simple voice special effects. Put it on a vocal track, click a character, done.

## Characters
| Family | Characters |
|---|---|
| Machine | **Robot** (monotone: your words sung on one note), **Cyborg** (ring mod + metallic ring) |
| Sci-fi | **Alien** (wobbly, ring mod, short echo), **Radio** (walkie-talkie, lo-fi) |
| Creature | **Monster**, **Demon** (octave-down layer + dark echo), **Giant**, **Goblin** |
| Spooky | **Ghost** (whisper + long echo), **Zombie** (raspy half-whisper) |
| Animal | **Chipmunk**, **Duck**, **Cat**, **Dog**, **Elephant**, **Insect** |

The character is a normal parameter, so you can automate it to switch voices during a song.

## Knobs
| Knob | What it does |
|---|---|
| **AMOUNT** (big) | How far toward the character. 0% = your normal voice, 100% = full effect. |
| **PITCH** | Up/down in semitones, on top of the character. On **Robot** it sets the note the robot sings (shown in the display). |
| **FORMANT** | The size of the "throat": LOW = bigger, darker creature; HIGH = smaller, cartoonier. |
| **GRIT** | How rough: scales the growl, distortion, crunch and ring modulation. |
| **MIX** | Original voice vs effect. |
| **OUTPUT** | Output level. |

## Switches
- **FX ON**: compare with your original voice.
- **AUTO LEVEL**: keeps the effect as loud as your original voice, whichever character you pick.

## The display
**Voice shape** shows the resonances (formants) of your voice in grey and the changed ones in blue, plus what's making the sound (your voice, robot note or whisper), the pitch and formant shift, and in/out levels.

## Notes
- Latency is about 43 ms (2048 samples at 44.1/48 kHz). REAPER compensates it automatically, so the effect stays in time. Fine for mixing; not meant for live monitoring.
- Mono or stereo tracks. The voice is processed in mono (as voices are), the dry part keeps its stereo.
- The window is resizable and remembers its size.

## Build
GitHub Actions builds the Windows VST3 on every push: **Actions → latest run → VoxFXDeck-VST3-Windows**. Copy `VoxFXDeck.vst3` to `C:\Program Files\Common Files\VST3`.
