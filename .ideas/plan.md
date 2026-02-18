# NeonFameReverberation — Implementation Plan

## Complexity Score: 3 / 5

## Implementation Strategy: Phased

Complexity 3 warrants splitting implementation into three logical passes to reduce debugging surface area and catch instability early.

---

## Phase 4.1: Core DSP (Processor)

Get audio flowing through the complete signal path with hard-coded defaults before connecting any parameters.

- [ ] `PluginProcessor.h` — declare all member objects:
  - `juce::dsp::DelayLine<float>` — pre-delay
  - Per-string: 3x `juce::dsp::AllpassFilter<float>` (or manual IIR allpass)
  - Per-string: `juce::dsp::FirstOrderTPTFilter<float>` — damping LP
  - Per-string: `float` feedback accumulator
  - Per-string: `float` LFO phase, LFO rate, LFO depth
  - `juce::LinearSmoothedValue<float>` × 7 — one per parameter
- [ ] `PluginProcessor.cpp` — `prepareToPlay()`:
  - Initialize `juce::dsp::ProcessSpec`
  - Prepare all DSP objects
  - Reset LFO phases
  - Set initial smoothed value targets
- [ ] `processBlock()` — implement full signal chain (sample loop):
  1. Write input to pre-delay, read with offset
  2. Apply tanh drive
  3. Run string A: allpass × 3, add feedback, apply LP
  4. Run string B: allpass × 3, add feedback, apply LP
  5. Sum strings, apply mix blend
- [ ] Verify: dry audio passes through unmodified at `mix=0`, wet signal audible at `mix=1`

---

## Phase 4.2: Parameter Binding (APVTS)

Connect all 7 parameters to the DSP objects using `AudioProcessorValueTreeState`.

- [ ] Declare APVTS in `PluginProcessor.h` with `ParameterLayout`
- [ ] Add all 7 parameters with correct ranges, defaults, and string formatting
- [ ] In `processBlock()` — per block, pull smoothed values:
  - `decay` → compute feedback gain from RT60 formula
  - `tension` → clamp and apply to allpass coefficients
  - `damping` → map [0,1] to [16000, 2000] Hz, set LP cutoff
  - `wobble` → set LFO depth (ms → samples)
  - `drive` → compute tanh scale factor
  - `mix` → read directly
  - `pre_delay` → convert ms → samples, update delay line
- [ ] Verify: all parameters audibly affect output, no zipper noise

---

## Phase 4.3: Editor & Polish

Build the UI and resolve any edge cases found during testing.

- [ ] `PluginEditor.h/cpp` — scaffold editor (framework TBD in design phase)
- [ ] Bind all 7 UI controls to APVTS using `SliderAttachment` / `ButtonAttachment`
- [ ] Stability guards:
  - Clamp feedback gain to max 0.95 regardless of decay value
  - Clamp allpass coefficients to [0.2, 0.8]
  - Guard LFO depth against exceeding delay line bounds
- [ ] State save/restore: verify `getStateInformation` / `setStateInformation` round-trips correctly
- [ ] Tail handling: `getTailLengthSeconds()` returns max decay value (8.0s)
- [ ] `releaseResources()` resets all DSP objects

---

## Dependencies

**Required JUCE Modules:**
- `juce_audio_basics`
- `juce_audio_processors`
- `juce_dsp`
- `juce_gui_basics`
- `juce_gui_extra` (WebView path only)

**No external DSP libraries required.** All components use JUCE DSP primitives.

---

## Risk Assessment

### High Risk
- **Spring tank stability:** Allpass + feedback loops can produce runaway feedback or silence if coefficients are outside safe bounds. Must clamp before writing to DSP objects.
- **Sample-accurate LFO modulation:** Delay line length changes mid-block must be interpolated; abrupt changes cause clicks. Use linear or cubic interpolation on the delay read position.

### Medium Risk
- **Parameter smoothing interactions:** `tension` and `decay` both affect the feedback loop. If both change simultaneously (e.g., automation), the interaction needs testing to confirm no instability.
- **Pre-delay at low latency:** At very short buffer sizes (64 samples), the 100ms pre-delay buffer still functions correctly but must be allocated to maximum size in `prepareToPlay()`, not dynamically.

### Low Risk
- **Drive stage:** tanh is monotone and bounded — cannot go unstable. No edge cases beyond gain=0 (bypass).
- **Mix blend:** Linear interpolation, trivially safe.
- **APVTS parameter binding:** Standard JUCE pattern, well-tested.

---

## Estimated Phase Sequence

```
Phase 4.1  →  DSP skeleton + signal flow (no parameters)
Phase 4.2  →  APVTS binding + parameter response
Phase 4.3  →  Editor + stability guards + state management
```
