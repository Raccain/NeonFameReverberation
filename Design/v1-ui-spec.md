# UI Specification v1 — NeonFameReverberation

## Design Base
**Library:** NeoGrid Minimal

## Display Name
`NEONFAME` (cyan) + `REVERBERATION` (white) — rendered as `<em>NEONFAME</em>REVERBERATION`

## Layout
- **Window:** 680 × 280px (fixed, overflow hidden)
- **Structure:** Vertical stack — Header (44px) / Body (flex) / Footer (24px)
- **Body layout:** Three horizontal panels in a single row, left-to-right

### Panels
| Panel | Label | Controls | Width |
|-------|-------|----------|-------|
| INPUT | "Input" | Drive (1 knob) | 88px fixed |
| SPRING | "Spring" | Pre-Delay, Tension, Decay, Damping, Wobble (5 knobs) | flex (fills remaining) |
| OUTPUT | "Output" | Mix (1 knob) | 88px fixed |

## Controls

| Parameter ID | Name | Panel | Type | Range | Default | Display |
|:---|:---|:---|:---|:---|:---|:---|
| `drive` | Drive | INPUT | Rotary knob | 0.0 – 1.0 | 0.2 | 0.00 |
| `pre_delay` | Pre-Dly | SPRING | Rotary knob | 0.0 – 100.0 | 10 | `10ms` |
| `tension` | Tension | SPRING | Rotary knob | 0.0 – 1.0 | 0.5 | 0.50 |
| `decay` | Decay | SPRING | Rotary knob | 0.1 – 8.0 | 2.0 | `2.00s` |
| `damping` | Damping | SPRING | Rotary knob | 0.0 – 1.0 | 0.4 | 0.40 |
| `wobble` | Wobble | SPRING | Rotary knob | 0.0 – 1.0 | 0.3 | 0.30 |
| `mix` | Mix | OUTPUT | Rotary knob | 0.0 – 1.0 | 0.5 | `50%` |

## Color Palette
- **Background:** `#111115`
- **Surface (panels):** `#1E1E24`
- **Surface Highlight:** `#2A2A33`
- **Primary Accent:** `#00FFFF` (cyan — knob arcs, glow on drag)
- **Text Primary:** `#F0F0F5`
- **Text Secondary / Labels:** `#A0A0B0`
- **Border:** `#333340`

## Visual Style Notes
- 3px cyan top border on plugin shell (NeoGrid signature element)
- Corner screw details at all four corners (10px decorative circles)
- Panels: semi-transparent dark background, 1px solid border, 4px radius
- Knobs: SVG arc-only style (270° sweep, no physical cap body)
- Value display: monospace cyan text beneath each knob label
- Header: plugin name left-aligned in spaced caps; subtitle right-aligned muted text
- Footer: very low opacity monospace status text

## Interaction
- Vertical mouse drag on knob: up = increase value (180px = full range)
- Double-click knob: reset to default
- Hover knob label: label brightens from `--muted` → `--text`
- Active drag: cyan glow `drop-shadow(0 0 4px #00FFFF)` on value arc
