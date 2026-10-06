# SimPHY Circuit Simulator

Evidence: `org/shikhar/simphy/circuit/**` (~40 classes, ~11k lines), toolbar
catalogue in `GuiManager` (element list + labels + icons), `Serializer` (element
registry), examples `Circuit Demo.ssim`, `Potentiometer Experiment.ssim`.

## 1. Architecture
- A circuit is a separate schematic (not dyn4j). `CircuitManager` owns the active
  circuit; `CirSim` is the **solver + document** (node list, element list,
  time-step integrator).
- **Solver = time-domain Modified Nodal Analysis.** Nodes (`CircuitNode`) are
  merged by position; a linear system is stamped from element conductances/
  sources and solved (Gaussian elimination). Singular/NaN matrices are detected
  and reported. Inductors/capacitors use the **Trapezoidal** discretization
  (per-element toggle).
- Each element exposes `getValue()`/`getUnit()`/`getCurrent()`/`getVoltageDiff()`/
  `getPower()` and one or more editable parameters (`getEditInfo`/`setEditValue`),
  where a parameter can be a **numeric value or an expression** (time-dependent).
- Live display: per-element info (value, current, power) can be drawn; the
  circuit can be color-coded by voltage/power; current direction animates.

## 2. Element catalogue (toolbar groups, OBSERVED)
### Elements
| Element | Class | Editable parameters |
|---|---|---|
| Wire | `WireElm` | — |
| Resistor | `ResistorElm` | Resistance (ohms) [expr] |
| Capacitor | `CapacitorElm` | Capacitance (F), Trapezoidal on/off, initial Voltage |
| Inductor | `InductorElm` | Inductance (H), Trapezoidal, Initial Current |
| DC Battery | `DCVoltageElm` | Voltage, (frequency n/a) |
| AC Source | `ACVoltageElm` | Voltage (amplitude), Frequency |
| Current Source | `CurrentElm` | Current (A) [expr] |
| Switch | `SwitchElm` | Momentary, Switch Group, Closed |
| SPDT Switch | `Switch2Elm` | Number of Throws, Position |
| Push Switch | `PushSwitchElm` | — |

### Devices (meters / sources / loads)
| Element | Class | Editable parameters |
|---|---|---|
| Ammeter | `AmmeterElm` | Resistance, Range (A), Input Waveform, Mode |
| Voltmeter | `VoltmeterElm` | Resistance, Range (V), Input Waveform, Mode |
| Galvanometer | `GalvanometerElm` | Resistance, Max Deflection Current, Mode |
| Transformer | `TransformerElm` | Primary Inductance, Ratio, Coupling Coefficient, Trapezoidal, Swap Secondary Polarity |
| Tapped Transformer | `TappedTransformerElm` | Primary Inductance, Ratio, Coupling Coefficient, Trapezoidal |
| Potentiometer | `PotElm` | Resistance, Position (0..1), Show Ruler |
| Voltage In / Ground / Voltage Out | `RailElm`/`GroundElm`/`OutputElm`/`SquareRailElm`/`ACRailElm` | — |
| Bulb | `BulbElm` | Nominal Power, Nominal Voltage, Light Color |

### Electronics
| Element | Class | Notes |
|---|---|---|
| Diode | `DiodeElm` | Fwd Voltage @ 1A |
| LED | `LEDElm` | forward voltage, emits light |
| Zener Diode | `ZenerElm` | Fwd Voltage @ 1A, Zener Voltage @ 5mA |
| NPN Transistor | `NTransistorElm`/`TransistorElm` | Beta (hFE), Swap E/C |
| PNP Transistor | `PTransistorElm` | Beta (hFE), Swap E/C |
| NOT / AND / OR / NOR / NAND / XOR | `InverterElm`,`AndGateElm`,`OrGateElm`,`NorGateElm`,`NandGateElm`,`XorGateElm` (base `GateElm`) | logic gates |
| Logic Input / Logic Output | `LogicInputElm`,`LogicOutputElm` | digital pins |

### Tools
- Add Graph to the element (attach a `Grapher` to any element).
- Show info tool (display V/I/P on an element).

Also present: `AntennaElm` (RF antenna, AC), `TextElm` (label), `RowInfo`.

## 3. AC / transients
- AC sources produce sinusoidal drive at a set frequency; combined with
  R/L/C + transformers + diodes this supports rectification, filters, resonance,
  coupled circuits (see `Resonance in Action`, `Charge oscillation` examples).
- Digital logic block (gates + inputs/outputs) coexists with analog in one net.

## 4. Serialization
Directly inspected evidence: [Potentiometer XML](fixtures/extracted/Potentiometer_Experiment/simulation.xml),
`/Simulation/World/Circuit`, version 4.0. Its CDATA contains 18 nonempty lines:
one `$` header, 16 element records, and a final `%` line. Observed element tokens
are `v`, `w`, `r`, `p`, `)`, and `s`; records do not start with Java class names.
The earlier class-name/reflection account is unverified and must not define the
file parser. It may concern a different serialization path; that remains unknown.

Observed lexical structure for this fixture only:

```text
document       = header-line, newline, element-lines, final-line
header-line    = "$", whitespace, opaque-header-fields
element-line   = token, whitespace, number, whitespace, number, whitespace,
                 number, whitespace, number, whitespace, integer, whitespace,
                 name-token, whitespace, opaque-tail
token          = "v" | "w" | "r" | "p" | ")" | "s"
final-line     = "%"
```

This describes the inspected sample, not an accepted general grammar. Numbers
include signed decimals and scientific notation. Four leading numbers appear to
be endpoint coordinates; the integer appears to encode flags. Those meanings are
inferences. Type-specific tails differ and must remain uninterpreted until mapped.
Repeated `name_#_=_#_value` tokens occur; their escaping/metadata semantics are unknown.
The header contains numeric fields and hex-color-like tokens whose order/meaning
has not been verified. `$` and `%` are observed delimiters; their general roles,
optionality, and version behavior require further evidence.

Before a decoder is READY, obtain token-to-element mappings, field order/types,
units/defaults per element and version, header semantics, string escaping and
line-ending rules, unknown-record handling, malformed-input cases, and round-trip
fixtures. Keep the circuit payload separate from outer XML parsing. No circuit
simulation or round-trip behavior was tested in this correction pass.

## 5. Graphing
Any circuit element can be graphed: time vs **Voltage / Current / Power** (see
GRAPHING_DATA.md). Meters report within their range; over-range behavior shown.

## Cross-refs
FILE_FORMAT §9, GRAPHING_DATA, SCRIPTING (§Circuit API), FEATURES.md (OBJ-C###).
