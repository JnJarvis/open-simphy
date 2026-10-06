# Fixture provenance

Every fixture needs origin, explicit license or permission, SHA-256 of exact bytes,
version, expected result, units and tolerances. Mark generated examples synthetic;
they establish no SimPHY compatibility. Unknown-rights binaries are prohibited.
Record evidence/requirement IDs for compatibility and defect task IDs for regression.
Keep fixtures small and immutable; justify changes to expected results independently
of current implementation output. Hash changes caused by line endings count too.

The executable example lives in tests/support/sample.metadata.json, with its data
beside it. Its validator is intentionally scoped to that exact sample, not a
production file schema. Future fixture owners must validate their own metadata.
