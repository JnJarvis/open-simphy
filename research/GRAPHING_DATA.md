# SimPHY Graphing & Data Analysis

Evidence: `org/shikhar/simphy/gui/Grapher.java`, `LabTable`, `lab.*` +
`grapher.*` strings in `messages_english.properties`, `XChart` usage.

## 1. Live grapher (`Grapher`)
Attaches to a **body**, **joint**, **circuit element**, or **expression**.
Plots time-series data, continuously or by polling.

### Plottable quantities (grapher.* strings)
- **Time** (x-axis domain)
- Body: **Position, Speed, Velocity, Acceleration, Momentum, Angle, Angular
  Velocity, Angular Acceleration, Angular Momentum, Linear KE, Rotational KE,
  Kinetic Energy, Gravitational Energy (PE)**
- Joint: **Force, Torque, Length**
- Circuit: **Current** (plus voltage/power via element info)
- Custom **Script** expressions (up to 3 per object, `Script1/2/3`).

### Controls
- **Polling interval** (frames) and **time span** (seconds) — how fast data is
  sampled / window size.
- **Continuous input** toggle (keep updating) vs stop after span.
- **Stacked** (multiple graphs over one) toggle.
- **Show values**, display axis info / grids / legend.
- **Play/pause**, **Reset** (time→0), **Export to CSV**.
- **Domain (X-axis)** and **Range (Y-axis)** parameter selectors — choose which
  component (e.g. velocity x vs y) to plot.
- Multiple graphs can be stacked; each object can have its own plot.

## 2. Virtual Laboratory table (`LabTable`)
A data table (columns × rows) that can be fed from:
- body/joint/circuit **grapher data**,
- **manual entry**,
- **CSV import**,
- script.

### Columns
Each `LabColumn` has: `text` (header/legend), `formula` (a math expression
computed per row — e.g. `2*x + 3`), `format` (number format), and `data` (comma
list of values). Formulas reference other columns, enabling derived quantities.

### Statistics (per column)
Mean, Median, Mean Deviation, Standard Deviation, Error — computed automatically.

### Graphing from the table
- **Regression mode:** None / Constant (y=c) / Linear (y=mx+c).
- **Domain column** (x) + **Range columns** (y, multiple allowed).
- **Rendering style:** Markers Only / Solid Line Only / Dotted Line Only /
  Dotted+Markers / Solid+Markers.
- **Options:** display grids, legends, axis ticks, axis title.
- **Colors:** graph background / foreground.

## 3. Measurement & readout
- **Ruler** (length), **Protractor** (angle) tools draw live measurement objects.
- **Info mode** (toolbar `InfoMode`) shows an object's properties/coordinates on
  hover/click.
- **FBD** (free-body diagram) mode draws force vectors with configurable names/
  colors (`FBDForceNames`, `FBDForceColors`, `FBDForceStates` in Preferences).
- **Velocity/position/acceleration** display settings (scale, min threshold,
  arrow style) in Preferences.

## 4. Export
- Grapher → **CSV** (time + columns).
- Lab table → **CSV** import/export.
- `World.saveToJson()` / **Export Java** (standalone dyn4j program).
- **Screenshot** (PNG) of the canvas; auto-generated `screenshot.png` in `.ssim`.

## Cross-refs
GRAPHING items per object in SCRIPTING (`Grapher`), FILE_FORMAT §10, FEATURES.md
(SIMPHY-ANALYSIS-###).
