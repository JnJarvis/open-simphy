# INT-005 — Executable circular source mechanisms

Reviewed integration continuation, 2026-10-08, explicitly requested by user after
Newton Cradle comparison. This adds a narrow owning scene::Mechanism contract;
it does not replace the proposed full compound rigid-body model or particle APIs.
See spec/contracts/mechanism.md for exact units, limits and ports.

Affected-contract review: scene owns circle/constraint values and validation;
physics owns a private Box2D world and publishes owning samples; compat privately
runs bounded QuickJS initialization/actions and returns canonical mechanism values,
source visuals/widgets/assets; app owns SDL textures, input, playback/drag/reset.
Serialization/particle physics/renderer/editor contracts and native v1 stay intact.
Renderer's accepted Frame bytes are untouched: app overlays imported source views
through SDL, as existing source preview does. No new internal DAG edges:
compat->scene and physics->scene were already permitted. External dependencies
Box2D (MIT), QuickJS-NG (MIT), stb_image (MIT), pinned in the mechanics build helper.
Only OPENSIM_BUILD_COMPAT enables the experimental source-mechanics backend;
ordinary headless particle builds do not acquire these dependencies or enable C.

The source script, not filename recognition, produces copied balls/colors/joints.
The screenshot shows a retained original ball and five runtime copies: the initial
bridge defines World.clear as clearing runtime additions; clearAll is unsupported.
Saved DynamicallyAddedJoint records reference omitted runtime bodies and are not
used before script reconstruction. Neither interpretation is a blanket claim of
SimPHY API equivalence. Capability limitations remain visible and logged.

Scripts have bounded memory, stack, time, object and geometry counts; no std/os,
network, filesystem/module loading or native object handles. Unknown APIs fail
staged construction and keep source preview. Startup/button actions are supported;
collision script callbacks and sound are explicitly unavailable in this profile.
Play is labeled experimental: it exercises native circles/distance constraints,
not a claim of full source solver parity. All unknown nonmechanical domains,
non-circle/compound fixtures and unsupported joints stay source-only.
