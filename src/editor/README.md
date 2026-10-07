# Headless particle editor

Implements accepted EDT-001 with owning State, checked View/picking, scene edits,
bounded history and baseline-relative drag transactions. Dependencies are only
core/math/scene. History restores content/selection while retaining session IDs.
App owns SDL, property widgets, coordinate normalization and simulation modes.
All test fixtures are synthetic SI values from E01-E30; no external assets.
