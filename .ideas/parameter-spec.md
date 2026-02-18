# MyReverb — Parameter Specification

## Parameter Table

| ID | Name | Type | Range | Default | Unit | Description |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| `mix` | Mix | Float | 0.0 – 1.0 | 0.5 | % (0–100) | Dry/wet blend. 0 = fully dry, 1 = fully wet. |
| `decay` | Decay | Float | 0.1 – 8.0 | 2.0 | s | Reverb tail length (RT60). Short for tight rooms, long for deep washes. |
| `tension` | Tension | Float | 0.0 – 1.0 | 0.5 | — | Spring tightness. Low = loose, elastic boing. High = tight, metallic ring. Controls allpass density and resonance. |
| `pre_delay` | Pre-Delay | Float | 0.0 – 100.0 | 10.0 | ms | Delay before the reverb signal is fed into the spring. Creates separation between dry transient and reverb bloom. |
| `damping` | Damping | Float | 0.0 – 1.0 | 0.4 | — | High-frequency absorption in the reverb tail. Low = bright, sizzling tail. High = dark, muffled decay. |
| `wobble` | Wobble | Float | 0.0 – 1.0 | 0.3 | — | Modulation depth applied to the spring delay lines. Adds the characteristic elastic pitch drift of a real spring tank. |
| `drive` | Drive | Float | 0.0 – 1.0 | 0.2 | — | Soft saturation applied before the spring input. Warms the feed and adds harmonic density without hard clipping. |

## Parameter Notes

### Tension
Implemented as control over the allpass filter coefficients in the spring tank simulation. Higher tension values increase the number of dense reflections and boost the resonant frequency of the tank — producing the metallic "twang" associated with tighter springs.

### Wobble
A low-frequency oscillator (LFO) modulates the delay line lengths within the spring tank, creating subtle pitch modulation. Rate is fixed internally at ~0.5–1.2 Hz (appropriate for spring simulation); only depth is user-controlled. Increasing wobble adds warmth and movement, especially noticeable on sustained pads.

### Drive
Soft-knee saturation (tanh-based or waveshaper) applied to the pre-spring signal chain. Not a distortion effect — the intent is subtle harmonic enrichment. At maximum, a gentle crunch becomes audible on transient peaks.

### Pre-Delay
Classic mixing technique: increasing pre-delay separates the dry source from the reverb onset, helping transients cut through in a dense mix. Particularly useful on snare hits and percussive stabs in house music.

## Automation
All 7 parameters are automatable. Smooth interpolation applied to avoid zipper noise on decay, tension, and wobble (minimum 10ms ramp time).
