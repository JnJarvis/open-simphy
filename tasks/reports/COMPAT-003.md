# COMPAT-003 local inspection report

Worker codex-worker; branch codex/compat-003; local commit
9d098024f1effa6152395115008947d1fd036a54. Not pushed or merged.

Read the supplied research and independently inspected67 installed SSIM archives
without extraction or code execution. All67 passed ZIP member/CRC/XML inspection;
all67 SHA-256 hashes match the previous manifest. Versions57x4.0,3x4.1,7x4.2.
Ten synthetic utility tests pass. Initial backslash-name test needed literal-byte
mutation because Windows zipfile normalized the constructed name; fixed and rerun.
No C++ importer or runtime simulation compatibility test exists yet.

Commands: python -m unittest discover -s research/tools -p test_inspect_ssim.py;
python research/tools/inspect_ssim.py <installed simulations directory> --output
research/evidence/ssim-inspection.json. Staged whitespace passed. Detailed provenance,
limits and findings in the worker's research/SSIM_INSPECTION.md.

Changed paths: research/tools/inspect_ssim.py, research/tools/test_inspect_ssim.py,
research/evidence/ssim-inspection.json, research/SSIM_INSPECTION.md. All allowed.
No original samples/assets/XML/proprietary binaries committed. Report metadata
includes names, hashes, sizes and element inventory, not permission to redistribute.

Automatic approval review rejected public push of sample-derived metadata without
explicit user authorization. Local commit succeeded. Publication awaits user
approval for this payload to JnJarvis/open-simphy; do not retry without it.
Separate approved IO-002/PLAT-001 merges and non-evidence statuses were published.

Follow-up: review evidence, complete intake normalization prerequisites, then scope
archive/XML parser and loss-report/translation contracts. Container parsing alone
cannot establish script/asset/rigid-body/3D/circuit behavior or full format coverage.

User explicitly approved publication and all work; evidence pushed and merged as
ee1ea9509961bf2af690f877d8eb400939f52836. Task DONE. Prior publication blocker resolved.
