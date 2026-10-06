"""Validate the synthetic provenance example and reject damaged records."""
import hashlib
import json
from pathlib import Path


def validate(record, root):
    for key in ("origin", "permission", "sha256", "version", "expected_result", "units"):
        if not isinstance(record.get(key), str) or not record[key].strip():
            raise ValueError(f"Missing {key}")
    if record.get("synthetic") is not True:
        raise ValueError("This sample must explicitly be synthetic")
    path = (root / record["file"]).resolve()
    if not path.is_relative_to(root.resolve()):
        raise ValueError("Fixture escapes metadata directory")
    if hashlib.sha256(path.read_bytes()).hexdigest() != record["sha256"]:
        raise ValueError("Fixture hash mismatch")
    if record.get("tolerances") != {"kind": "exact"}:
        raise ValueError("Sample requires exact comparison")


if __name__ == "__main__":
    root = Path(__file__).parent
    record = json.loads((root / "sample.metadata.json").read_text(encoding="utf-8"))
    validate(record, root)
    for key, bad in (("origin", ""), ("permission", ""), ("sha256", "0" * 64),
                     ("synthetic", False), ("file", "../outside.txt"),
                     ("tolerances", {})):
        altered = dict(record, **{key: bad})
        try:
            validate(altered, root)
        except ValueError:
            continue
        raise SystemExit(f"Invalid metadata accepted: {key}")
    print("Synthetic fixture metadata valid; six malformed records rejected")
