"""BUILD-004: native executable checks, not an interactive/high-DPI smoke test."""
import argparse
import hashlib
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

SDL_LICENSE_SHA256 = "97f35b302b361680ec1e891e95d2d52097bb95abff361434916d99dc1305f127"


def verify_layout(build, tests, license_hash=SDL_LICENSE_SHA256):
    cache = {}
    for line in (build / "CMakeCache.txt").read_text(encoding="utf-8").splitlines():
        if line and not line.startswith(("#", "//")) and "=" in line:
            key, value = line.split("=", 1)
            cache[key.split(":", 1)[0]] = value
    if cache.get("OPENSIM_BUILD_APP") != "ON" or cache.get("OPENSIM_BUILD_TESTS") != tests:
        raise ValueError("Unexpected app/tests configuration")
    binary = build / "src/app" / ("opensim_demo.exe" if os.name == "nt" else "opensim_demo")
    if not binary.is_file():
        raise ValueError("Native executable missing")
    notice = binary.parent / "SDL-LICENSE.txt"
    if not notice.is_file() or hashlib.sha256(notice.read_bytes()).hexdigest() != license_hash:
        raise ValueError("Pinned SDL license missing or modified")
    if tests == "OFF":
        if any(key.lower().lstrip("_").startswith(("python3_", "catch2_")) for key in cache):
            raise ValueError("Tests-off configuration discovered test dependencies")
        if any((build / "_deps").glob("catch2*")):
            raise ValueError("Tests-off configuration acquired Catch2")
    return binary.resolve()


def verify_failures(binary, runner=subprocess.run):
    env = dict(os.environ, SDL_VIDEODRIVER="dummy", SDL_RENDER_DRIVER="software")
    for option, message in (("--fail-window", "Create window: injected failure"),
                            ("--fail-texture", "Create texture: injected failure")):
        result = runner([str(binary), option], env=env, capture_output=True, text=True, timeout=30)
        if result.returncode != 1 or message not in result.stdout + result.stderr:
            raise ValueError(f"{option}: expected injected failure, got {result.returncode}: "
                             f"{result.stdout}{result.stderr}")


class VerificationTests(unittest.TestCase):
    def test_layout_rejects_missing_or_wrong_outputs_and_dependencies(self):
        with tempfile.TemporaryDirectory() as temp:
            build = Path(temp)
            folder = build / "src/app"
            folder.mkdir(parents=True)
            binary = folder / ("opensim_demo.exe" if os.name == "nt" else "opensim_demo")
            cache = build / "CMakeCache.txt"
            baseline = "OPENSIM_BUILD_APP:BOOL=ON\nOPENSIM_BUILD_TESTS:BOOL=OFF\n"
            cache.write_text(baseline, encoding="utf-8")
            expected = hashlib.sha256(b"synthetic notice").hexdigest()
            with self.assertRaises(ValueError):
                verify_layout(build, "OFF", expected)
            binary.write_bytes(b"synthetic executable, never run")
            with self.assertRaises(ValueError):
                verify_layout(build, "OFF", expected)
            notice = folder / "SDL-LICENSE.txt"
            notice.write_bytes(b"wrong notice")
            with self.assertRaises(ValueError):
                verify_layout(build, "OFF", expected)
            notice.write_bytes(b"synthetic notice")
            self.assertEqual(verify_layout(build, "OFF", expected), binary.resolve())
            for suffix in ("Python3_EXECUTABLE:FILEPATH=python\n", "Catch2_SOURCE_DIR:STATIC=deps\n"):
                cache.write_text(baseline + suffix, encoding="utf-8")
                with self.assertRaises(ValueError):
                    verify_layout(build, "OFF", expected)
            cache.write_text(baseline, encoding="utf-8")
            (build / "_deps/catch2-src").mkdir(parents=True)
            with self.assertRaises(ValueError):
                verify_layout(build, "OFF", expected)
            with self.assertRaises(ValueError):
                verify_layout(build, "ON", expected)

    def test_failure_check_requires_both_exit_and_diagnostic(self):
        def valid(args, **kwargs):
            self.assertEqual(kwargs["timeout"], 30)
            self.assertEqual(kwargs["env"]["SDL_VIDEODRIVER"], "dummy")
            message = "Create window: injected failure" if args[1] == "--fail-window" else "Create texture: injected failure"
            return subprocess.CompletedProcess(args, 1, "", message)
        verify_failures(Path("fixture"), valid)
        for code, text in ((0, "Create window: injected failure"), (1, "Unexpected startup failure")):
            def invalid(args, **kwargs):
                return subprocess.CompletedProcess(args, code, "", text)
            with self.assertRaises(ValueError):
                verify_failures(Path("fixture"), invalid)
        def timeout(args, **kwargs):
            raise subprocess.TimeoutExpired(args, kwargs["timeout"])
        with self.assertRaises(subprocess.TimeoutExpired):
            verify_failures(Path("fixture"), timeout)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("build", type=Path, nargs="?")
    parser.add_argument("--tests", choices=("ON", "OFF"))
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if args.self_test:
        unittest.main(argv=[__file__])
    elif args.build is None or args.tests is None:
        parser.error("build and --tests are required")
    else:
        verify_failures(verify_layout(args.build, args.tests))
        print("Native executable, pinned SDL notice and dummy-driver failure checks passed")
