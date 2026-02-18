// =============================================================================
// NeonFameReverberation — JUCE Parameter Integration
// Connects 7 WebSliderRelay parameters to the SVG arc knob UI.
// =============================================================================

import * as Juce from "./juce/index.js";

// On-screen debug log — visible directly in the plugin footer
function dbg (msg) {
  console.log (msg);
  const el = document.getElementById ("debug-log");
  if (el) el.textContent = msg;
}

// Arc geometry (matches CSS/SVG spec in index.html)
const ARC_CIRC  = 175.929;   // full circumference at r=28
const ARC_SWEEP = 131.947;   // 270° sweep = ARC_CIRC × 0.75

// =============================================================================
// KnobBinding — wires one HTML knob element to one JUCE SliderState
// =============================================================================
class KnobBinding {
  constructor (el) {
    this.el      = el;
    this.param   = el.dataset.param;
    this.min     = parseFloat (el.dataset.min);
    this.max     = parseFloat (el.dataset.max);
    this.defVal  = parseFloat (el.dataset.default);
    this.unit    = el.dataset.unit || "";

    this.arc     = el.querySelector (".value-arc");
    this.display = el.querySelector (".knob-value");

    // JUCE slider state (connected to C++ WebSliderRelay)
    this.state = Juce.getSliderState (this.param);

    this.dragging  = false;
    this.startY    = 0;
    this.startNorm = 0;

    // ── Listen for value changes from C++ (automation, preset load) ──────
    this.state.valueChangedEvent.addListener (() => this._onBackendChange());

    // ── Mouse events for user interaction ────────────────────────────────
    el.addEventListener ("mousedown", e => this._onDown (e));
    el.addEventListener ("dblclick",  () => this._reset ());

    // Render initial value
    this._onBackendChange();
  }

  // Called when the C++ backend sends a new value (automation / preset load)
  _onBackendChange () {
    const norm = this.state.getNormalisedValue();
    this._renderNorm (norm);
  }

  // Mouse press — begin drag
  _onDown (e) {
    dbg (`DOWN: ${this.param} y=${e.clientY}`);
    this.dragging  = true;
    this.startY    = e.clientY;
    this.startNorm = this.state.getNormalisedValue();
    this.el.classList.add ("dragging");
    this.state.sliderDragStarted();
    e.preventDefault();
  }

  // Mouse move (attached globally)
  onMove (e) {
    if (!this.dragging) return;
    const dy    = this.startY - e.clientY;    // up = positive = increase
    const delta = dy / 180;                   // 180 px = full [0,1] range
    const norm  = Math.max (0, Math.min (1, this.startNorm + delta));
    this.state.setNormalisedValue (norm);
    this._renderNorm (norm);
  }

  // Mouse release
  onUp () {
    if (!this.dragging) return;
    this.dragging = false;
    this.el.classList.remove ("dragging");
    this.state.sliderDragEnded();
  }

  // Double-click — reset to default
  _reset () {
    const norm = (this.defVal - this.min) / (this.max - this.min);
    this.state.setNormalisedValue (norm);
    this._renderNorm (norm);
  }

  // Update SVG arc and value display from normalised [0,1] value
  _renderNorm (norm) {
    const len = norm * ARC_SWEEP;
    const gap = ARC_CIRC - len;
    this.arc.setAttribute ("stroke-dasharray", `${len.toFixed(3)} ${gap.toFixed(3)}`);
    this.display.textContent = this._formatScaled (this.state.getScaledValue());
  }

  _formatScaled (v) {
    switch (this.unit) {
      case "ms": return `${Math.round (v)}ms`;
      case "s":  return `${v.toFixed (2)}s`;
      case "%":  return `${Math.round (v * 100)}%`;
      default:   return v.toFixed (2);
    }
  }
}

// =============================================================================
// Initialise all knob bindings on DOM ready
// =============================================================================
document.addEventListener ("DOMContentLoaded", () => {
  const knobEls = document.querySelectorAll (".knob-wrap");
  console.log (`NFR: DOMContentLoaded, found ${knobEls.length} knob-wrap elements`);

  const bindings = Array.from (knobEls).map (el => new KnobBinding (el));

  // Global mouse handlers so drag works even if cursor leaves the knob
  document.addEventListener ("mousemove", e => bindings.forEach (b => b.onMove (e)));
  document.addEventListener ("mouseup",   () => bindings.forEach (b => b.onUp()));

  console.log (`NFR: ${bindings.length} parameters bound to JUCE`);
});
