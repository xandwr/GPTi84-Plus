# 11 : Command index

Alphabetical command listing for the TI-83+ / 84+ family. Includes 84+CE/CSE color-only commands flagged as `[CE]` or `[CSE]`. Source: TI-84 Plus C Silver Edition / TI-84 Plus CE alphabetical command index from tibasicdev (Apr 2026 crawl).

For monochrome 83+ / 84+ targets, ignore `[CE]` / `[CSE]` entries; they raise `ERR:VERSION` or simply don't exist on those calculators.

Categories in the rightmost column:
- `Math` : math, trig, calculus
- `Op` : operators
- `IO` : I/O surface (home / graph)
- `Var` : variable manipulation
- `Ctrl` : control flow
- `Stat` : statistics, regressions, distributions
- `Fin` : finance / TVM
- `Mem` : memory / archive
- `Mode` : mode flag (window / graph / display)
- `Link` : link / send / receive
- `Asm` : asm bridge
- `Time` : date / time (84+ only)
- `Sys` : system (rarely user-typed)

## Operators

| token | category | notes |
|-------|----------|-------|
| `+` `-` `*` `/` | Op | binary math |
| `^`             | Op | power |
| `xroot`         | Op | n'th root |
| `^-1` `^2` `^3` | Op | postfix power |
| `!`             | Op | factorial |
| `pi` `e` `i`    | Op | constants |
| `%` `r` `degree`| Op | postfix conversion |
| `=` `!=` `>` `>=` `<` `<=` | Op | relational |
| `->`            | Op | store |

## Numeric prefix tokens

| token | category | notes |
|-------|----------|-------|
| `^-1` `^2` `^3` | Math | one/square/cube |
| `sqrt(` `cubert(` | Math | square / cube root |
| `1-PropZInt(` `2-PropZInt(` `1-PropZTest(` `2-PropZTest(` | Stat | proportion intervals/tests |
| `2-SampFTest` `2-SampTInt` `2-SampTTest` `2-SampZInt(` `2-SampZTest(` | Stat | two-sample tests |
| `1-Var Stats` `2-Var Stats` | Stat | univariate / bivariate descriptive stats |

## A

| token | category | notes |
|-------|----------|-------|
| `abs(`        | Math | absolute value / modulus |
| `and`         | Op   | logical AND |
| `angle(`      | Math | complex argument |
| `ANOVA(`      | Stat | analysis of variance |
| `Ans`         | Var  | last answer |
| `Archive`     | Mem  | move to Flash |
| `Asm(`        | Asm  | run asm program (TI-83+ and up) |
| `AsmComp(`    | Asm  | compile asm hex to executable |
| `Asm84CPrgm`  | Asm  | mark as 84+CSE asm `[CSE]` |
| `Asm84CEPrgm` | Asm  | mark as 84+CE asm `[CE]` (OS 5.3.1+) |
| `augment(`    | Math | concatenate lists or matrices |
| `AUTO Answer` | Mode | answer-display mode |
| `AxesOff` `AxesOn` | Mode | graph axes visibility |
| `a+bi`        | Mode | complex display, rectangular |

## B

| token | category | notes |
|-------|----------|-------|
| `BackgroundOff` | IO  | clear background image `[CSE]/[CE]` |
| `BackgroundOn`  | IO  | set background image `[CSE]/[CE]` |
| `bal(`        | Fin  | amortization balance |
| `binomcdf(`   | Stat | binomial CDF |
| `binompdf(`   | Stat | binomial PMF |
| `BorderColor` | IO   | screen-border color `[CE]` |

## C

| token | category | notes |
|-------|----------|-------|
| `c/d` `cd`     | Math | continued fraction support `[CE]` |
| `Circle(`      | IO   | draw circle on graph screen |
| `chi^2cdf(` `chi^2pdf(` `chi^2-Test(` `chi^2GOF-Test(` | Stat | chi-squared family |
| `Clear Entries` | Mem | clears home-screen history |
| `ClockOff` `ClockOn` | Time | RTC enable / disable |
| `ClrAllLists`  | Mem  | zero all lists' dim |
| `ClrDraw`      | IO   | clear graph screen |
| `ClrHome`      | IO   | clear home screen |
| `ClrList`      | Mem  | zero one list's dim |
| `ClrTable`     | IO   | clear function-table cache |
| `conj(`        | Math | complex conjugate |
| `Connected` `Dot` `Dot-Thick` `Dot-Thin` | Mode | graph plot style |
| `cos(` `cos^-1(` `cosh(` `cosh^-1(` | Math | trig / hyperbolic |
| `CoordOff` `CoordOn` | Mode | x/y indicator on graph |
| `cumSum(`      | Math | cumulative sum on a list |

## D

| token | category | notes |
|-------|----------|-------|
| `dayOfWk(`     | Time | weekday number `[84+]` |
| `dbd(`         | Time | days between dates `[84+]` |
| `>Dec`         | Op   | display as decimal |
| `Degree`       | Mode | trig in degrees |
| `DelVar`       | Var  | delete variable |
| `DependAsk` `DependAuto` | Mode | function table dep mode |
| `det(`         | Math | matrix determinant |
| `DiagnosticOff` `DiagnosticOn` | Mode | regression r/r2 display |
| `dim(`         | Math | length / shape |
| `Disp`         | IO   | print to home screen |
| `DispGraph`    | IO   | switch to graph screen |
| `DispTable`    | IO   | switch to table view |
| `>DMS`         | Op   | display in degrees-min-sec |
| `DrawF`        | IO   | graph an expression |
| `DrawInv`      | IO   | graph an inverse |
| `DS<(`         | Ctrl | decrement-skip-on-less |

## E

| token | category | notes |
|-------|----------|-------|
| `e^(`          | Math | exponential |
| `Eng`          | Mode | engineering notation |
| `Else`         | Ctrl | conditional alternative |
| `End`          | Ctrl | block terminator |
| `Eq>String(`   | Var  | equation slot to string |
| `equ>string(`  | Var  | alias of above |
| `expr(`        | Var  | parse string as expression |
| `ExprOff` `ExprOn` | Mode | show/hide function expr in trace |

## F

| token | category | notes |
|-------|----------|-------|
| `Fill(`        | Var  | fill list/matrix with value |
| `Fix`          | Mode | fixed decimal places |
| `Float`        | Mode | floating decimals |
| `FnOff` `FnOn` | Mode | disable/enable function plotting |
| `For(`         | Ctrl | counted loop |
| `fMax(` `fMin(`| Math | numerical extremum on interval |
| `fnInt(`       | Math | numerical definite integral |
| `fPart(`       | Math | fractional part |
| `>Frac` `>F<>D`| Op   | display as fraction / fraction-decimal toggle |
| `Full` `Horiz` `G-T` | Mode | screen split mode |
| `Func`         | Mode | function graphing |

## G

| token | category | notes |
|-------|----------|-------|
| `GarbageCollect` | Mem | compact archive |
| `gcd(`         | Math | greatest common divisor |
| `geometcdf(` `geometpdf(` | Stat | geometric distribution |
| `Get(`         | Link | receive variable |
| `getDate` `getDtFmt` `getDtStr(` `getKey` `getTime` `getTmFmt` `getTmStr(` | Time / IO | RTC + key polling |
| `Goto`         | Ctrl | unconditional jump |
| `GraphColor(`  | IO   | set graph color `[CE]` |
| `GraphStyle(`  | Mode | per-equation plot style |
| `GridLine` `GridDot` `GridOff` `GridOn` | Mode | grid display |

## H

| token | category | notes |
|-------|----------|-------|
| `Histogram`    | Stat | plot type |
| `Horiz` `Horizontal` | IO | screen split / horizontal line |
| `hypergeocdf(` `hypergeopdf(` | Stat | hypergeometric `[CE]` |

## I

| token | category | notes |
|-------|----------|-------|
| `i`            | Math | imaginary unit |
| `If` `Then`    | Ctrl | conditional |
| `imag(`        | Math | imaginary part |
| `IndpntAsk` `IndpntAuto` | Mode | table independent mode |
| `Input`        | IO   | user input |
| `inString(`    | Math | substring search |
| `int(`         | Math | floor |
| `Sigma int(`   | Fin  | sum of interest payments (`>Int(`) |
| `invNorm(`     | Stat | inverse normal CDF |
| `invT(`        | Stat | inverse t CDF (84+ OS 2.30) |
| `iPart(`       | Math | integer part (truncate) |
| `IS>(`         | Ctrl | increment-skip-on-greater |
| `irr(`         | Fin  | internal rate of return |

## L

| token | category | notes |
|-------|----------|-------|
| `Label`        | Mode | graph axis labels |
| `LabelOff` `LabelOn` | Mode | toggle |
| `Lbl`          | Ctrl | label target |
| `lcm(`         | Math | least common multiple |
| `length(`      | Math | string length |
| `Line(`        | IO   | draw segment |
| `LinReg(a+bx)` `LinReg(ax+b)` `LinRegTInt` `LinRegTTest` | Stat | linear regression family |
| `dList(`       | Math | pairwise difference |
| `List>matr(`   | Var  | list to matrix |
| `ln(`          | Math | natural log |
| `LnReg`        | Stat | logarithmic regression |
| `log(`         | Math | base-10 log |
| `logBASE(`     | Math | arbitrary-base log (84+) |
| `Logistic`     | Stat | logistic regression |

## M

| token | category | notes |
|-------|----------|-------|
| `Manual-Fit`   | Stat | manual regression (84+ OS 2.30) |
| `MATHPRINT`    | Mode | pretty-printed math entry (84+ OS 2.55+) |
| `Matr>list(`   | Var  | matrix to list |
| `max(` `min(`  | Math | extremes |
| `mean(` `median(` | Stat | central tendency |
| `Med-Med`      | Stat | median-median regression |
| `Menu(`        | IO   | choice menu |
| `ModBoxplot`   | Stat | modified box plot |

## N

| token | category | notes |
|-------|----------|-------|
| `nCr` `nPr`    | Math | combinations / permutations |
| `n/d` `Un/d`   | Math | mixed number forms |
| `nDeriv(`      | Math | numerical derivative |
| `>n/d` `>Un/d` | Op   | display as mixed number |
| `>Nom(`        | Fin  | nominal interest rate |
| `Normal`       | Mode | normal display notation |
| `normalcdf(` `normalpdf(` | Stat | normal distribution |
| `NormProbPlot` | Stat | normal probability plot |
| `not(`         | Op   | logical NOT |
| `npv(`         | Fin  | net present value |

## O

| token | category | notes |
|-------|----------|-------|
| `OpenLib(`     | Asm  | load Flash library (84+) |
| `or` `xor`     | Op   | logical OR / XOR |
| `Output(`      | IO   | print at home-screen cell |

## P

| token | category | notes |
|-------|----------|-------|
| `Param`        | Mode | parametric graphing |
| `Pause`        | IO   | wait for ENTER |
| `piecewise(`   | Math | piecewise expr `[CE OS 5.3]` |
| `Plot1(` `Plot2(` `Plot3(` | Stat | stat plot definitions |
| `PlotsOff` `PlotsOn` | Mode | enable/disable plots |
| `Pmt_Bgn` `Pmt_End` | Fin | payment timing |
| `poissoncdf(` `poissonpdf(` | Stat | Poisson distribution |
| `Polar`        | Mode | polar graphing |
| `>Polar` `>Rect` | Op | complex display form |
| `PolarGC` `RectGC` | Mode | graph cursor coordinate type |
| `prgm`         | Ctrl | invoke subprogram |
| `Sigma Prn(`   | Fin  | sum of principal payments (`>Prn(`) |
| `prod(`        | Math | product of list elements |
| `Prompt`       | IO   | input variable(s) by name |
| `Pt-Change(` `Pt-Off(` `Pt-On(` | IO | Cartesian point ops |
| `PwrReg`       | Stat | power regression |
| `Pxl-Change(` `Pxl-Off(` `Pxl-On(` `pxl-Test(` | IO | pixel ops |
| `P>Rx(` `P>Ry(` | Math | polar to rect components |

## Q

| token | category | notes |
|-------|----------|-------|
| `QuadReg` `QuartReg` | Stat | polynomial regressions |

## R

| token | category | notes |
|-------|----------|-------|
| `Radian`       | Mode | trig in radians |
| `rand`         | Math | random uniform; also seed name |
| `randBin(`     | Math | binomial samples |
| `randInt(`     | Math | uniform integer |
| `randIntNoRep(`| Math | distinct integers (84+) |
| `randM(`       | Math | random matrix |
| `randNorm(`    | Math | normal samples |
| `Rcl`          | Mem  | recall variable contents inline |
| `re^thetai`    | Mode | complex display, polar |
| `Real`         | Mode | only real numbers |
| `real(`        | Math | real part |
| `RecallGDB`    | IO   | restore graph state |
| `RecallPic`    | IO   | restore graph image |
| `>Rect`        | Op   | display complex as a+bi |
| `RectGC`       | Mode | rectangular graph cursor |
| `ref(`         | Math | row-echelon form |
| `remainder(`   | Math | signed remainder (84+) |
| `Repeat`       | Ctrl | post-test loop |
| `Return`       | Ctrl | exit current program |
| `round(`       | Math | round to digits |
| `*row(` `row+(` `*row+(` `rowSwap(` | Math | matrix elementary row ops |
| `rref(`        | Math | reduced row echelon |
| `R>Pr(` `R>Ptheta(` | Math | rect to polar |

## S

| token | category | notes |
|-------|----------|-------|
| `Scatter`      | Stat | scatter plot |
| `Sci`          | Mode | scientific notation |
| `Select(`      | Stat | select stat plot subset |
| `Send(`        | Link | transmit variables |
| `seq(`         | Math | list-by-formula |
| `Seq`          | Mode | sequence graphing |
| `Sequential`   | Mode | sequential graph drawing |
| `setDate(` `setDtFmt(` `setTime(` `setTmFmt(` | Time | RTC writes |
| `SetUpEditor`  | Mem  | initialize / unarchive list (portable) |
| `Shade(`       | IO   | shade graph region |
| `Shadechi^2(` `ShadeF(` `ShadeNorm(` `Shade_t(` | Stat | shade distribution tail |
| `Simul`        | Mode | simultaneous graphing |
| `sin(` `sin^-1(` `sinh(` `sinh^-1(` | Math | trig / hyperbolic |
| `SinReg`       | Stat | sinusoidal regression |
| `solve(`       | Math | numerical root |
| `SortA(` `SortD(` | Math | sort list ascending / descending |
| `startTmr` `checkTmr(` | Time | timer primitives |
| `statwizard-off` `statwizard-on` | Mode | stat-wizard UI gate |
| `stdDev(`      | Stat | standard deviation |
| `Stop`         | Ctrl | halt entire program chain |
| `StoreGDB`     | IO   | save graph state |
| `StorePic`     | IO   | save graph image |
| `String>Equ(`  | Var  | string to equation slot |
| `sub(`         | Math | substring; also `/100` shortcut |
| `sum(`         | Math | sum of list |
| `summation Sigma(` | Math | summation operator (`Sigma(...)`) |

## T

| token | category | notes |
|-------|----------|-------|
| `tan(` `tan^-1(` `tanh(` `tanh^-1(` | Math | trig / hyperbolic |
| `Tangent(`     | IO   | draw tangent line |
| `tcdf(` `tpdf(`| Stat | t-distribution |
| `Text(`        | IO   | graph-screen text |
| `TextColor(`   | IO   | text color `[CE]` |
| `Then`         | Ctrl | begin then-block |
| `Thick` `Thin` | Mode | graph stroke weight `[CE]` |
| `Time`         | Mode | sequence-vs-time |
| `timeCnv(`     | Time | seconds to {d,h,m,s} |
| `TInterval`    | Stat | t-interval |
| `toString(`    | Var  | number to string `[CE OS 5.2+]` |
| `Trace`        | IO   | invoke trace mode |
| `T-Test`       | Stat | one-sample t-test |
| `tvm_FV` `tvm_I%` `tvm_N` `tvm_Pmt` `tvm_PV` | Fin | TVM solvers |

## U

| token | category | notes |
|-------|----------|-------|
| `UnArchive`    | Mem  | move from Flash to RAM |
| `Un/d`         | Math | mixed number form |
| `uvAxes` `uwAxes` | Mode | sequence-mode phase plots |

## V

| token | category | notes |
|-------|----------|-------|
| `variance(`    | Stat | variance |
| `Vertical`     | IO   | full-height vertical line |
| `vwAxes`       | Mode | sequence-mode phase plot |

## W

| token | category | notes |
|-------|----------|-------|
| `Wait`         | Ctrl | sleep `[CE OS 5.2+]` |
| `Web`          | Mode | sequence-mode web plot |
| `While`        | Ctrl | pre-test loop |

## X

| token | category | notes |
|-------|----------|-------|
| `xor`          | Op   | logical XOR |
| `xyLine`       | Stat | plot type |

## Z

| token | category | notes |
|-------|----------|-------|
| `ZBox`         | Mode | zoom-to-box |
| `ZDecimal`     | Mode | zoom to decimal-friendly window |
| `ZFrac1/2` `ZFrac1/3` `ZFrac1/4` `ZFrac1/5` `ZFrac1/8` `ZFrac1/10` | Mode | zoom to fraction-friendly windows `[CE]` |
| `ZInteger`     | Mode | zoom to integer-friendly window |
| `ZInterval`    | Stat | z-interval |
| `Zoom In` `Zoom Out` | Mode | zoom |
| `ZoomFit`      | Mode | fit window to selected functions |
| `ZoomRcl` `ZoomSto` `ZPrevious` | Mode | zoom history |
| `ZQuadrant1`   | Mode | restrict to first quadrant `[CE]` |
| `ZSquare`      | Mode | aspect-ratio square window |
| `ZStandard`    | Mode | default window |
| `Z-Test(`      | Stat | z-test |
| `ZTrig`        | Mode | trig-friendly window |

---

This index is a name-and-category map. For semantics, follow the relevant section file:
- Math, calculus, probability, complex : [08-math-library.md](08-math-library.md).
- I/O : [06-io.md](06-io.md), [07-graphics.md](07-graphics.md).
- Variable / memory commands : [03-types-and-variables.md](03-types-and-variables.md).
- Control flow : [05-control-flow.md](05-control-flow.md).
- Errors : [09-errors.md](09-errors.md).
