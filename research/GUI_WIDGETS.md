# SimPHY GUI Widget System (scripted UI)

Evidence: `simphy.script.widgets.*` (in scripts.jar), `org/shikhar/simphy/gui/**`
(`Gui`, `GuiManager`, `SimphyWidget`, property tables), `propertytable.*` tooltips
in `messages_english.properties`, examples with `<GuiXML>`.

SimPHY lets scripts **build custom GUI panels** (docked, tabbed, or dialog) using
an XML widget description (`<GuiManager><GuiXML><![CDATA[…]]></GuiXML>`) or the
`Widgets` JS API. Each widget can bind an **action** and a **perform** callback to
any public script method.

## 1. Widget catalogue
Containers: `Desktop`, `Panel`, `Dialog`, `SplitPane`, `TabbedPane`, `MenuBar`,
`Menu`, `Table`, `TreeView`, `Plotter`.
Items: `Label`, `Button`, `ToggleButton`, `CheckBox`, `TextField`,
`PasswordField`, `TextArea`, `ComboBox`, `ListBox`, `Slider`, `SpinBox`,
`ProgressBar`, `ItemWidget`, `SelectableItemWidget`, `ActionItemWidget`.

## 2. Common widget properties (from property-table tooltips)
- `text` — caption; for listbox/combobox a **comma-separated** item list.
- `tabPlacement` — `top|left|bottom|right|stack|none` (TabbedPane).
- **`action`** — script method invoked on interaction (button click, slider
  change, list double-click, checkbox toggle…).
- **`perform`** — script method invoked on Enter/double-click (text fields, lists).
- Both accept params: `this`, `this.attribute`, string literals, numeric literals.
- Layout: padding, alignment, borders, preferred size, visible/enabled, colors.
- `Plotter` widget: text = set of plottable expressions in `x` joined by `;`
  (e.g. `sin(x); x*x`) — embeds live function plots.

## 3. Interaction model
- `Widgets.loadFromXml(xmlString)` parses & lays out the tree, returns the root.
- Script attaches handlers; widget exposes `name`, `value`/`text`, and a
  `perform()`/`action` trigger. `this` inside handlers refers to the widget
  (so scripts read `this.value`, `this.text`, etc.).
- Widgets can host `Table` (data grid), `Plotter` (graphs), `ProgressBar`.

## 4. Docking & management
- `GuiManager` manages multiple GUI panels (one per `<GuiManager>`), each
  dockable to a side/bottom tab, resizable, closable.
- `SimphyWidget`/`Gui` provide the Swing side (XChart plotter, RSTA tables).
- Built-in panels (not script widgets): **Objects table**, **Grapher**, **Lab
  table**, **Console**, **Script editor**, **Calculator**, **Preferences**,
  **Status bar** — see EDITOR_TOOLS.md / GUI panels.

## 5. Example usage (OBSERVED pattern)
A simulation's `<GuiXML>` declares panels with sliders/buttons/labels; the
embedded `<Script>` reads `widget.value` in `action`/`perform` methods to drive
the physics (e.g. a "launch angle" slider that sets an initial velocity, a
"show graph" toggle).

## Cross-refs
SCRIPTING §5, FILE_FORMAT §5.9, EDITOR_TOOLS, FEATURES.md (SIMPHY-UI-###).
