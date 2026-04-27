---
source: tibasicdev.wikidot.com (Apr 2026 crawl), TI-83 Plus / TI-84 Plus family.
scope: Language reference for TI-BASIC as it runs on the monochrome 84+ (project's primary target). 84+CSE / 84+CE color-only commands are flagged where they appear.
purpose: A self-contained spec for the TI-BASIC dialect on the project's target calculators, so firmware/tooling work in this repo doesn't have to round-trip to the public wiki.
---

# TI-BASIC Language Reference

Local distillation of [tibasicdev.wikidot.com](http://tibasicdev.wikidot.com) into a single reference, scoped to the TI-83+ / TI-84+ / TI-84+SE family with notes flagging the 84+CSE and 84+CE color models. The TI-83 (no Flash) is treated as a portability constraint, not a primary target.

This is not a transcript. It is a synthesis aimed at someone working on a calculator-side runtime: precise enough to specify behavior, but skipping the tutorial framing of the source.

## Index

- [01-overview.md](01-overview.md) : what TI-BASIC is, on-calc tokenized model, source vs. binary, file extensions.
- [02-lexical.md](02-lexical.md) : character set, tokens (1 vs 2 byte), line structure, identifiers.
- [03-types-and-variables.md](03-types-and-variables.md) : the seven variable kinds, naming rules, system variables, RAM vs archive.
- [04-expressions.md](04-expressions.md) : operators, precedence, implicit multiplication, Ans, the store operator, list/matrix broadcasting.
- [05-control-flow.md](05-control-flow.md) : conditionals, loops, branching, IS>(/DS<(, subprograms, exit semantics.
- [06-io.md](06-io.md) : home-screen layout, Disp / Output(, Input / Prompt, Pause, Menu(, getKey and key codes.
- [07-graphics.md](07-graphics.md) : graph-screen geometry, Text, Pt-/Pxl- families, Line, Circle, Shade, GDB, Pic.
- [08-math-library.md](08-math-library.md) : math/trig/calculus/complex/probability functions; list and matrix automatic broadcasting.
- [09-errors.md](09-errors.md) : full ERR: catalog with trigger conditions.
- [10-portability.md](10-portability.md) : cross-model differences, optimization patterns, model/OS feature gates.
- [11-command-index.md](11-command-index.md) : alphabetical command listing with category tags.
- [12-glossary.md](12-glossary.md) : glossary of terms used throughout the spec.

## Conventions

TI-BASIC is fundamentally a tokenized language: programs on disk are sequences of opcodes, not ASCII text. To write the language in this document we use the following ASCII transliteration:

| token on calc          | written here as | notes |
|------------------------|-----------------|-------|
| store arrow (STO> key) | `->`            | the user-rule bans unicode arrows; on calc this is a single right-pointing arrow glyph |
| conversion pointer     | `>` (e.g. `>Frac`, `>DMS`) | TI prints these as a black right-pointing triangle |
| not-equal              | `!=` or kept as the math glyph where unambiguous | both forms appear in the wild |
| less-or-equal / greater-or-equal | `<=` / `>=` | both forms used |
| list-name marker (small L) | `L.` prefix or just bare list name | e.g. `L.NAME`; on-calc this is the small-L glyph |
| built-in lists L1-L6   | `L1`..`L6`      | on calc these are subscript glyphs (single tokens) |
| imaginary unit         | `i`             | always a single token, never a variable |
| theta                  | `theta`         | letter variable, distinct from `T` |
| pi / euler             | `pi` / `e`      | constants, single tokens |

Where on-calc syntax differs meaningfully from the ASCII transliteration (for example, `->` is a single byte, `STO>` typed as text isn't), the byte-level note appears in [02-lexical.md](02-lexical.md).

A leading colon `:` in code samples indicates a program line as it appears in the calculator's source view; it is not a typed character.

## What this spec is for in the project

`firmware/c` and `ti84_plus/` build artifacts that interoperate with on-calc programs. Knowing the exact set of tokens, the variable type system, the error surface, and the file extensions lets the host-side tooling produce / consume `.8xp` and friends without reverse-engineering the wiki on every change.

The spec stops at the boundary where calculator firmware (the OS, link protocol, flash format) takes over: see `docs/hackspire/` for that side of the system.
