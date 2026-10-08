# User interface direction — 2026-10-08

User supplied a SimPHY screenshot and requested this general layout eventually.
Use it as a workflow/layout reference, not a source of branding or copied assets.

- Dominant central/right 2D viewport with adaptive major/minor grid, numbered rulers,
  visible origin and useful coordinate/zoom readouts.
- Left sidebar split vertically: expandable scene/resource tree above grouped
  property/value rows. Prefer this to the current opposite-side inspector layout.
- Compact top menu and domain tool groups, then contextual toolbars above viewport.
- Playback/reset/step and history controls along the bottom.
- Dark neutral theme, compact readable professional typography and icons; replace
  the current temporary SDL debug font as the UI matures.
- Resizable panels and display-scale-aware layout, keeping viewport input mapping
  correct. Keep actual functionality explicit; do not imply unimplemented domains
  work merely by drawing their tabs.

Current INT-003 remains a runnable first workspace, not the final design. Scope a
follow-up integration task for this layout/grid after review; do not silently change
domain contracts or mix collision/import work into visual layout changes.
