# INT-007 — Continuous circle contacts and responsive interaction

Affected-contract review before implementation: scene schemas and particle ports
are unchanged. Physics may refine world microstep duration using minimum circle
radius/current maximum speed, so fast dynamic pairs cannot skip whole contacts.
Bound work explicitly; excessive step budget fails without publishing a partial
snapshot. Keep sensor/mask/connected-joint collision exceptions explicit.
Relocate becomes a swept placement: it stops at the first enabled circle obstacle,
zeroes moved velocity, preserves paused time, and rejects an initially intersecting
new placement. This strengthens the private experimental mechanism port; it does
not change particle editing/history. SourceView reads the resulting publication.

App adds cursor-anchored camera zoom, right/middle mouse capture/pan and live body
dragging (other bodies continue playback). Ground-suspended dragging follows the
constraint arc in short segments, avoiding straight chord penetration. Focus loss
cancels capture. Paused dragging does not advance the clock or cause new overlap.
Preview navigation works independently of unsupported simulation capability.

Open remains SDL's platform dialog; release shared callback locks before dispatch,
start in the last opened local folder and measure dispatch time. Imported previews
as well as runnable mechanisms bypass covered particle rasterization. Paused editor
frames may be reused until events/state changes; no canonical renderer modification.
This does not claim OS shell/extensions/network-location cold startup is bounded.
