# Style Guide v1 — NeonFameReverberation

## Design Base: NeoGrid Minimal

---

## Color Tokens

| CSS Variable | Hex | Usage |
|:---|:---|:---|
| `--bg` | `#111115` | Plugin shell background |
| `--surface` | `#1E1E24` | Panel fill |
| `--surface-hl` | `#2A2A33` | Knob arc track, borders highlight |
| `--primary` | `#00FFFF` | Knob value arcs, center dot, value text, glow |
| `--text` | `#F0F0F5` | Primary labels (plugin name, hover state) |
| `--muted` | `#A0A0B0` | Knob labels (default state), section names |
| `--border` | `#333340` | Panel borders, screw outlines |

---

## Typography

| Element | Font | Size | Weight | Tracking | Transform |
|:---|:---|:---|:---|:---|:---|
| Plugin name | Inter | 14px | 700 | 0.18em | uppercase |
| Plugin subtitle | Inter | 10px | 500 | 0.20em | uppercase |
| Section labels | Inter | 9px | 700 | 0.20em | uppercase |
| Knob labels | Inter | 9px | 700 | 0.12em | uppercase |
| Knob value | Courier New | 9px | 400 | 0.05em | — |
| Footer | monospace | 9px | 400 | 0.12em | uppercase |

Fallback stack: `'Inter', 'Helvetica Neue', Arial, sans-serif`

---

## SVG Knob Specification

### Geometry
- **ViewBox:** `0 0 64 64`
- **Center:** `cx=32, cy=32`
- **Radius:** `r=28`
- **Sweep:** 270° (7 o'clock to 5 o'clock, clockwise through 12 o'clock)
- **Rendered size:** 52×52px container

### Track Arc (static background)
```svg
<path d="M12.2 51.8 A28 28 0 1 1 51.8 51.8"
      fill="none" stroke="#2A2A33" stroke-width="7" stroke-linecap="round"/>
```

### Value Arc (dynamic, JS-controlled)
```svg
<circle class="value-arc"
        cx="32" cy="32" r="28"
        fill="none" stroke="#00FFFF" stroke-width="7" stroke-linecap="round"
        transform="rotate(135 32 32)"
        stroke-dasharray="0 175.93"/>
```
**Dasharray formula:**
- Full circumference: `2π × 28 ≈ 175.929`
- Full 270° arc: `175.929 × 0.75 ≈ 131.947`
- For normalized value `n` (0.0 – 1.0):
  - `stroke-dasharray: {n × 131.947} {175.929 − (n × 131.947)}`

### Center Dot
```svg
<circle cx="32" cy="32" r="3" fill="#00FFFF" opacity="0.4"/>
```

---

## Panel Specification

```css
.panel {
  background: rgba(0,0,0,0.18);
  border: 1px solid #333340;
  border-radius: 4px;
  padding: 8px 12px 6px;
}
.panel-label {
  font-size: 9px; font-weight: 700; letter-spacing: 0.2em;
  color: #A0A0B0; opacity: 0.5; text-transform: uppercase;
  margin-bottom: 6px;
}
```

---

## Plugin Shell Specification

```css
.plugin {
  width: 680px; height: 280px;
  background: linear-gradient(160deg, #1E1E24 0%, #111115 100%);
  border: 1px solid #333340;
  border-top: 3px solid #00FFFF;   /* NeoGrid signature */
  overflow: hidden;
}
```

**Corner screws:** 10px circles, `#0d0d12` fill, `#2a2a35` border, positioned 8px inset from each corner. 45°-rotated 1px `#333` line inside.

---

## State Definitions

| State | Description | Visual Change |
|:---|:---|:---|
| Default | Knob at rest | Label: `--muted`, arc: `--primary` |
| Hover | Mouse over knob | Label: `--text` |
| Dragging | Mouse held + moving | Arc: `drop-shadow(0 0 4px #00FFFF)` |
| At min | Value = minimum | Arc dasharray: `0 175.93` (no arc visible) |
| At max | Value = maximum | Arc dasharray: `131.95 43.98` (full 270° arc) |

---

## Spacing System

| Element | Value |
|:---|:---|
| Body padding | 12px vertical, 20px horizontal |
| Panel gap | 12px |
| Knob gap (in panel) | 8px |
| Knob container width | 68px |
| Knob label gap below SVG | 4px |
| Header height | 44px |
| Footer height | 24px |
