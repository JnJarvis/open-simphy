# CORE-002 implementation report

Owner: codex-worker. Branch: codex/core-002. Commit: 0e5c46d.
Status: DONE; native CI passed; user approved and implementation merged.

Changed paths: src/core/CMakeLists.txt, src/core/README.md,
src/core/include/opensim/core/values.hpp, tests/unit/core/{CMakeLists.txt,header.cpp,
values.cpp}, tests/contract/core/{CMakeLists.txt,values.cpp}. Scope unchanged.

Implements core v1: uint64 identity, per-document monotonic allocation, exhaustion
without wrap, owned diagnostics and explicit Result<T>/Result<void> branches.
Conformance tests cover C01-C14; unit tests check every stable code spelling and
owned message/path copies. Isolated header translation unit includes only the
public header. No external or internal module dependencies, OS APIs or globals.
Duplicate reservation is allowed; actual entity duplicate rejection belongs to scene.

Windows MSVC 19.44 / CMake 4.4.2 / Ninja 1.13, warnings as errors:
`cmake --preset headless-debug -DOPENSIM_WARNINGS_AS_ERRORS=ON
-DOPENSIM_CATCH2_SOURCE_DIR=<previously hash-verified Catch2 3.7.1 source>`;
`cmake --build --preset headless-debug --parallel 3`;
`ctest --preset headless-debug --output-on-failure`: PASS, 15/15.
Formatting with clang-format 18.1.8 passed before final string-conversion adjustment;
the adjusted file was formatted with that same executable. Staged whitespace check passed.

Initial link failed because the bundled Catch2 build did not provide string_view
diagnostic formatting. Tests now convert code names to owned strings for display;
the production API and exact expected strings are unchanged. Full suite rerun passed.

Result<T> supports copy/move construction but deliberately deletes assignment to
prevent throwing payload replacement from leaving a valueless variant. This API
choice is documented for review. Inactive accessors return null; returned pointers
are borrowed. UTF-8 text is supplied by producers; no encoding validator added.
No simulator or scene validation is implemented. Follow-up proposal: build owner
should evaluate Catch2 feature configuration before other tests need string_view
formatting directly. No build policy was changed in this task.

Native CI run https://github.com/JnJarvis/open-simphy/actions/runs/37530520508
completed successfully at 0e5c46d: eight MSVC/GCC/Clang/AppleClang Debug/Release jobs.
This includes formatting, full tests and tests-disabled configuration/build.

Independent reviewer: user. Approval: "looks good" in response to the explicit
request to approve both core and math. Merge: 6d72e5d72e09ec040c9ab7168f40ca46038d2865.
This supersedes pending-review statements above.
