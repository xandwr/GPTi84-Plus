# 08 : Math library

The TI-83 / 84 family ships with a substantial numerics library. Most of it is exposed to TI-BASIC. Almost every numeric function broadcasts over lists; many work over matrices too.

## Number operations

`MATH` menu, `NUM` submenu, plus the keyboard:
- `>Frac`, `>Dec` : display next answer as fraction or decimal.
- `iPart(n)` : integer part (truncation toward zero).
- `int(n)` : floor (toward negative infinity).
- `fPart(n)` : `n - iPart(n)`.
- `round(n[, digits])` : banker's-aware rounding to specified decimal places.
- `abs(n)` : absolute value (for complex, modulus).
- `min(a, b)`, `max(a, b)` : also accept lists.
- `lcm(a, b)`, `gcd(a, b)` : work pairwise on lists.
- `remainder(numerator, divisor)` : signed remainder (84+ feature).

## Powers, roots, logarithms

- `^` : binary exponent operator.
- `^2`, `^3`, `^-1` : single-token postfix power, square, cube, inverse (the calculator-keyboard tokens).
- `sqrt(`, `cubert(` : single-token square root and cube root.
- `xroot(` : the `nth-root` token. Used as `n xroot x` -> `x ^ (1/n)`.
- `e^(` : single-token exponential.
- `10^(` : single-token base-10 exponential.
- `ln(` : natural log.
- `log(` : base-10 log.
- `logBASE(value, base)` : arbitrary-base log (84+ feature).

## Trigonometry

Sensitive to mode (`Radian` vs `Degree`). Set `Radian` for portable results.

- `sin(`, `cos(`, `tan(` : trigonometric.
- `sin^-1(`, `cos^-1(`, `tan^-1(` : inverse trig (single tokens).
- `sinh(`, `cosh(`, `tanh(`, `sinh^-1(`, `cosh^-1(`, `tanh^-1(` : hyperbolic (CATALOG).
- `degree`, `radian` : postfix angle conversion (`60 degree` = 60 degrees in radian mode -> radians).
- `>DMS` : display in degrees / minutes / seconds.
- `R>Pr(x, y)`, `R>Ptheta(x, y)`, `P>Rx(r, t)`, `P>Ry(r, t)` : rectangular / polar conversions.

## Calculus (numeric only)

The TI-83/84 family does not ship with a CAS. Calculus commands are numeric.

- `nDeriv(expr, var, value[, eps])` : numeric derivative. Symmetric difference quotient.
- `fnInt(expr, var, lower, upper[, tol])` : numeric definite integral. Adaptive Simpson.
- `solve(expr, var, guess[, {lo, hi}])` : root finder. Looks for `expr = 0` near `guess`.
- `fMin(expr, var, lo, hi)`, `fMax(expr, var, lo, hi)` : numerical minimum / maximum on an interval.

These can fail subtly: `solve(` near a singularity returns `ERR:SINGULARTY` (sic, the on-calc spelling); near a zero of zero derivative it returns `ERR:NO SIGN CHNG`. `fnInt(` is sensitive to the integrand's smoothness; it can return very wrong numbers without erroring.

## Probability and random

- `rand` : uniform `[0, 1)`. Single token, no parens.
- `randInt(lo, hi[, n])` : integer in `[lo, hi]`, optionally a list of `n`.
- `randNorm(mean, stddev[, n])` : normal-distributed.
- `randBin(n, p[, samples])` : binomial samples.
- `randIntNoRep(lo, hi[, n])` : `n` distinct integers from `[lo, hi]`, sampled without replacement.
- `randM(rows, cols)` : random integer matrix.
- The seed is a real variable also called `rand`. Storing into `rand` reseeds: `42->rand`. Reading `rand` returns the next random number AND advances the seed.

## Combinatorics

- `nPr` : permutations, infix.
- `nCr` : combinations, infix.
- `!` : factorial, postfix. Accepts non-integers (gamma function on 0.5 increments).

## Complex numbers

- `i` : imaginary unit (single token, never a variable).
- `conj(z)` : complex conjugate.
- `real(z)`, `imag(z)`, `angle(z)`, `abs(z)` : projection / argument / modulus.
- `>Rect`, `>Polar` : display form.
- Mode flags `Real`, `a+bi`, `re^thetai` control whether complex values can be produced. In `Real` mode, an op that would yield complex raises `ERR:NONREAL ANS`.

Most numeric functions accept complex args. `sqrt(-1)` works in `a+bi` mode (returns `i`); raises in `Real` mode.

## Constants

- `pi` : single token.
- `e` : single token (the math constant; not the variable letter `E`).
- `theta`, `T`, `X`, `Y`, `R` : letter variables, but heavily used by graphing modes; see [03-types-and-variables.md](03-types-and-variables.md).

## List-specific functions

(Detail in [03-types-and-variables.md](03-types-and-variables.md). Listed here as math primitives.)
- `dim(L)` : length.
- `seq(expr, var, lo, hi[, step])` : list comprehension.
- `cumSum(L)` : cumulative sum, returns list.
- `dList(L)` : pairwise differences `L(i+1) - L(i)`.
- `sum(L[, lo, hi])`, `prod(L[, lo, hi])` : reduce.
- `mean(L[, freqList])`, `median(L[, freqList])`, `stdDev(L)`, `variance(L)`.
- `max(L)`, `min(L)`.
- `sortA(L)`, `sortD(L)` : in-place sort ascending / descending. Mutates the list.
- `Fill(value, L)` : fills all elements.
- `augment(L1, L2)` : concatenate.

## Matrix-specific functions

- `dim([A])` : returns `{rows, cols}` as a list.
- `rowSwap(`, `row+(`, `*row(`, `*row+(` : elementary row ops.
- `ref([A])`, `rref([A])` : row-echelon, reduced row-echelon.
- `det([A])`, `T` (suffix transpose), `^-1` (inverse).
- `randM(rows, cols)`.
- `identity(n)` : square identity matrix.

## Statistics

The stat commands are numerous; they live in `STAT` and `STAT CALC` menus. From a TI-BASIC perspective, the three useful traits:

1. They populate **stat system variables** (`xbar`, `n`, `Sx`, `r`, `r^2`, regression coefficients `a..e`). After running `1-Var Stats L1`, those variables are defined and readable.
2. They have **list-typed return values**: `LinReg(ax+b) L1, L2, Y1` stores the regression equation into `Y1` and updates the stat vars; the equation slot is then a callable function.
3. **Statistical analyses are list-broadcast on the input.** Matched argument shapes apply.

A program needing a curve fit reads stat vars after the appropriate regression command runs.

## Finance functions

- `tvm_FV`, `tvm_PV`, `tvm_I%`, `tvm_N`, `tvm_Pmt` : Time-value-of-money solvers.
- `bal(`, `nPr(...) ` and friends : amortization and present-value tools.
- `npv(`, `irr(` : NPV and IRR with cash flow lists.
- All read / write the finance system vars: `N`, `I%`, `PV`, `PMT`, `FV`, `P/Y`, `C/Y`, `PMT:END` / `PMT:BEGIN` flag.

Rarely used in games and tools; useful in stats / business programs and as a reference for the TVM equation.

## Date and time (84+ only)

The TI-84+/SE has a real-time clock. Tokens added in OS 2.30+:
- `getDate`, `getTime` : return as lists.
- `setDate(y, m, d)`, `setTime(h, m, s)`.
- `setDtFmt`, `setTmFmt`, `getDtFmt`, `getTmFmt`.
- `dayOfWk(y, m, d)`, `dbd(d1, d2)` : day-of-week, days-between.
- `timeCnv(seconds)` : seconds to `{d, h, m, s}` list.
- `startTmr`, `checkTmr(t)` : timer primitives. `startTmr` returns a "timer handle" (a unix-like seconds count); `checkTmr(handle)` returns elapsed seconds since.

These are absent on the TI-83+. Programs that use them are non-portable below 84+.

## Auto-broadcast cheat sheet

The behavior of "math function on a list":

- Single-arg functions (`sin`, `abs`, `sqrt`, `ln`, ...): apply to each element.
- Two-arg functions (`min`, `max`, `gcd`, `lcm`, `nPr`, `nCr`, `xroot`, ...): with list + scalar, scalar broadcasts; with list + list, pairwise (must be same dim).
- Calculus (`nDeriv`, `fnInt`, `solve`, `seq`): do **not** broadcast over lists; the list is the variable being summed / iterated.
- `rand`: each evaluation gives one number. `rand(n)` produces a list of length `n`. Same for `randInt`, `randNorm`, `randBin`.

`ERR:DIM MISMATCH` appears when a binary op gets two lists of different `dim`. `ERR:DATA TYPE` appears when a function gets the wrong kind (string into `sin`, etc.).
