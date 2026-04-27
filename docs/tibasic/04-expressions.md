# 04 : Expressions and operators

Expressions follow a strict precedence ladder enforced by the OS routine TI calls "EOS" (Equation Operating System). All operators are infix or prefix; there is no operator overloading exposed to the user (concatenation of strings via `+` is built in, not user-defined).

## Operator inventory

### Math

| op  | meaning           | notes |
|-----|-------------------|-------|
| `+` | add / concatenate | numeric add, list/matrix add (broadcast), string concat |
| `-` | subtract / negate | binary subtract; unary negation is a separate token (the `(-)` key, not the `-` key) |
| `*` | multiply          | scalar mul, matrix mul (when dims agree), broadcast |
| `/` | divide            | numeric divide; matrix division is not defined |
| `^` | power             | also `^-1` token for inverse |
| `xroot` | x'th root      | the `xsqrt(` token; x = a in `a xroot b` |
| `!`     | factorial      | postfix |
| `^2`, `^3` | square / cube | postfix tokens, single-byte |

The unary minus on the calc keyboard `(-)` is a different token from the binary `-`. They are distinct in the byte stream and have different precedence (see below). Mixing them up parses but produces the wrong result.

### Relational

`=`, `!=`, `>`, `>=`, `<`, `<=`. Each yields `1` (true) or `0` (false). The result is a real number; you can `+`, `*`, store it.

There is no separate boolean type. **Anywhere a boolean is expected, any nonzero value is true and 0 is false.**

### Logical

`and`, `or`, `xor`, `not(`. Treat all nonzero values as 1. Truth table:

| A | B | and | or | xor | not(A) |
|---|---|-----|----|----|--------|
| 0 | 0 |  0  | 0  | 0  | 1      |
| 0 | 1 |  0  | 1  | 1  | 1      |
| 1 | 0 |  0  | 1  | 1  | 0      |
| 1 | 1 |  1  | 1  | 0  | 0      |

`and` short-circuits: if the left operand is 0, the right is not evaluated. `or` does **not** short-circuit reliably (depends on OS version; do not rely on it).

`not(` is single-arg and **must close its paren** when followed by a non-trivial expression. The closing paren can be elided at end-of-line / before store, like every other paren on this calculator.

### Conversions and "pointer" tokens

Tokens that appear with a leading `>` glyph (printed as a black right-pointing triangle on calc; rendered `>` here):
- `>Frac`, `>Dec` : display next answer as fraction / decimal.
- `>DMS` : degrees-minutes-seconds.
- `>Polar`, `>Rect` : polar / rectangular display for complex numbers.
- `>n/d`, `>Un/d`, `>F<>D` : 84+ family fraction conversions.

These are **postfix display modifiers**, not operators in the normal sense. They affect how the result of an expression is shown but typically do not change the underlying value.

### The store operator: `->`

`expression -> destination` evaluates `expression` and writes the result into `destination`. `destination` is a variable name (or `Y1`, `[A](2,3)`, `L1(5)`, etc.).

- Right-associative is moot : there is only one `->` per statement (using two on a line is `ERR:SYNTAX`).
- `->` is the lowest-precedence operator. Everything to its left is one expression.
- `->` is also the line terminator for the auto-close behavior: an open `(` or `"` closes when `->` is seen.

## Precedence (priority levels)

From the EOS ladder, lowest number = highest precedence:

| level | tokens |
|-------|--------|
| 1 | prefix functions (`sin(`, `sqrt(`, `cos(`, ...) except negation |
| 2 | postfix functions (`^2`, `!`, ...) |
| 3 | `^`, `xroot` |
| 3.5 | unary negation (the `(-)` token) |
| 4 | `nPr`, `nCr` |
| 5 | `*`, `/`, **implicit multiplication** |
| 6 | `+`, `-` (binary) |
| 7 | relational: `=`, `!=`, `>`, `>=`, `<`, `<=` |
| 8 | `and` |
| 9 | `or`, `xor` |
| 10 | conversion tokens (`>Frac`, `>Dec`, ...) |
| -- | `->` (separate; effectively last) |

Within a level, evaluation is **left-to-right**.

Two consequences worth flagging:
- **Implicit multiplication is at the same level as explicit `*`.** `2A` is `2*A`. `2(A+1)` is `2*(A+1)`. There is no exception. This is unlike most programming languages and matches the printed-math convention.
- **Negation has a quirky level (3.5).** `-A^2` is `-(A^2) = -A^2`, not `(-A)^2`. Because negation is below `^` and `xroot` but above `*`, `/`. (Same as printed math.)

## Implicit multiplication

`A` next to `B` (with no operator between) means `A * B`. Allowed contexts:

```
2A         // 2 * A
2pi        // 2 * pi
A(B+1)     // A * (B+1)
sin(X)cos(X) // sin(X) * cos(X)
```

Disallowed contexts:
- `(A)(B)` is fine; `AB` is **two characters**, parsed as the variable name `A` then `B` only when each is a single-letter real. There is no two-letter variable name in TI-BASIC. So `AB` reads as `A * B` because each is a one-token variable.
- `XY` similarly is `X * Y`. Do not assume identifiers can be more than one letter (they can't).

## `Ans`

`Ans` is the last computed value, a single token. It is the only "register" the language exposes:

- After `Disp X+1`, `Ans` is `X+1`.
- After `5->A`, `Ans` is `5`.
- After `Disp "HI"`, `Ans` is `"HI"` (a string).
- After most commands that don't return a value (`ClrHome`, `Pause`, `Stop`), `Ans` retains its previous value.

`Ans` participates in expressions transparently:
```
:5
:Ans+3->A   // A = 8
```

`Ans` is **typed**; a string-valued `Ans` cannot be added to a number. A list-valued `Ans` propagates list-ness through subsequent arithmetic.

`Ans` is heavily used as an optimization: it avoids storing into a named variable, and on the home screen it makes terse one-liners possible. In programs, prefer named variables for clarity unless saving a token is critical.

## Auto-close

The interpreter closes any open `(` and `"` at:
- End of statement (newline or `:`).
- Before `->`.

This means `Disp "HI`, `sin(X+1`, and `"HELLO->Str1` are all syntactically complete. The closing tokens are not inserted in the byte stream; the parser handles the unclosed form.

Exceptions: closing parens **before** further expression content cannot be elided (the parser cannot guess where to close). `(A+B)*C` cannot be written `A+B*C` and have it mean the same thing : it parses as `A+(B*C)` per precedence.

## List and matrix arithmetic

Every numeric operator and most functions auto-broadcast over lists and matrices:

- `scalar op list` : applies `op` to scalar against each element.
- `list op list` : pairwise, requires equal `dim`, else `ERR:DIM MISMATCH`.
- `scalar op matrix`, `matrix op matrix` : same rules, plus matrix multiplication for `*`.

Single-argument functions (`sin`, `abs`, etc.) similarly map element-wise:
```
:cos({30,60,90})    // {cos(30),cos(60),cos(90)}
```

Comparison broadcasts too:
```
:{3,4,5}={6,7,5}    // {0,0,1}
```

This makes lists a very fast tool for batch computations and conditional masks (multiply by a comparison list to zero out non-matching elements).

## Calculus and equation-shaped expressions

Several "operator-like" tokens take an expression argument and expect it to be a function of an implicit variable:

- `nDeriv(expr, var, value)` : numeric derivative.
- `fnInt(expr, var, lower, upper)` : numeric definite integral.
- `solve(expr, var, guess[, {lo, hi}])` : root finder, expects `expr = 0`.
- `seq(expr, var, lo, hi[, step])` : list construction.

Inside these, the expression can use any variable already defined; the named `var` is the iteration / evaluation variable. The most common subtle error: using a named `var` that aliases something already in use (`X` is heavily reused).

## Sample expressions

```
:5+3->A                      // A = 8
:not(A)                      // 0
:A and B                     // 1 if both nonzero
:not(A or B)                 // De Morgan equivalent: not(A) and not(B)
:(A=1 and B=2) or (A=2 and B=1)
:A=1 and B=2 or A=2 and B=1  // same: and binds tighter than or
:{1,2,3}+{4,5,6}             // {5,7,9}
:{1,2,3}*2                   // {2,4,6}
:sin({0,30,60,90})           // four sines, returned as a list
:expr("3+4")                 // 7
```

## Optimization patterns

(Detailed in [10-portability.md](10-portability.md), summarized here:)

- **Boolean-conditional substitution.** Replace `If C:X+1->X` with `X+(C)->X`. Saves tokens and is faster when the condition is true; slightly slower when false.
- **Drop closing tokens.** `Disp "HI` instead of `Disp "HI"`. Saves a byte per omitted close.
- **Use `Ans` to avoid named storage.** `5+3:Disp Ans` instead of `5+3->A:Disp A`.
- **Implicit multiplication is free.** `2A` is shorter than `2*A`.
