# 03 : Types and variables

TI-BASIC has no user-defined types and no type declarations. There are seven built-in variable kinds plus a fuzzy "system variables" category that the OS uses internally. The kind of a variable is implied by its name; `A` is real (or complex), `L1` is a list, `[A]` is a matrix, `Str0` is a string. Storing a value of the wrong kind into a name raises `ERR:DATA TYPE`.

## Variable kinds

| kind          | identifier form           | count        | element type             | notes |
|---------------|---------------------------|--------------|--------------------------|-------|
| real / complex | `A`-`Z`, `theta`          | 27 fixed     | one number               | implicitly initialized to 0 |
| list          | `L1`-`L6`, `L.NAME`       | 6 + N user   | up to 999 numbers (99 on TI-83) | resizable; can be archived |
| matrix        | `[A]`-`[J]`               | 10 fixed     | 2D array of numbers      | up to 99 rows, 99 cols; cannot be archived on TI-83 |
| string        | `Str0`-`Str9`             | 10 fixed     | sequence of tokens       | up to RAM-limited length |
| picture       | `Pic0`-`Pic9`             | 10 fixed     | one graph-screen image   | mono or color depending on model |
| GDB           | `GDB0`-`GDB9`             | 10 fixed     | graph database           | window + format flags + equation list |
| AppVar        | 1-8 char name             | unbounded    | opaque blob              | created by Flash apps / Asm; survives RAM clear |

The last (AppVar) is not creatable from TI-BASIC alone, but TI-BASIC programs can read and write to AppVars created by an accompanying Flash app or Asm utility.

## Real and complex numbers

- 14 digits of internal precision, 10 digits displayed and used for comparison.
- Real or complex per-variable; the same name can hold either kind in succession.
- Complex literals use `i` (imaginary unit, single token; never a variable name).
- Mode flag (`Real` / `a+bi` / `re^thetai`) controls the display form and gates `ERR:NONREAL ANS`. In `Real` mode, an operation that would yield a complex result raises `ERR:NONREAL ANS` instead. In `a+bi` or `re^thetai`, complex results are produced silently.

The 27 real-typed names (`A`-`Z`, `theta`) are always allocated. Reading one before storing returns 0; `DelVar A` makes `A` "deleted" but a subsequent read returns 0 again. The names `R`, `T`, `X`, `Y`, `theta` cannot be archived (they back the graphing system).

## Lists

```
:{1,2,3}->L1
:{4,5,6}->L.DATA
:L1+10->L2
```

- Six built-in names `L1`-`L6` (single tokens) plus user names. User names are `1`-`5` chars, first char a letter or `theta`, remaining chars letters / digits / `theta`. Reference user lists with the small-L prefix (`L.NAME` in this doc; on-calc, the small-L token).
- Element count cap: 999 (99 on TI-83). RAM is the practical limit.
- **Storing past the end is allowed** if the index is `dim(L)+1`: the list grows by one and the value is stored. Storing further than that raises `ERR:INVALID DIM`.
- **Vector broadcasting**: most numeric operations and functions accept lists and apply element-wise. With two lists of equal `dim`, operations are pairwise; with mismatched `dim`, `ERR:DIM MISMATCH`. With a scalar and a list, the scalar is broadcast.
- **Linked lists**: a list defined with a leading `"` (e.g. `"2L1->L2`) becomes a formula-linked list. `L2` recomputes when `L1` changes. Mutating `L2` directly breaks the link. Marker on-calc: a diamond on 84+, a small lock on 84+CE.
- **Archive interaction**: lists can be archived. While archived, they are not directly indexable. `ERR:ARCHIVED` on access. `SetUpEditor L.NAME` is the portable alternative to `UnArchive` (works on TI-83 too, and does not crash on undefined lists).

## Matrices

```
:[[1,2][3,4]]->[A]
:dim([A])->L1   // returns {2,2}
```

- 10 fixed names `[A]`-`[J]`.
- Rows up to 99, columns up to 99 (`ERR:INVALID DIM` past that).
- **Element-wise broadcasting** like lists for most numeric operations. Matrix multiplication is `*` between matrices when dims line up; matrix division is not defined : invert and multiply (or you get `ERR:DATA TYPE`).
- **Conversions** with `Matr>list(` and `List>matr(`. `List>matr(` zero-pads short lists.

## Strings

```
:"Hello"->Str1
:"World"->Str2
:Str1+" "+Str2->Str3   // "Hello World"
```

- 10 fixed names `Str0`-`Str9`.
- `+` is concatenation (overloaded for strings). `=` and `!=` test equality. No other operators apply to strings.
- **Each token in the string costs that token's byte size.** A string of lowercase ASCII is dense.
- Two characters cannot be inserted into a string from a TI-BASIC program: the literal `"` and `->`. Workarounds: render `"` as two apostrophes (`''`), render `->` as `-` followed by `>`. To get a literal `"` or `->` token into a string, use the home-screen `Equ>String(` trick documented in the Strings page upstream (relevant when distributing programs that need to contain those bytes).
- **`Input` accepts string-typed input** and will store typed-`"` characters into the destination string variable, which is the one in-program path to insert a literal `"`.
- The `expr(` command parses a string as an expression: `expr("5")` -> 5, `expr("{1,2,3}")` -> `{1,2,3}`. The reverse direction (number/list/matrix to string) has no built-in: convert character-by-character or via display routines.

## Pictures and GDBs

- **Picture** stores the exact pixel state of the graph screen. Mono pictures on monochrome calcs (96x64-ish bitmap), color pictures on 84+CSE / CE. Use `StorePic`, `RecallPic`. Pictures cannot be edited piecewise from TI-BASIC; they are opaque blobs to the interpreter.
- **GDB (Graph DataBase)** stores window settings and graph format flags : `Xmin`, `Xmax`, `Xres`, axis on/off, polar/sequential mode, the equation slots `Y1`-`Y0`, etc. Does not store the rendered pixels. Use `StoreGDB`, `RecallGDB`.

A common idiom is to `StoreGDB 1` at the top of a graph-using program and `RecallGDB 1` on exit, restoring the user's settings.

## System variables

A loose category for everything the OS exposes by name that isn't one of the seven kinds above. Includes:

- **Equation slots**: `Y1`-`Y0` (function), `r1`-`r6` (polar), `X1T`-`X6T` / `Y1T`-`Y6T` (parametric), `u`, `v`, `w` (sequence). Hold expressions; act as functions of `X` (or `T`, `theta`, `n`).
- **Window variables**: `Xmin`, `Xmax`, `Xscl`, `Ymin`, `Ymax`, `Yscl`, `Xres`; `Tmin`/`Tmax`/`Tstep`; `thetamin`/`thetamax`/`thetastep`; `nMin`/`nMax`/`PlotStart`/`PlotStep`. Real-typed.
- **Statistical variables**: `xbar`, `Sx`, `sigmax`, `n`, `r`, `r^2`, regression coefficients `a`, `b`, `c`, `d`, `e`. Populated by stat commands; reading before any stat command runs raises `ERR:UNDEFINED`.
- **Finance variables**: `N`, `I%`, `PV`, `PMT`, `FV`, `P/Y`, `C/Y`. Used by TVM solver and `tvm_*` family.
- **Graph format flags**: `AxesOn`/`AxesOff`, `LabelOn`/`LabelOff`, `GridOn`/`GridOff`, `CoordOn`/`CoordOff`, `Connected`/`Dot`, `Sequential`/`Simul`, `RectGC`/`PolarGC`, `ExprOn`/`ExprOff`. Each flag pair is a token; storing one sets the OS state.
- **`Ans`**: the last computed value. See [04-expressions.md](04-expressions.md).

System variables are mostly write-through to OS state, not "variables" in the program-memory sense. Many cannot be archived (`ERR:VARIABLE`). Many cannot be deleted with `DelVar`.

## Storage and lifetime

- **Storage**: the `->` operator. Cannot fail at the language level. Errors come from kind mismatch (`ERR:DATA TYPE`), dim mismatch (`ERR:INVALID DIM`), or memory exhaustion (`ERR:MEMORY`).
- **Initialization**: real variables `A`-`Z`, `theta` and most system variables are always defined (default 0 or a sane OS default). All other kinds raise `ERR:UNDEFINED` if read before being stored.
- **Deletion**: `DelVar name` removes the binding and reclaims memory. For `A`-`Z`, `theta`, the next read returns 0 again (the name is "default-defined"). `DelVar` does **not** accept a list/matrix element selector : `DelVar L1(3)` is `ERR:SYNTAX`.
- **Scope**: global. A subprogram (`prgm`) called from another program shares all variables.

## RAM, archive, and garbage collect

The TI-83+ and up have two memory regions:
- **RAM**: working memory. Programs run from RAM, variables they touch live here. Cleared on RAM-clear (intentional or after a crash).
- **Archive (Flash)**: long-term storage. Variables here are read-only to TI-BASIC: any access raises `ERR:ARCHIVED` until you `UnArchive` them.

Commands:
- `Archive var` : moves to Flash. Some types cannot be archived (`R`, `T`, `X`, `Y`, `theta`, `LRESID`, all system variables, AppIdList).
- `UnArchive var` : moves back to RAM. Equivalent to `SetUpEditor` for lists, except `SetUpEditor` is portable to TI-83 (which has no archive) and does not crash on undefined lists.
- `GarbageCollect` : compacts archive. Run periodically in long-lived workflows; otherwise the OS prompts when it must do this.

The TI-83 has no archive. Programs that use `Archive`, `UnArchive`, or `GarbageCollect` are not TI-83 portable.

## Type-coercion rules

There are no implicit conversions between kinds. Crossing kinds is explicit:

| direction | tool |
|-----------|------|
| string -> number / list / matrix | `expr("...")` |
| number -> string | no built-in; render via custom routine |
| list -> matrix | `List>matr(L1, ..., [M])` |
| matrix -> list | `Matr>list([M], col, L1, ...)` |
| equation slot -> string | `Equ>String(Y1, Str1)` |
| string -> equation slot | `String>Equ(Str1, Y1)` |

Numeric kinds (real, complex) coexist within a single name : storing complex into a real name promotes the variable to complex.

## Memory accounting summary

The relevant facts for sizing a program:
- Each variable kind has a per-variable header (a few bytes) plus payload.
- Real variables: fixed payload (one number).
- Lists: header + `8 * dim` bytes for real lists, `16 * dim` for complex lists (numbers are 9-byte BCD on disk; the in-RAM accounting differs but stays linear in `dim`).
- Strings: header + tokens.
- Programs: header + tokens.

Concrete byte counts vary slightly by OS version. For exact accounting, use the on-calc `2nd MEM > 2:Mem Mgmt/Del`, which reports the actual byte cost of each variable.
