# 12 : Glossary

Terms used in this spec and the wider TI calculator community.

**`->` (store)** : the binary operator that assigns a computed value to a variable. Lowest precedence; one per statement. The single token produced by the calculator's `[STO>]` key.

**`Ans`** : single-token name for the most recently computed value. Typed in any expression context. Functions as a one-element implicit register.

**Archive** : the on-calc Flash region for long-term storage. Distinct from RAM. Contents survive RAM-clear. Inaccessible to TI-BASIC except by `UnArchive`-ing back to RAM. The TI-83 has no archive.

**AppVar** : opaque named blob managed by Flash apps. Created and read by asm / Flash apps; readable from TI-BASIC. Survives RAM-clear (lives in archive by default).

**Asm bridge** : the `Asm(`, `AsmComp(`, `AsmPrgm`, `Asm84CPrgm`, `Asm84CEPrgm` token family. Lets TI-BASIC programs invoke or contain Z80 (mono / CSE) or eZ80 (CE) machine code. `OpenLib(` / `ExecLib` reach into Flash app libraries.

**BCD** : binary-coded decimal. The internal numeric representation TI uses (9 bytes per real, ~14 digits precision, ~10 displayed). Affects the byte cost of lists and matrices.

**Boolean conditional** : the optimization pattern `X+(C)->X` standing in for `If C:X+1->X`. See [04-expressions.md](04-expressions.md), [05-control-flow.md](05-control-flow.md).

**Branching** : in TI-BASIC, the `Lbl` / `Goto` family. Generic "GOTO" jumps. Slow, leak-prone, but sometimes the only viable structure. See [05-control-flow.md](05-control-flow.md).

**Cartesian (graph) coordinates** : `(x, y)` real-number coordinates on the graph screen, mapped to pixels via `Xmin`, `Xmax`, `Ymin`, `Ymax`, `dX`, `dY`. Used by `Pt-`, `Line(`, `Circle(`.

**CSE / CE** : 84+CSE = TI-84 Plus C Silver Edition (color, eZ80 ish). 84+CE = TI-84 Plus CE (color, eZ80 explicitly, replaces CSE). 84+CE Python is a later sibling adding Python; not relevant to TI-BASIC.

**EOS (Equation Operating System)** : TI's name for the expression evaluator's precedence ladder. See [04-expressions.md](04-expressions.md).

**Equation slot** : one of `Y1`-`Y0`, `r1`-`r6`, `X1T`-`X6T`, `Y1T`-`Y6T`, `u`, `v`, `w`. Each holds an expression and is callable as a function (when the matching graphing mode is active).

**Flash** : the Flash ROM region on TI-83+ and up. Holds OS, Flash apps, and the archive.

**Flash app** : a signed Flash-resident program with menu integration. Distributed as `.8xk` or `.8ck` / `.8ek`. Distinct from a `.8xp` TI-BASIC program.

**GDB (Graph DataBase)** : opaque blob representing the current graph context (window, format flags, equation list). Saved with `StoreGDB`, restored with `RecallGDB`. Does not contain pixels; pair with `StorePic` if you want both.

**`getKey`** : non-blocking key-poll primitive. Returns the key code of the currently held key, or 0. The only async input mechanism.

**Implicit multiplication** : juxtaposition implies `*`. `2A` = `2*A`. Same precedence as explicit `*`.

**Linked list** : a list whose contents are recomputed from a formula stored at definition time. Created by storing a quoted expression: `"2L1->L2`. Marker: diamond on 84+, lock icon on 84+CE.

**Memory leak (TI-BASIC sense)** : the OS pushes an entry on its internal stack at the start of `For(`, `While`, `Repeat`, `If-Then`, `prgm`, etc.; pops at the matching `End` / `Return`. A `Goto` out of the block skips the pop, leaving the entry until the program exits. Repeated leaks exhaust RAM and cause `ERR:MEMORY`.

**Mode flag** : a single token whose execution sets a piece of OS state. Examples: `Radian`, `Degree`, `Real`, `a+bi`, `re^thetai`, `Float`, `Fix n`, `AxesOff`, `Func`. Persists across program runs unless restored.

**Pixel coordinates** : integer `(row, col)` on the graph screen. 0-indexed, row-then-column. Used by `Pxl-`, `Text(`. Independent of window settings.

**`prgm`** : the token that calls another program as a subprogram. Followed by the program name token (the editor inserts both as one menu pick).

**Programmer indicator** : the moving symbol in the upper-right corner during program execution. See [10-portability.md](10-portability.md).

**RAM** : the volatile memory region. Contains running programs and writable variables. Cleared on RAM-clear / crash / `2nd MEM > 7:Reset`.

**Real variable** : one of `A`-`Z`, `theta`. Single tokens. Always allocated. Default value 0.

**Stat plot** : one of three on-calc stat-plot configurations (`Plot1`, `Plot2`, `Plot3`). Holds plot type, list refs, marker style. Tokens `PlotsOn` / `PlotsOff` toggle.

**System variable** : any name the OS uses internally and exposes to TI-BASIC. Window, equation slots, finance, statistical regs, format flags. See [03-types-and-variables.md](03-types-and-variables.md).

**Token** : the unit of TI-BASIC source storage. 1 or 2 bytes per token. The on-calc editor produces and consumes tokens, not characters.

**TVM (Time Value of Money)** : the finance equation `tvm_*` solvers compute over. Backed by the system variables `N`, `I%`, `PV`, `PMT`, `FV`, `P/Y`, `C/Y`, plus `Pmt_Bgn` / `Pmt_End` flag.

**Tokenization** : the host-side process of turning text source into the calculator's binary token stream. Required for any tool producing `.8xp`, `.83p`, `.8xv`, etc. Reverse: detokenization. Tools: TokenIDE, tilp, Cemetech's SourceCoder.

**Window variables** : `Xmin`, `Xmax`, `Xscl`, `Ymin`, `Ymax`, `Yscl`, `Xres`, plus parametric / polar / sequence siblings. Real-typed system variables. Set the Cartesian-to-pixel mapping for the graph screen.

**`Y1`-`Y0`** : ten function-graphing slots. Hold quoted expressions: `"X^2"->Y1`. Callable: `Y1(3)` evaluates `Y1` at `X=3`. The closest TI-BASIC has to user-defined functions.

**`Z*`** (zoom commands) : `ZStandard`, `ZSquare`, `ZDecimal`, `ZInteger`, `ZBox`, `ZTrig`, ... preset window-variable changes. Useful in graphing-program preambles.

**`.8xp`, `.8xv`, `.8xu`, `.8xk`** : TI-83+ family file extensions. Program, AppVar, OS image, Flash app respectively. See [01-overview.md](01-overview.md), [10-portability.md](10-portability.md).
