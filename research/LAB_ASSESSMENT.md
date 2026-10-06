# SimPHY Virtual Laboratory & Assessment

Evidence: `org/shikhar/simphy/lab/**` (`LabTable` 832L, assessment question
classes), `lab.*` strings in `messages_english.properties`, FILE_FORMAT §10,
examples with `<LabTable>`/`<AssessmentQuestions>`.

## 1. Virtual Laboratory table
A spreadsheet-style data table docked in a panel, designed for recording
experimental measurements and computing derived quantities + statistics.

### Structure
- **Rows:** add/remove rows (`lab.tooltip.addrow`/`removerow`); auto serial
  number column (A).
- **Columns:** each has
  - `id` — a valid variable name (used in formulas),
  - `text` — header / graph legend,
  - `formula` — a **math expression over other column names** (exp4j). Empty =
    manual entry. Example headers in properties: `A`, `A^2`, `A+B+C`.
  - `format` — `java.text.DecimalFormat` pattern for display.
- **Data source:** manual entry, **CSV import/export**, or fed from graphers /
  scripts.

### Per-column statistics (right-click popup)
- **Mean**, **Median**, **Mean Deviation**, **Standard Deviation**, **Error**
  (standard error). Computed over the column's numeric values.

### Graphing (built-in, from table data)
- **Regression mode:** None / Constant (y=c) / Linear (y=mx+c) — fits a line and
  shows the fit.
- **Domain column** (x) + multiple **range columns** (y).
- **Rendering style:** Markers Only / Solid / Dotted / Dotted+Markers /
  Solid+Markers.
- **Options:** grids, legends, axis ticks, axis title; **colors:** background /
  foreground.
- This is a self-contained "plot with regression" for lab data, independent of
  the live physics Grapher.

## 2. Assessment questions (embedded in a simulation)
`<AssessmentQuestions><AssessmentQuestion type="…">text</AssessmentQuestion>` —
let a simulation author attach quiz questions to a simulation (e.g. for
auto-graded labs). Four types (OBSERVED in examples + loader):
- **BOOLEAN** — true/false question (answer a boolean).
- **NUMERIC** — numeric answer (tolerance-checked).
- **SELECT** — multiple choice; `options="a,b,c"` attribute lists choices.
- **DESCRIPTION** — free-response / descriptive (manual or rubric).

The loader skips unknown types with a log line. `dialog.test.open.*` strings show
a "Test" file concept (open a test).

## 3. Related: snapshots (test/verify state)
`Snapshot` menu: **Take Snapshot**, **Clear All**, **Last Run** — captures the
world state; `dialog.snapshot.load.error.*` implies snapshots can be saved/loaded
(named). Used to compare runs (error analysis).

## 4. Workflow (inferred from strings)
A lab simulation: user runs the physics, records values into the Lab Table
(manually or auto), computes stats + fits a line (e.g. measure g from a
free-fall t² plot), answers embedded Assessment questions, exports CSV.

## Cross-refs
GRAPHING_DATA.md (live grapher), FILE_FORMAT §10, FEATURES.md (SIMPHY-ANALYSIS-###).
