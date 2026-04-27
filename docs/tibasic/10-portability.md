# 10 : Portability and optimization

These two topics are tangled in TI-BASIC: most optimization patterns either rely on or affect portability. Treat them together.

## Calculator family compatibility

Five generations matter for TI-BASIC source compatibility:

| family            | RAM   | Flash | clock  | display | tokens |
|-------------------|-------|-------|--------|---------|--------|
| TI-83             | 27 KB | none  | 6 MHz  | mono    | base   |
| TI-83+            | 24 KB | 160 KB| 8 MHz  | mono    | base + archive + asm |
| TI-83+SE / 84+ / 84+SE | varies | varies | 15 MHz | mono | base + archive + asm + 84+ extras |
| TI-84+CSE         | varies | varies| 15 MHz | color   | base + 84+ extras + color extras |
| TI-84+CE          | varies | varies| 48 MHz | color   | full superset |

Differences worth knowing:

- **TI-83 has no Flash and no archive.** `Archive`, `UnArchive`, `GarbageCollect`, `Asm(`, `AsmComp(`, `AsmPrgm` do not exist. `SetUpEditor` is the portable list-equivalent of `UnArchive`.
- **TI-83 launches asm differently.** `Send(9prgmNAME)` on TI-83; `Asm(prgmNAME)` on 83+ and up. The same byte stream is `ERR:SYNTAX` on the wrong family.
- **OS 2.30+ on 84+ adds time / date and statistics commands** (`Manual-Fit`, `invT(`, `LinRegTInt`, `chi^2GOF-Test(`, `getTime`, `getDate`, ...).
- **OpenLib(` and `ExecLib`** are 84+ family only; primary user is `usb8x`.
- **Color models** add `BackgroundOn`, `TextColor(`, color-arg drawing variants. Programs containing those tokens are mono-incompatible.

## Character / token portability tiers

Three waves of new characters and tokens:

- **Tier 1 (TI-83+ baseline)**: lowercase letters, Greek letters, international glyphs.
- **Tier 2 (OS 1.15)**: `~ @ # $ & ; \ | _ %`. The `%` token also gained an undocumented "divide by 100" use.
- **Tier 3 (OS 1.16)**: `... angle ss x T <- -> up down integral sqrt`, plus subscripts 0-10.

A program containing a higher-tier character will not load on calculators below that line. The most common gotcha is lowercase letters: they are tier 1 but absent on the original TI-83.

## Undocumented behaviors

Worth knowing because they look like bugs:

- **Large font on the graph screen** (TI-83+ and up): `Text(-1, row, col, ...)` switches to large font. Documented as small-font-only in TI's manual; the `-1` first arg is the magic number.
- **Fast circle drawing** (mono only): `Circle(x, y, r, {i})` triggers an 8-fold-symmetry path that runs ~3x faster than the unspecified-color form. Color models do not implement this.
- **`%` token** (OS 1.15+): undocumented. `50%` divides by 100. Useful for clean percentage math.
- **`sub(` single-arg** (OS 1.15+): `sub(x)` divides by 100 if `x` is real / complex / list. A simpler form of the `%` undocumented op.
- **`Text(` erasing one row below** (84+ / SE small font): cosmetic, often invisible on a white background.

## Per-feature portability gates

| feature                               | minimum requirement |
|---------------------------------------|---------------------|
| `Archive` / `UnArchive` / `GarbageCollect` | TI-83+              |
| `Asm(` / `AsmComp(` / `AsmPrgm`       | TI-83+              |
| `Send(9prgmNAME)` (asm launch)        | TI-83 only          |
| Lowercase / Greek / international     | TI-83+              |
| `~ @ # $ & ; \ | _ %`                 | TI-83+, OS 1.15     |
| `... angle ss x T <- -> up down integral sqrt`, subscripts | TI-83+, OS 1.16 |
| Time / date commands                  | TI-84+ / 84+SE, OS 2.30 |
| `Manual-Fit`, `invT(`, `LinRegTInt`, `chi^2GOF-Test(` | OS 2.30 |
| `OpenLib(` / `ExecLib`                | TI-84+ family       |
| Color drawing args, `BackgroundOn` etc. | TI-84+CSE / CE       |
| `toString(`, `Wait`                   | TI-84+CE OS 5.2+    |
| `piecewise(`                          | TI-84+CE OS 5.3+    |
| `Asm84CEPrgm`                         | TI-84+CE OS 5.3.1+  |

## Optimization patterns

Optimization in TI-BASIC has two distinct goals: **size in tokens** and **execution time**. They sometimes conflict.

### Size

- **Drop closing tokens.** Trailing `"`, `)`, `}` before a newline or `->` are auto-closed by the parser. Each save: 1 token.
- **Use `Ans` to skip a store.** `5+3:Disp Ans` instead of `5+3->A:Disp A`.
- **Implicit multiplication.** `2A`, `pi r^2`. Saves the `*` token.
- **Combine boolean conditions into arithmetic.** `X+(C)->X` instead of `If C:X+1->X`. Saves several tokens; semantics differ slightly (assigns even when false). See [05-control-flow.md](05-control-flow.md).
- **Replace `If A=0` with `If not(A`.** Saves a token.
- **Use uppercase letters in displayed strings.** Lowercase letters are 2 bytes per char; uppercase are 1 byte. A "Hello" string is 10 bytes; "HELLO" is 5.
- **Use command tokens in strings instead of words.** Inserting the `If` token (1 byte) and a space inside a string costs the same as typing "If " in lowercase. The `or`, `and`, `If` tokens are particularly cheap.

### Speed

- **`If`/`Then` is faster than back-to-back `If`s** when the common condition is false.
- **`For(` is faster than `While` / `Repeat`** for known iteration counts (it's purpose-built).
- **Avoid `Goto` when label is far from program start.** `Goto` does a linear scan from byte 0 every time.
- **Drop `DelVar` of always-defined names.** `DelVar A` is wasted on `A` (it just resets to 0; reading `A` does the same).
- **Pull invariants out of loops.** Standard for any language.
- **Use `Pxl-` rather than `Pt-`** for pure pixel access on the graph screen (no Cartesian transform per call).
- **`Output(` with embedded numbers needs no `>String`** because `Output(` formats numbers itself.

### Speed cost of branching

The OS implementation makes specific patterns slow:
- `Goto` scans from byte 0 of the program. A 5KB program with `Lbl Z` near the bottom: every `Goto Z` is hundreds of token comparisons.
- `prgm` calls invoke the OS program-launch path; ~10x slower than an inline body.
- Large `If`/`Then`/.../`End` blocks that always fall through cost the conditional check; not measurable in normal programs.

### Speed cost of memory layout

- Variables in archive are not directly readable. Reading any expression involving an archived list raises `ERR:ARCHIVED`. Plan unarchive before access.
- `GarbageCollect` is **slow** (multi-second). Run it deliberately, not in a hot path.
- Storing into a list one element past the end is allowed and cheap (extends `dim`). Storing further is `ERR:INVALID DIM`.

## Patterns that cost more than they save

- **`DelVar Ans`**: the variable `Ans` is special; this can clear it but is rarely useful (next expression sets `Ans` anyway).
- **Replacing `=` with `not(!=`**: smaller in tokens (the `!=` token is 1 byte vs `=` 1 byte); functionally identical; less readable. Marginal.
- **Math-logic substitution (`AB` for `A and B`)**: saves a token but is slower because the result is computed numerically before the `If` test (no short-circuit).

## Programmer indicators (the busy / pause / RAM dots)

The TI-83/84 OS shows little indicators in the upper-right of the screen during program execution:
- **Vertical bar / dot** moving downward: program executing.
- **Static checker pattern**: program paused on `Pause`.
- **Hourglass-style indicator**: graphing or other long-running OS op.
- **`PAUSE` text**: 84+CE shows this explicitly while at `Pause`.

These are useful debug signals: if your program is "stuck" with no indicator moving, it has hit `getKey` polling (silent loop). The right column of the graph screen is reserved for this on monochrome models.

## Recommended program scaffolding for portability

A program targeting "TI-83+ family, mono, OS 1.15+" :

```
:StoreGDB 1                  // save graph context
:ClrHome
:...                         // initialization

:Lbl M                       // main loop start (or Repeat-based)
:getKey->K
:If K=45:Goto Q              // MODE -> exit
:...
:Goto M

:Lbl Q                       // cleanup
:RecallGDB 1
:DelVar GDB1
:DelVar K
:ClrHome
:Return
```

Restore graph context, clean up named vars, clear the home screen. This is a courtesy to the user (next program runs in a known state) and avoids the appearance of bugs (leftover plots, etc.).
