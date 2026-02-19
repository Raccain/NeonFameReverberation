# NeonFameReverberation — Claude Context

## Repo Identity
This is a **standalone git repo** living inside `audio-plugin-coder/plugins/NeonFameReverberation/`.
All `git` commands must target this repo, NOT the parent `audio-plugin-coder` repo.

```bash
# Correct — always use -C or work from this directory
git -C /Users/peterbojesen/Programming/DSP/audio-plugin-coder/plugins/NeonFameReverberation status
```

## Build Rules (CRITICAL)
- **NEVER** copy the built VST3 to `~/Library/Audio/Plug-Ins/VST3/` — the user does this manually
- **Always build to the local `build/` folder only**
- **Always build universal binary (x86_64;arm64)** — arm64-only builds are invisible to Cubase
- If cmake was reconfigured for arm64-only, restore with:
  ```bash
  cmake -S /Users/peterbojesen/Programming/DSP/audio-plugin-coder -B /Users/peterbojesen/Programming/DSP/audio-plugin-coder/build -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64"
  ```
- Build command (run from `audio-plugin-coder/` root):
  ```bash
  cmake --build /Users/peterbojesen/Programming/DSP/audio-plugin-coder/build --config Release --target NFReverb_VST3
  ```
- If cmake says "no work to do", force rebuild by touching a source file first:
  ```bash
  touch /Users/peterbojesen/Programming/DSP/audio-plugin-coder/plugins/NeonFameReverberation/Source/PluginProcessor.cpp
  ```
- Built artifact location:
  ```
  build/plugins/NeonFameReverberation/NFReverb_artefacts/VST3/NeonFameReverberation.vst3
  ```
- **Note:** Finder shows the bundle folder's timestamp (stale), not the binary inside. The binary timestamp is always correct.

## Platform
- **Development & testing on macOS** — Cubase for Mac
- WebView backend: `defaultBackend` (maps to WKWebView on macOS automatically)
- Windows: `Backend::webview2` + `WinWebView2Options` (guarded by `#if JUCE_WINDOWS`)

## Project Structure
```
NeonFameReverberation/
├── CLAUDE.md                        ← this file
├── CMakeLists.txt                   ← VERSION currently 1.1.0
├── status.json                      ← APC phase state
├── .ideas/                          ← creative-brief, parameter-spec, architecture, plan
├── Design/                          ← v1/v2 UI specs and preview HTML files
└── Source/
    ├── PluginProcessor.h/cpp        ← DSP: Schroeder allpass spring reverb
    ├── PluginEditor.h/cpp           ← WebView host, parameter relay bindings
    ├── ParameterIDs.hpp             ← parameter ID constants
    └── ui/public/
        ├── index.html               ← PRODUCTION UI (inline JS, self-contained)
        └── js/
            ├── index.js             ← old ES module JS (not loaded by index.html)
            └── juce/
                ├── index.js         ← JUCE frontend library
                └── check_native_interop.js
```

## UI Architecture
- **Framework:** WebView (HTML5 Canvas / SVG knobs)
- **Size:** 620 × 220 px
- **Production file:** `Source/ui/public/index.html` — fully self-contained, all JS inline
- **JS interop pattern:** Inline `SliderState` class using `window.__JUCE__.backend.emitEvent` / `addEventListener`
- **Do NOT use** `Source/ui/public/js/index.js` — it uses ES module imports which are unreliable in WKWebView
- **Preview file:** `Design/v2-test.html` — open in browser for UI iteration (has local Knob simulator, no JUCE needed)

## Parameters (7 total)
| ID | Label | Range | Default |
|---|---|---|---|
| `mix` | Mix | 0–1 | 0.5 |
| `decay` | Decay | 0.1–10.0s | 2.0s |
| `tension` | Tension | 0–1 | 0.5 |
| `pre_delay` | Pre-Delay | 0–100ms | 0 |
| `damping` | Damping | 0–1 | 0.5 |
| `wobble` | Wobble | 0–1 | 0.2 |
| `drive` | Drive | 0–1 | 0 |

## Git Workflow
- **Active branch:** `feature/ui-signal-flow`
- **Branch structure:** `master → develop → feature/...`
- **Never stage:** `.DS_Store` files
- **Commit only source files:** `Source/`, `Design/`, `.ideas/`, `CMakeLists.txt`, `status.json`

## Known Issues / History
- **Mix knob had no effect** (fixed): production `index.html` had preview-only Knob simulator — replaced with real `SliderState` JUCE interop
- **Window size mismatch** (fixed): C++ `setSize` was 680×280, HTML was 620×220 — now both 620×220
- **WebView backend** (fixed): was hardcoded to `webview2` (Windows-only) — now platform-conditional
- **Audio artefacts** (under investigation): user reported strange artefacts — possibly drive saturation or feedback accumulation (fbGain ≈ 0.95)

## DSP Notes
- Schroeder allpass spring reverb (3 allpass stages per channel)
- Mix blend: `output = dry * (1 - mix) + wet * mix` — DSP code is correct
- Feedback gain: `fbGain` computed from decay parameter, approaches 0.95 at long decay settings
- Drive: soft saturation via `applyDrive()`, gain = `1.0 + drive * 3.0`
