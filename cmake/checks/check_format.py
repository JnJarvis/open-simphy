"""Check only first-party C++ sources, never fetched dependency/build trees."""
from pathlib import Path
import subprocess

root = Path(__file__).resolve().parents[2]
files = sorted(
    path
    for folder in (root / "src", root / "tests", root / "cmake" / "checks")
    for path in folder.rglob("*")
    if path.suffix in {".cpp", ".hpp", ".h", ".cc"}
)
if not files:
    raise SystemExit("No first-party C++ sources found to check")
version = subprocess.check_output(["clang-format", "--version"], text=True)
if "version 18.1.8" not in version:
    raise SystemExit(f"Expected clang-format 18.1.8, got: {version.strip()}")
subprocess.run(
    ["clang-format", "--dry-run", "--Werror", *(str(path) for path in files)],
    check=True,
    cwd=root,
)
print(f"Formatting passed for {len(files)} first-party files")
