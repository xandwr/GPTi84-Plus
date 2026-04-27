# 09 : Error catalog

When the interpreter raises an error, it stops the program and shows a menu:
- `1: Quit` : abandon program, return to home screen.
- `2: Goto` : jump cursor to the offending line in the program editor (sometimes hidden; see "no Goto" notes below).

The error name is shown as `ERR:NAME`. The full set follows alphabetically. Trigger conditions are summarized; see the upstream tibasicdev page for the corner cases of each.

## Errors silently suppressed during graphing

When the OS is rendering a graph (`DrawF`, `DrawInv`, `Tangent(`, `Shade(`, or function-mode plotting), the following errors **do not** raise; the offending point is just treated as undefined and skipped:
- `DATA TYPE`
- `DIVIDE BY 0`
- `DOMAIN`
- `INCREMENT`
- `NONREAL ANS`
- `OVERFLOW`
- `SINGULAR MAT`

`Tangent(` is a partial exception: an error at the tangent point itself does raise.

## Phantom errors (in OS, never observed live)

The following error strings exist in OS images but no normal-use trigger is known. Asm programs and Flash apps may use them:
- `ERR:SOLVER`
- `ERR:SCALE`
- `ERR:NO MODE`
- `ERR:LENGTH`
- `ERR:APPLICATION`

## Catalog

### A

**ARCHIVED**
- Use, edit, or delete an archived variable. `dim(L1)` if `L1` is archived raises this.
- Fix: `UnArchive` first. For lists, prefer `SetUpEditor` (works on TI-83 too).
- Programs cannot be archived from pure TI-BASIC; use an asm utility.
- The `2:Goto` option is hidden when this fires.

**ARCHIVE FULL**
- Tried to archive a variable; not enough Flash. Sometimes appears when answering "No" to a "Garbage Collect?" prompt.

**ARGUMENT**
- Wrong number of args to a function or instruction.
- A variadic function got 256+ args.
- If a function has both this and a SYNTAX problem, this surfaces first.

### B

**BAD ADDRESS**
- Send / receive of a Flash app saw a transmission error.

**BAD GUESS**
- `solve(`, equation solver, or CALC operation got a guess outside the bounds, or the function is undefined at the guess.

**BOUND**
- CALC operation or `Select(` defined `LeftBound > RightBound`.
- `fMin(`, `fMax(`, `solve(`, equation solver: lower bound must be `<` upper.

**BREAK**
- User pressed `[ON]` to abort. Affects programs, draw operations, and home-screen evaluations alike.

### D

**DATA TYPE**
- Wrong data kind passed to a function or instruction.
- Implicit multiplication where one operand is a wrong kind.
- Editor input of an invalid kind (matrix into stat list, etc.).
- Stored a wrong kind into a name (matrix into a list slot).
- Tried to divide matrices.
- Suppressed during graphing.

**DATE** (84+ / 84+SE only)
- Invalid date supplied to a date command. The error menu appends an explanation ("Invalid day for month selected.").

**DIM MISMATCH**
- Binary op on two lists / matrices with incompatible dims.
- Exceptions: matrix `*` requires `cols(A) == rows(B)`; `augment(` only requires equal row counts; `List>matr(` zero-pads short lists.

**DIVIDE BY 0**
- Self-explanatory. Linear regression on a vertical-line dataset also raises this.
- Suppressed during graphing.

**DOMAIN**
- Argument outside a function's defined range.
- Bad regression input (negative `X` for log/power, negative `Y` for exp/power).
- Bad finance input (`pmt2 < pmt1` for `>Prn(` / `>Int(`).
- Sequential graphing: `n` < 0 or non-integer for `nMin` / `nMax`.
- Suppressed during graphing.

**DUPLICATE**
- Tried to create a duplicate group name, or a duplicate program via `AsmComp(`.

**DUPLICATE NAME**
- Sending a variable; a name collision in the receiver. Receiver-side menu offers Omit / Rename / Overwrite.
- Also fires when unpacking a group with a colliding member.

### E

**EXPIRED**
- Trial-period Flash app expired.

**ERROR IN XMIT**
- Cable issue, [ON] during transmit, or model-incompatible transmission (e.g., 83+ trying to back up to 82).
- `2:Goto` redirects to the LINK screen, not the home screen.

### I

**ID NOT FOUND**
- `SendID` could not find the calc's ID block.
- `2:Goto` is hidden.

**ILLEGAL NEST**
- Banned recursive use of an argument-taking function: `seq(` inside `seq(`'s expression, `fnInt(` inside `fnInt(`'s first arg, `expr(` inside its own string arg, `Sigma(` inside `Sigma(`.

**INCREMENT**
- `seq(` increment is 0 or wrong-signed.
- `For(` increment is 0.
- Suppressed during graphing.

**INVALID**
- Bad variable / function reference. `Yn` cannot reference `Y`, `Xmin`, `dX`, `TblStart`.
- Reference to a variable transferred from TI-82 that's not valid on 83+.
- Sequence mode: phase plot without both equations defined.
- Sequence mode: recursive without enough initial conditions.
- Sequence mode: referencing `n-3` or earlier.
- Graph style invalid for current mode.
- `Select(` without an active `xyLine` / scatter plot.
- Used certain control tokens (`If`, `Then`, `Else`) **outside a program**.

**INVALID DIM**
- List / matrix index past `dim` (one past `dim` is allowed for list append).
- List `dim` not an integer in `[1, 999]`.
- Matrix `dim` not in `[1, 99]`.
- Tried to invert a non-square matrix.

**ITERATIONS**
- `solve(` / equation solver exceeded iteration cap. Examine the function's graph; tighten bounds or change the guess.
- `irr(` exceeded cap.
- `I%` exceeded cap.

### L

**LABEL**
- `Goto` target has no matching `Lbl`.
- `2:Goto` hidden.

**LINK**
- A Flash app encountered an invalid state during execution.
- `2:Goto` hidden.

### M

**MEMORY**
- RAM (or Flash, in archive context) exhausted.
- Try archiving non-essential vars.
- Recursive equation: `Y1 = Y1` raises this. (`Y1 = X` and `Y2 = Y1` does not.)
- The biggest cause: **`Goto` out of an unended loop / `If-Then` block** leaks the `End` bookkeeping (see [05-control-flow.md](05-control-flow.md)).

**MEMORYFULL**
- Receiver's RAM/Flash insufficient for transmitted item or full backup. Receiver shows the byte shortfall.

**MODE**
- Stored to a window var in the wrong graphing mode, or used a mode-specific instruction (e.g., `DrawInv` outside `Func`).

### N

**NO SIGN CHNG**
- `solve(` / equation solver did not detect a sign change in the search interval.
- `I%` computation when `FV`, `N*PMT`, and `PV` share sign.
- `irr(` when `CFList` and `CFO` share sign.

**NONREAL ANS**
- In `Real` mode, an op produced a complex result. Switch to `a+bi` or `re^thetai` to allow it.
- Suppressed during graphing.

### O

**OVERFLOW**
- Magnitude exceeds `1e100`.
- Reordering can sometimes help: `60!*30!/20!` overflows but `60!/20!*30!` does not.
- Suppressed during graphing.

### R

**RESERVED**
- Used a system var inappropriately, e.g., `1-Var Stats L.RESID`.

### S

**SINGULAR MAT**
- Singular matrix as `^-1`'s argument.
- Regression generated a singular matrix (no unique solution).
- Suppressed during graphing.

**SINGULARTY** (sic)
- `solve(` / equation solver hit a singularity in the function. Inspect the graph; adjust bounds.
- The on-calc spelling is "SINGULARTY"; do not "fix" it in error-text matching code.

**STAT**
- Bad list shapes for a stat command.
- Statistical analyses need >=2 data points; Med-Med needs >=3 per partition; frequency lists need elements >=0.
- Histogram requires `(Xmax - Xmin) / Xscl == 47`.

**STAT PLOT**
- Tried to display a graph while a plot using an undefined list is on. `PlotsOff` to clear.

**SYNTAX**
- Generic syntax problem.
- Also raised in place of `DATA TYPE` when the type is a "variable name" (with `seq(`, `solve(`, `For(`, etc.).
- `expr(` of a non-value string (space, equals sign, etc.).

### T

**TOL NOT MET**
- Numerical algorithm cannot meet the requested tolerance.

### U

**UNDEFINED**
- Read of a never-stored non-real variable. `A`-`Z`, `theta` default to 0 and never raise this.
- Lists, matrices, strings, pictures, GDBs require explicit storage first.
- Most system variables are always defined; stat variables are exceptions (defined only after the corresponding stat command runs).
- Equation slot variables raise `INVALID` instead of `UNDEFINED`.

### V

**VALIDATION**
- Link interference, or this calc is not authorized to run the given app / OS.

**VARIABLE**
- Tried to archive a variable that cannot be archived, or unarchive an app or group.
- Cannot be archived: `R`, `T`, `X`, `Y`, `theta`; `L.RESID`; system variables (stat / finance / equation / plot / window); `AppIdList`.

**VERSION**
- Tried to receive an incompatible variable version.
- Used / displayed corrupted tokens.

### W

**WINDOW RANGE**
- `Xmin > Xmax` or `Ymin > Ymax`.
- `Xmin == Xmax` (or numerically too close).
- `thetamin/max/step` produces empty or never-ending loop.
- `Tmin/max/step` similarly.
- `2:Goto` hidden.

### Z

**ZOOM**
- `ZBox` saw a degenerate (point or line) selection.
- A zoom op caused a math error.
- `2:Goto` hidden.

### Other

**?** : an unknown error. Rare. Surfaces only in odd OS / Flash app states.
