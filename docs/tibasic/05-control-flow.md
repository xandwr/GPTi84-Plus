# 05 : Control flow

TI-BASIC has three categories: conditionals, loops, and unstructured branching. The conditionals and loops use a stack-based `End` terminator; branching uses unconditional jumps to label markers anywhere in the program.

## Conditionals

### `If` (one-statement form)

```
:If condition
:command
```

The single statement immediately after the `If` line runs iff `condition` is nonzero. There is no `End` for this form. The "next statement" can be on the same line after `:` (`If A=1:Disp "ONE"`) or on the line after.

Common idiom: replace short `If`-guards with arithmetic (the "boolean conditional"):

```
:If C
:X+1->X
```
becomes
```
:X+(C)->X
```

This is faster when `C` is true (no branch), slightly slower when false (still does a multiply and a store), and smaller in tokens.

### `If`/`Then`/`Else`/`End`

```
:If condition
:Then
:command(s)
:Else      // optional
:command(s)
:End
```

The `Then` form is required when the body is more than one statement. `Else` is optional. `End` closes the construct and is mandatory.

Performance note: the `If`/`Then` form is roughly twice as fast as repeated `If`-only forms when the condition is false (because the parser jumps past the whole block on a false test). Worth knowing for hot loops.

### `IS>(` and `DS<(`

Two specialized one-line forms for "increment/decrement and skip-on-overflow":

```
:IS>(var, value)
:command_to_skip_on_overflow
```

`IS>(var, value)` increments `var` by 1, then **skips the next statement** if `var > value`. `DS<(var, value)` decrements by 1, skips if `var < value`. The compare is `>`/`<` (strict).

Useful for terse loops:
```
:0->A
:Lbl L
:Disp A
:IS>(A, 9)
:Goto L
```
prints `0` through `9` and falls through.

Caveat: `var` must already be defined before the call (`DelVar` resets it; an undefined real is read as 0 so this rarely matters in practice). These are not loops on their own : pair with `Goto` or place above a tight body.

## Loops

### `For(`

```
:For(var, start, end[, increment])
:command(s)
:End
```

- `var` is real (`A`-`Z`, `theta`).
- `var` is set to `start` at loop entry; not implicitly zero before that.
- After each iteration, `var += increment`. Default `increment` is 1.
- Loop ends when `var > end` (positive step) or `var < end` (negative step).
- **`increment` of 0 raises `ERR:INCREMENT`.**

To exit early, set `var > end` inside the body:
```
:For(A, 5, 100)
:110->A
:End      // exits after one iteration
```

Empty `For` is a delay primitive: `:For(X,1,200):End` produces a perceptible pause without `Pause`'s "press ENTER" behavior.

### `While`

```
:While condition
:command(s)
:End
```

Pre-test loop. Skips the body entirely if `condition` is false on entry. The body is responsible for changing whatever the condition tests; otherwise the loop is infinite (escape only via [ON]).

`While 1` is the canonical infinite loop.

### `Repeat`

```
:Repeat condition
:command(s)
:End
```

Post-test loop. Always runs the body at least once. Exits when `condition` is true (note: opposite sense from `While`).

`Repeat 0` is the canonical infinite loop. `Repeat 1` runs exactly once.

### Nesting

Any loop / conditional may nest in any other. Each open requires a matching `End`. Mismatched `End`s typically surface at run time with `ERR:SYNTAX` or, worse, run with surprising semantics.

## Branching: `Lbl` / `Goto`

```
:Lbl A
:...
:Goto A
```

Labels are 1 or 2 alphanumeric characters from `A`-`Z`, `0`-`9`, `theta`. One-character labels are smaller and faster.

Semantics:
- `Goto target` searches for `Lbl target` **from the start of the program** every time. There is no jump-target cache.
- Labels are program-local. `Goto` cannot jump into a different program (subprogram). `prgm` is the only inter-program transfer.
- Jumping into the middle of a `For(`/`While`/`Repeat`/`If-Then` block is legal at the language level but creates the most common TI-BASIC bug: the OS pushes loop / conditional bookkeeping onto an internal stack at the **opening** token and pops it at the matching `End`. **Jumping out of a loop with `Goto` skips the matching `End`, so the bookkeeping entry leaks.** This is the canonical TI-BASIC memory leak.

### Memory-leak rework patterns

To exit a loop on a condition without leaking, push the condition into the loop guard:

Bad (leaks):
```
:Repeat 0
:getKey->B
:If B:Goto A
:End
:Lbl A
```

Good:
```
:Repeat B
:getKey->B
:End
```

If the `Goto` is inside a `Then` block, restructure with separate `If` guards or use a sentinel variable:

```
:If A=1:Then
:3->A
:4->B
:Goto Q
:End
```
becomes
```
:DelVar C
:If A=1:Then
:3->A
:4->B
:pi->C
:End
:If C=pi
:Goto Q
```

The leak is **bounded by the program lifetime**: the OS releases the stale stack entries when the program exits. So a leaky program that runs to completion is wasteful but not corrupt; a long-running event-loop with `Goto`-based exits will eventually `ERR:MEMORY` (often the most common cause of that error).

### When `Goto` is appropriate

- Single end-of-program cleanup label (`Goto Q` from anywhere; `Lbl Q` near the bottom does `DelVar`s, restores GDB, returns).
- Tight inner branches in size-critical code where a `For` would cost more tokens than the manual jump.
- Recursive programs that cannot use `Repeat` without leaking : `Goto` has no per-iteration overhead.

`Goto` is slow when the label is far from the start of the program (linear search). Speed-sensitive code usually avoids it.

## Subprograms: `prgm`

```
:prgmCHILD
```

The `prgm` token followed (with no space; the editor inserts the program name token) by a program name calls that program as a subprogram. When the called program exits (end-of-file or `Return`), execution resumes after the call.

Properties:
- **All variables are global.** Caller and callee share state. Common pattern: dedicate a few real variables (`Z`, `theta`) as "argument registers" and document the calling convention.
- **`Lbl` / `Goto` are program-local.** A `Goto` in `CHILD` cannot target a label in the parent.
- **No call stack visibility.** You cannot inspect or unwind the stack from TI-BASIC.
- **Recursion works** but each level pushes onto the same OS stack used by `End`-balancing : deep recursion can `ERR:MEMORY`.
- **Stop in a subprogram exits the whole chain** (back to home screen). `Return` exits only the current program.

Naming convention used by the on-calc community: subprograms named `Z<parent><n>` or `theta<parent><n>` so they sort to the bottom of the program menu.

## Exiting a program

| token | effect |
|-------|--------|
| `Return` | exit current program; in subprogram, control returns to caller |
| `Stop` | exit current program **and all callers**; control returns to home screen |
| (end of file) | implicit `Return` |

`Return` on the last line is redundant. `Stop` is occasionally needed to terminate the entire chain in error paths; for normal exits, prefer `Return` for shell compatibility (some assembly shells like DCS dislike `Stop`).

The `[ON]` key, pressed during execution, raises `ERR:BREAK` and offers a "Goto" option (jump cursor to the failing line). This is the user-side abort path; programs cannot trap it.

## Boolean conventions in conditions

Any nonzero value is true. Use this to simplify:

```
:If A!=0       ->   :If A
:If not(A=0)   ->   :If A
:If A=0        ->   :If not(A
```

`not(A` saves one character over `A=0` and is one token less.

DeMorgan substitutions:
```
:not(A) and not(B)   ==   not(A or B)
:not(A) or  not(B)   ==   not(A and B)
```

Math-logic substitutions (`*` for `and`, `+` for `or`):
```
:If A and B    ->    :If AB
:If A or B     ->    :If A+B
```
Slower than the dedicated logic operators, smaller in tokens. Useful when token count is the binding constraint.
