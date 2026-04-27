# 01 : Overview

TI-BASIC is the on-calc, interpreted, line-oriented programming language built into the TI-83 / 84 family. It is not a separate runtime: it is part of the OS. A "TI-BASIC program" is a named entity in the calculator's variable table, edited with the on-board program editor, executed by the OS interpreter, and stored as tokenized bytes.

## Execution model

- **Interpreted.** No compilation step. The program editor stores tokens; the OS dispatches on them at run time.
- **Single-threaded.** No concurrency primitive. The OS handles input events; the program does not.
- **Cooperative interrupt only via [ON].** Pressing the `ON` key raises `ERR:BREAK`, which is the only general-purpose escape.
- **Global state.** All variables (real, list, matrix, string, picture, GDB, system) are global to the calculator. Subprograms see and mutate the same names as their callers.
- **No first-class functions, no closures, no objects.** Function-shaped abstractions are built out of `prgm`, `Lbl`/`Goto`, and the `Y_n` equation slots (`Y1`, `Y2`, ..., `Y0`) which can hold expressions and be evaluated as a function of `X`.

## Source representation: tokenized, not ASCII

Every command, operator, variable, and most punctuation glyphs are stored as a single **token** : a numeric opcode the OS interprets directly. Two consequences matter:

1. **Program size is a function of token count, not character count.** A short identifier-heavy line can be larger on disk than a long line full of common single-byte tokens. Lowercase letters are 2 bytes each; the keyword `If` is one byte. (See [02-lexical.md](02-lexical.md).)
2. **There is no "syntax error from a typo" in the editor.** Every entry comes from a menu or a token-emitting key combo, so the editor admits only well-formed token streams. Errors surface at run time, not at edit time.

## File representation

On-calc objects are transferred to/from a host as TI variable files. The container format is the same across types; only the type byte and payload differ. Extensions on disk reflect the source calculator and the variable type:

| family | program | list | matrix | string | picture | OS image | Flash app |
|--------|---------|------|--------|--------|---------|----------|-----------|
| TI-83   | .83p   | .83l | .83m   | .83s   | .83i    | (none)   | (none)    |
| TI-83+ / TI-84+ / 84+SE | .8xp | .8xl | .8xm | .8xs | .8xi | .8xu | .8xk |
| TI-84+CSE | .8xp | .8xl | .8xm | .8xs | .8ci | .8cu | .8ck |
| TI-84+CE  | .8xp | .8xl | .8xm | .8xs | .8ci | .8eu | .8ek |

- A `.8xp` is a calculator program. Its body is the token stream the editor produced.
- A `.8xv` is an "AppVar" : an opaque named blob, used by Flash apps as a persistence container. App developers use `.8xv` because it survives `2nd MEM > Reset`.
- A `.8xu` is a signed OS image. Modifying it requires breaking the boot-time RSA chain (out of scope for TI-BASIC; relevant only to firmware work, see `docs/hackspire/`).

A complete file-extension table by family is reproduced in [10-portability.md](10-portability.md).

## What "the language" includes

Because TI-BASIC is bolted directly onto the OS, the language proper, the menu system, the math runtime, the graphing system, and the I/O primitives are not separable. The set of available commands varies with calculator model and OS version : new commands appear in OS releases, color-only commands appear on 84+CSE / 84+CE. A program written for the latest 84+CE will not run on a TI-83 even if it doesn't visibly use the color screen.

A useful mental partition of the surface:

- **Core control flow and expressions.** `If`/`Then`/`Else`/`End`, `For(`, `While`, `Repeat`, `Lbl`/`Goto`, expressions, store operator. Stable across the family.
- **Variable model.** Real, complex, list, matrix, string, picture, GDB, plus a category of "system variables" (window settings, stat regs, finance regs, equation slots).
- **I/O surface.** Home screen (Disp / Output( / Input / Prompt / Pause / Menu(), graph screen (Text, Pt-/Pxl- families, Line, Circle, Shade), and `getKey` polling.
- **Math library.** Arithmetic, trig, calculus (numeric `nDeriv`, `fnInt`, `solve`), probability, complex numbers, matrices, regressions, finance.
- **Calculator features as language.** `Archive`/`UnArchive`, `GarbageCollect`, link commands (`Send`/`Get`), date/time (84+ only), Asm bridge.

## What it is not

- Not Turing-complete in the strict sense without `Goto` (a flat program with only `If`/`For`/`While`/`Repeat`/`Repeat` is bounded by the loop guards), but trivially Turing-complete in practice : `Goto` plus arithmetic on lists is enough.
- Not memory-safe in any modern sense. Programs can corrupt list contents, leave dangling state in system variables, and exhaust RAM by leaking `End` entries from `Goto`-out-of-loop patterns. See [05-control-flow.md](05-control-flow.md) on memory leaks.
- Not lexically scoped. Every variable is global. Recursion is supported via `prgm` calling itself, but there is no per-call stack frame.

## Reading the rest of the spec

The numbered files build up from lexical structure to semantics, then list the surface area:

1. Lexical (tokens, encoding).
2. Types and variables.
3. Expressions and operators.
4. Control flow.
5. I/O surface (text I/O and getKey).
6. Graphics.
7. Math library.
8. Errors.
9. Portability and optimization.
10. Alphabetical command index.
11. Glossary.

If you only need to look something up, [11-command-index.md](11-command-index.md) and [09-errors.md](09-errors.md) are the index pages.
