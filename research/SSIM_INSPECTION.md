# Real SSIM archive inspection — 2026-10-07

The existing FILE_FORMAT.md already described ZIP plus simulation.xml, but its
archive evidence was a supplied manifest. This task independently reads the actual
67 bundled archives in the user's installed SimPHY app/libs/simulations directory.
All67 archive SHA-256 hashes match the supplied archive_manifest.json records.
No original archive, asset, XML payload or proprietary binary is added to Git.
Redistribution permission for originals remains unestablished; metadata only.

## Observed results

-67/67 passed bounded ZIP member streaming/CRC and XML structure inspection.
-Every archive has one simulation.xml at the archive root.
-XML root versions:57 at4.0,3 at4.1,7 at4.2. These are serialized version attributes,
  not independently verified application release identifiers.
-59 contain at least one Script element; contents were not executed. This count
  does not establish that all59 contain nonempty or necessary scripts.
-Largest archive:14743729 bytes. Largest simulation.xml:201895 bytes.
-Asset names, compressed/uncompressed sizes, compression type, SHA-256 hashes,
  top-level sections and element counts are in evidence/ssim-inspection.json.
-Examples include 3D Physics Demo/3D car Racing Demo.ssim with three members and
  3D Physics Demo/3D Joints Demo.ssim with two. These cannot be treated as the
  current free-particle scene just because the ZIP/XML can be read.

This is **container/XML evidence, not successful simulation import or parity**.
No scene translation, asset decoding, external reference fetch, script execution,
physics equivalence or native .osim conversion was attempted. Opening real SSIM
requires scoped parser/translation contracts and support for the represented
features; unsupported required data must produce diagnostics rather than vanish.
The broader goal includes .sim and every other supported SimPHY input format,
but this corpus establishes .ssim evidence only; do not assume .sim is an alias.

## Reproduce

```text
python -m unittest discover -s research/tools -p test_inspect_ssim.py
python research/tools/inspect_ssim.py <installed-simulations-directory> --output <report.json>
```

Ten synthetic tests passed. The first run exposed Windows zipfile name normalization
in the backslash test fixture; the corrected fixture mutates literal archive-name
bytes so the intended malformed path is actually tested. Inspection limits are
explicit in the utility:64 MiB compressed input,128 MiB declared total expansion,
4096 entries,1000:1 per-member ratio,8 MiB XML,128 XML depth,200000 elements.
Names/duplicates/links/encryption/compression are checked; DTD/entities are rejected.
Files are read-only and never extracted. This research utility is not a hardened
production archive parser or a replacement for future adversarial importer testing.

Public sample discovery was attempted at the [SimPHY download page](https://simphy.com/downloads)
and [author repository](https://github.com/maheshkurmi/simphy). The repository listing
contained README/images, no SSIM samples; the site advertises bundled examples and
login-based application downloads. The installed, hash-matching originals supplied
better direct evidence without acquiring another application or redistributing files.

COMPAT-001 templates and COMPAT-002 normalization/gates remain separate. This task
does not mark compatibility requirements PASS or claim the research gate satisfied.
