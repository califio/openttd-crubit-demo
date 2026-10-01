# Word navigation with a Rust compatibility binding

This demo replaces the word-segmentation backend of OpenTTD's existing text editor. It replaces word segmentation with ICU4X 2.2.0 and keeps character/grapheme movement on ICU4C.

## What ICU4X provides, and what the adapter adds

ICU4X's word-break iterator already supports forward iteration: `next()` yields the next boundary. It does not expose ICU4C's stateful, bidirectional cursor API. In particular, OpenTTD uses `following(cursor)` and `preceding(cursor)` to find a boundary after or before an arbitrary current position. It also uses forward/backward steps to skip whitespace.

The reusable Rust binding collects ICU4X's boundaries and owns the cursor state. It supplies the navigation operations expected by the C++ consumer:

| Operation | Implementation in Rust |
| --- | --- |
| Segmentation | ICU4X's `segment_utf16` iterator |
| `first()` | Reset the adapter's index to the first boundary |
| `following(pos)` | Find the first stored boundary strictly after `pos` |
| `preceding(pos)` | Find the last stored boundary strictly before `pos` |
| `next()` / `previous()` | Move through the stored boundary vector |

Forward segmentation is therefore available; reverse traversal and arbitrary-position navigation are the missing direct APIs for this integration. Recreating an ICU4X iterator can also restart forward iteration, so `first()` is a convenience for matching OpenTTD's existing interface, not a missing segmentation algorithm.

The related [ICU4X reverse-iteration request, #6996](https://github.com/unicode-org/icu4x/issues/6996), is open as of September 30, 2026. It requests `DoubleEndedIterator`, which would help reverse traversal but would not by itself supply the entire ICU4C cursor API.

## Files to review

- [Rust binding](rust/src/lib.rs): `WordIterator` owns the boundary vector and cursor index. It implements `set_text`, `first`, `following`, `preceding`, `next`, and `previous`. The API contains no OpenTTD-specific policy.
- [C++ forwarding shim](../word_adapter.cc) and [declarations](../word_adapter.h): delegate navigation to the generated Rust API. This isolates Crubit headers in a translation unit built without exceptions, converts `char16_t` input to `uint16_t`, and keeps recording counters outside the reusable binding. It holds no boundary vector or cursor index.
- [OpenTTD integration](../../src/string.cpp): backend selection, existing UTF-8/UTF-16 position mapping, and whitespace handling.
- [Differential test](../../src/tests/calif_word_demo.cpp): real OpenTTD `StringIterator` behavior at valid grapheme positions.
- [Recording hook](../../src/misc_gui.cpp): scripted navigation/deletion in the standard query dialog.

Through Corrosion, Crubit generates `build/corrosion_generated/crubit/icu4x_word_bindings/include/crubit/icu4x_word_bindings.h` (relative to the checkout root), including ownership, moves, destruction, and method calls for `WordIterator`. Its Rust-owned vector is private. The struct now represents a stateful compatibility API, rather than a wrapper needed to expose a vector.

OpenTTD's call pattern, UTF-8/UTF-16 position mapping, whitespace handling, and editing behavior are unchanged by this refactor. Missing cursor functionality is supplied in the shared Rust binding, where it can serve other consumers and potentially inform an ICU4X contribution. No change has been submitted upstream.

After returning `DONE` at the end of the text, the Rust cursor remains at the last boundary, matching ICU4C. This also fixes an edge-state difference in the earlier C++ adapter when reversing direction after exhaustion. Normal OpenTTD behavior remains covered by the existing differential tests.

## Build and run

Use the toolchain and CMake configuration from the [main README](../README.md). CMake uses Corrosion to build the Rust crate and generate its Crubit bindings:

```sh
cmake --build build --target openttd openttd_test -j 6
build/openttd_test '[word]'
python3 demo/record.py
```

The [finished demo GIF](../assets/openttd-icu4x-word.gif) is included in the repository. The recording script captures fresh PPM/PNG frames under `demo/output/word/icu4x/` and writes `demo/output/word/verification.txt`; it does not encode the GIF. Raw frames and logs are ignored by Git.

The script selects `CALIF_WORD_BACKEND=icu4x` and records only the Rust implementation. For interactive use, select `CALIF_WORD_BACKEND=icu4x` at launch and open a text/rename dialog; the scripted scene only runs with `null:render`. The backend selector is retained for automated comparisons with ICU4C and defaults to ICU4C when unset.

The original differential test covers **270 strings** through OpenTTD's `StringIterator`. A second test compares the Rust cursor directly with ICU4C after repeated forward/backward steps, exhaustion, seeks, empty input, and replacing the text. It also checks that destroying the caller's input buffer does not invalidate the cursor. Seek tests use UTF-16 code point boundaries, as OpenTTD does. The direct state test resets with `first()` after backward exhaustion; it does not reproduce ICU4C 78's cache-specific `next()` results after that state, which OpenTTD does not use.

Both word tests passed: **22,915 assertions**. The recording verifies that all **11 editing steps** were captured and exercised Rust, including Ctrl+Left/Right and Ctrl+Delete/Backspace. ICU4C comparisons remain in the automated tests. See the main README for commands to reproduce the results and save local logs. These checks do not establish equivalence for every ICU4C iterator state, input, or locale.

The C++ shim copies UTF-16 into a correctly typed input buffer; Rust stores all boundaries and reuses vector capacity. Cursor operations now cross the Crubit boundary. These costs need measurement alongside segmentation. Performance parity, coverage-guided fuzzing, and sanitizer coverage remain unverified.
