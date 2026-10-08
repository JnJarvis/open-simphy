# COMPAT-004 — SSIM opening and rigid-body migration proposal

Worker: codex-worker. Coordinator: codex-coordinator. Date: 2026-10-08.
Branch: codex/compat-004. Submitted for affected-contract review; not implemented.

Delivered rfcs/COMPAT-004-ssim-opening.md and spec/ssim-delivery.md. Preserves
module boundaries and makes actual SSIM scene compatibility the next priority.
Specifies atomic opening, source preservation, bounded archive/XML/assets,
explicit preview/run states, required-feature diagnostics and no particle coercion.
Identifies shared scene/physics/collision/constraints/editor/renderer/native-format
changes and a delivery order toward playable 2D bodies, then joints and wider parity.

Evidence: reread COMPAT-003 results and supplied FILE_FORMAT/EVIDENCE_REVIEW;
parsed six local extracted XML files to check fixture shape/joint/script presence.
These observations establish syntax only, not numerical behavior or full scene
counts (compound/nested definitions need their own normalization).
Verified both document links and all cited evidence paths. git diff --check passed.
No runtime tests claimed: this task changes two Markdown documents only.

Limitations: no SSIM application loader or rigid-body solver was added by this task.
No parser dependency chosen without review; no original samples copied or scripts
executed. Broad COMPAT-001/002 research gates remain unchanged. Proposed shared
contract changes require review before implementation under repository rules.
Follow-ups and acceptance requirements are in spec/ssim-delivery.md.

Validation correction: extracted XMLs are local, untracked evidence in the canonical
checkout and are intentionally absent in the isolated worktree. Initial worktree
locator check failed on that absence; reran against canonical local evidence and
verified all locators. No fixtures were copied into Git. Proposal commit c62b896.

User explicitly continued the compatibility direction on 2026-10-08. Proposal
merged as a225171; task DONE. Existing domain contracts remain unchanged; detailed
rigid-body producer contracts still need affected-contract review before use.
INT-004 scopes the first real archive reader and application opening implementation.
