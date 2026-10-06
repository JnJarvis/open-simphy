"""Verify a real failing Catch2 assertion retains numeric context."""
import subprocess
import sys

result = subprocess.run([sys.argv[1], "[.diagnostic-probe]", "--reporter", "console"],
                        capture_output=True, text=True, timeout=20)
output = result.stdout + result.stderr
required = ("actual_value := 2", "expected_value := 1", "absolute_tolerance := 0.125",
            "relative_tolerance := 0.0625", "failed")
if result.returncode != 42 or not all(item in output for item in required):
    raise SystemExit(f"Missing numeric failure evidence (exit {result.returncode}):\n{output}")
print("Expected failing assertion includes actual, expected and both tolerances")
