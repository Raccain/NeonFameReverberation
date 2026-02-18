# MyReverb — DSP Architecture Specification

## Core Components

### 1. Pre-Delay Buffer
Circular delay buffer placed at the input before any processing.
- Length: 0–100 ms (sample-rate independent)
- Implementation: `juce::dsp::DelayLine<float>` or manual circular buffer
- Parameter: `pre_delay`

### 2. Drive Stage (Input Saturation)
Soft-knee waveshaper applied after pre-delay, before the spring tank.
- Algorithm: `tanh(x * gain) / tanh(gain)` (normalized so unity gain at drive=0)
- Drive range maps to internal gain factor: 0.0 → 1.0x, 1.0 → 4.0x (mild to noticeable saturation)
- Real-time safe: pure math, no allocation
- Parameter: `drive`

### 3. Spring Tank Simulation
Two parallel allpass-chain "strings" summed together, each with an internal feedback loop.

**Per string:**
- 3 allpass filters in series (Schroeder-style)
- Allpass delay lengths: approximately 5ms, 9ms, 14ms (staggered for density)
- Allpass coefficient controlled by `tension` → maps to range [0.3, 0.75]
- Feedback loop around each string with gain controlled by `decay`
- `juce::dsp::FirstOrderTPTFilter<float>` (lowpass) inside feedback loop, cutoff controlled by `damping`
- Delay line lengths of String B are offset by ~2ms vs String A to decorrelate the two strings

**Modulation (Wobble):**
- 1 LFO per string (slightly detuned: ~0.5 Hz and ~0.7 Hz)
- LFO modulates the last allpass delay length in each string (±0.5–3ms depth)
- Wobble parameter controls modulation depth: 0.0 → 0ms, 1.0 → 3ms peak deviation
- LFO phase starts at 0, advances per sample (not block-rate)

**Parameters consumed by Spring Tank:**
- `tension` → allpass coefficient [0.3 – 0.75]
- `decay` → feedback gain [0.1 – 0.95] (computed from RT60 target)
- `damping` → LP filter cutoff [2000 – 16000 Hz] (inverted: high damping = low cutoff)
- `wobble` → LFO depth [0 – 3ms]

### 4. Mix Blend
Simple crossfade between dry (original input, before pre-delay) and wet (spring tank output).
- `mix = 0.0` → 100% dry
- `mix = 1.0` → 100% wet
- Linear blend (not equal-power; spring reverbs are typically linear blend)
- Parameter: `mix`

---

## Processing Chain

```
Input (dry tap saved here)
  │
  ▼
[Pre-Delay Buffer]    ← pre_delay (0–100ms)
  │
  ▼
[Drive Stage]         ← drive (tanh saturation)
  │
  ├─────────────────────────────────┐
  ▼                                 ▼
[String A]                       [String B]
  AP1 → AP2 → AP3 → Σ              AP4 → AP5 → AP6 → Σ
  ↑  LFO-A modulates AP3           ↑  LFO-B modulates AP6
  └──[Feedback × decay]◄LP──┘      └──[Feedback × decay]◄LP──┘
     ↑ damping controls LP            ↑ damping controls LP
  │                                 │
  └──────────────┬──────────────────┘
                 │
                 ▼
         [String A + B sum]
                 │
                 ▼
         [Mix Blend]            ← mix (dry/wet)
                 │
                 ▼
              Output
```

---

## Parameter Mapping

| Parameter | Component | DSP Function | Internal Range |
|-----------|-----------|-------------|----------------|
| `mix` | Mix Blend | Wet/dry crossfade | 0.0 – 1.0 (linear) |
| `decay` | Spring Tank feedback | Feedback gain coefficient | 0.1 – 0.95 |
| `tension` | Spring Tank allpass | Allpass filter coefficient | 0.3 – 0.75 |
| `pre_delay` | Pre-Delay Buffer | Buffer read offset | 0 – sampleRate * 0.1 samples |
| `damping` | Spring Tank LP filter | LP cutoff frequency | 2000 – 16000 Hz (inverted) |
| `wobble` | LFO A + B | Delay modulation depth | 0 – 3ms peak |
| `drive` | Drive Stage | tanh gain factor | 1.0 – 4.0x |

### Parameter Smoothing
All parameters use `juce::LinearSmoothedValue<float>` with 10ms ramp time to prevent zipper noise. Applied to: `mix`, `decay`, `tension`, `damping`, `wobble`, `drive`. Pre-delay uses sample-accurate interpolation.

---

## JUCE Modules Required

```cmake
target_link_libraries(MyReverb PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_processors
    juce::juce_dsp
    juce::juce_gui_basics
    juce::juce_gui_extra
)
```

| Module | Usage |
|--------|-------|
| `juce_audio_basics` | AudioBuffer, MidiMessage |
| `juce_audio_processors` | AudioProcessor, parameters (APVTS) |
| `juce_dsp` | DelayLine, FirstOrderTPTFilter, LinearSmoothedValue |
| `juce_gui_basics` | Editor base, Component |
| `juce_gui_extra` | WebBrowserComponent (if WebView path) |

---

## Complexity Assessment

**Score: 3 / 5 — Advanced**

**Complexity factors:**
- **Spring tank:** Two parallel allpass-chain strings with individual feedback loops. Each string has 3 allpass filters, a lowpass damping filter, and a modulated delay line — roughly 8–10 DSP objects per string, 16–20 total.
- **LFO modulation:** Two independent LFOs with slightly offset rates, each modulating a delay line length. Requires sample-accurate modulation to avoid clicks.
- **Parameter interactions:** `tension`, `decay`, `damping`, and `wobble` all act within the same feedback loop. Incorrect ordering or smoothing can cause instability or audible artifacts.
- **Stability concerns:** Allpass-based reverb with high feedback can go unstable if decay > 1.0 or allpass coefficients exceed bounds. Needs clamping guards.
- **Straightforward parts:** Drive stage, pre-delay, and mix blend are all simple and well-understood.
