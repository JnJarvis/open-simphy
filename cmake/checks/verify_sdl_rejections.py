"""Offline negative checks; no download or SDL compilation needed."""
import pathlib
import subprocess
import sys
import tempfile

root = pathlib.Path(sys.argv[1])
with tempfile.TemporaryDirectory(prefix="opensim-sdl-invalid-") as temp:
    base = pathlib.Path(temp)
    fake = base / "wrong-version"
    (fake / "include" / "SDL3").mkdir(parents=True)
    (fake / "include" / "SDL3" / "SDL_version.h").write_text(
        "#define SDL_MAJOR_VERSION 3\n#define SDL_MINOR_VERSION 2\n#define SDL_MICRO_VERSION 0\n")
    (fake / "LICENSE.txt").write_text("Synthetic test metadata, not an SDL distribution")
    (fake / "CMakeLists.txt").write_text('message(FATAL_ERROR "must not configure fake SDL")')
    archive = base / "invalid.tar.gz"
    archive.write_bytes(b"synthetic invalid archive")
    cases = [(f"-DOPENSIM_SDL_SOURCE_DIR={fake}", "Offline source must be SDL 3.2.28"),
             (f"-DOPENSIM_SDL_ARCHIVE={archive}", "SDL archive SHA256 mismatch")]
    for index, (argument, expected) in enumerate(cases):
        result = subprocess.run([sys.argv[2], "-S", str(root / "cmake/checks/sdl_probe"),
            "-B", str(base / f"build-{index}"), "-G", "Ninja", argument],
            capture_output=True, text=True, timeout=60)
        if result.returncode == 0 or expected not in result.stdout + result.stderr:
            raise SystemExit(result.stdout + result.stderr)
    print("Wrong SDL version and archive hash rejected before acquisition")
