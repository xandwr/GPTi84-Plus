# 07 : Graphics

The graph screen has its own coordinate system, its own font, and its own command set. It coexists with the home screen but uses different commands and data types (pictures, GDBs).

## Graph-screen geometry

Monochrome (TI-83 / 83+ / 84+ / 84+SE):
- 96 columns by 64 rows of pixels.
- Pixel coordinates: `(0,0)` top-left to `(94, 62)` bottom-right after accounting for two reserved regions:
  - Right column reserved for the busy / pause indicator.
  - Bottom row reserved for the cursor / status overlays.
- Two coordinate systems coexist:
  - **Pixel** (`Pxl-On(row, col)`, `Text(row, col, ...)`) : integer pixel addresses, 0-indexed, row-then-column.
  - **Cartesian** (`Pt-On(x, y)`, `Line(x1, y1, x2, y2)`, `Circle(x, y, r)`) : floats, scaled by current window. `(x, y)` order, with the window's `Xmin`-`Xmax` and `Ymin`-`Ymax` setting the mapping.

84+CSE: 320x240 pixels, color (border around usable region; details in 84+CSE-specific commands).

84+CE: same screen as CSE, with additional commands (`Wait`, `toString(`, `BackgroundOff`, `BackgroundOn`).

Two fonts available on the graph screen:
- **Small font** (3x5, variable-width). Default for `Text(`. Allows much more text on screen than the home screen.
- **Large font** (5x7, identical to the home screen font). Used by passing `-1` as the first arg to `Text(`: `Text(-1, row, col, ...)`. 84+CSE / CE may differ.

## Window setup

Six numeric variables control the Cartesian-to-pixel mapping:

| variable | meaning |
|----------|---------|
| `Xmin`   | leftmost x-coordinate |
| `Xmax`   | rightmost x-coordinate |
| `Xscl`   | x-axis tick spacing (cosmetic only when axes off) |
| `Ymin`   | bottommost y-coordinate |
| `Ymax`   | topmost y-coordinate |
| `Yscl`   | y-axis tick spacing |
| `Xres`   | x-axis pixel sample resolution (graphing functions only) |
| `dX`     | shorthand: `(Xmax-Xmin)/94`. Settable; setting it adjusts `Xmax` accordingly. |
| `dY`     | shorthand: `(Ymax-Ymin)/62`. Same. |

Pixel-to-Cartesian map: `pixel(0, 0)` is `(Xmin, Ymax)`, `pixel(62, 94)` is `(Xmax, Ymin)`. Note pixel rows count downward but Cartesian y counts upward.

The standard "1 pixel per Cartesian unit" setup:
```
:0->Xmin
:1->dX     // sets Xmax = 0 + 94*1 = 94
:0->Ymin
:1->dY     // sets Ymax = 0 + 62*1 = 62
```
After this, `Pt-On(50, 30)` lights pixel column 50, pixel row 32 (because pixel rows count from the top).

`ZStandard` resets the window to defaults `(-10, 10, -10, 10)`. `ZSquare` adjusts so visual aspect is 1:1. Several `Z*` zoom commands manipulate the window in canned ways.

## Graph-screen commands

### Clear

`ClrDraw` clears all drawn graphics. Does not affect the function-graph (the `Y_n` plots redraw on the next graph).

### Format flags

Each is a single token; storing it sets the OS state.
- `AxesOn` / `AxesOff`
- `LabelOn` / `LabelOff`
- `GridOn` / `GridOff` (some 84+ OS variants offer `GridLine` / `GridDot`)
- `CoordOn` / `CoordOff`
- `Connected` / `Dot`
- `Sequential` / `Simul`
- `RectGC` / `PolarGC`
- `ExprOn` / `ExprOff`
- `Full` / `Horiz` / `G-T`

A typical "clean drawing canvas" preamble:
```
:StoreGDB 1
:ClrDraw
:GridOff
:PlotsOff
:AxesOff
:FnOff
:0->Xmin:1->dX
:0->Ymin:1->dY
```
And on exit, `:RecallGDB 1:DelVar GDB1`.

### `Text(`

Display text or values at pixel `(row, col)`:
```
:Text(row, col, expr1[, expr2, ...])
:Text(-1, row, col, ...)        // large font (TI-83+ and up)
```
- Pixel-row, pixel-col ordering (different from `Output(` on home screen, which is row-col but cell-based).
- `row` ranges 0-57 (small font, top-left), 0-50 ish (large font), `col` 0-91. `ERR:DOMAIN` past those.
- Each character is 3 px tall (small) or 5 px tall (large), variable width. Spacing between characters is automatic.
- `Text(` overwrites any pixels under the new text. **Caveat on 84+/SE**: small text can erase the row of pixels just below it (cosmetic on white background).

### Drawing in Cartesian (`Pt-`)

`Pt-On(x, y[, mark])`, `Pt-Off(x, y[, mark])`, `Pt-Change(x, y)`.
- `mark`: 1 (dot, default), 2 (3x3 box), 3 (3x3 cross). Use the same mark for `Pt-Off` as for the `Pt-On` that drew it.

### Drawing in pixels (`Pxl-`)

`Pxl-On(row, col)`, `Pxl-Off(row, col)`, `Pxl-Change(row, col)`, `pxl-Test(row, col)` (returns 1 if lit, else 0).
- Direct pixel addressing, ignores window settings.
- Row 0 is top.

### Lines and shapes

- `Line(x1, y1, x2, y2[, draw])` : segment in Cartesian. `draw` 1 = on (default), 0 = erase.
- `Horizontal y` : full-width horizontal line at `y` (Cartesian).
- `Vertical x` : full-height vertical line at `x`.
- `Circle(x, y, r[, color])` : circle in Cartesian. The `Xscl`/`Yscl` aspect ratio matters: in non-square windows, circles render as ellipses. `ZSquare` first if you want a circle.
  - **Trick**: pass `{i}` (a complex list) as the optional 4th arg (mono only) to use 8-fold symmetry, ~3x faster. Not on color models.
- `Tangent(expr, x)` : draws the tangent line to `expr` at `x`.
- `DrawF expr` : graphs an expression of `X` (or a `Y_n` slot).
- `DrawInv expr` : graphs the inverse (swaps X and Y).
- `Shade(lower, upper[, xleft, xright, pattern, patres])` : shades the region between two functions.

### Coordinate-system conversions

Programs sometimes need to translate between systems:
- `R>Pr(x, y)`, `R>Ptheta(x, y)` : rectangular to polar magnitude / angle.
- `P>Rx(r, theta)`, `P>Ry(r, theta)` : polar to rectangular x / y.

## Pictures

`Pic0`-`Pic9`. Save and restore the exact pixel state.

```
:StorePic 1     // save current graph screen to Pic1
:RecallPic 1    // restore Pic1 over the current screen (logical OR with existing pixels)
```

Caveats:
- `RecallPic` ORs into the current screen (does not clear first). To replace, `ClrDraw` first.
- Mono pictures are about 768 bytes. Color pictures are larger.
- `RecallPic` of a mono picture on a color calculator decompresses to color (background fill); the reverse direction is not supported.

## GDBs

`GDB0`-`GDB9`. Save the **graphing context** (window, format flags, equation list) but not the rendered image. Used to roundtrip user settings:
```
:StoreGDB 1
:...                // your program changes window etc.
:RecallGDB 1
:DelVar GDB1
```

Note: `GDB` does not save stat plots or the rendered pixels; pair with `StorePic` / `RecallPic` if you need those too.

## Graphing mode

`Func` / `Param` / `Polar` / `Seq` selects which slot family is active and how `Y_n` (or `r_n`, `X_nT`/`Y_nT`, `u`/`v`/`w`) is interpreted. Most TI-BASIC programs lock to `Func` and treat the equation slots as user-defined functions of `X`:

```
:"X^2"->Y1
:DrawF Y1                // graphs y = x^2
:Y1(3)->A                // evaluates Y1 at x=3, A = 9
```

`Y1`-`Y0` evaluated as functions of `X` is the only "user-defined function" facility in TI-BASIC.

## Performance notes

- Graph-screen commands are slower than home-screen output. Use the home screen for text-only UIs.
- `Pxl-` is faster than `Pt-` (no Cartesian -> pixel transform per call).
- `Line(` and `Circle(` are noticeably slower than equivalent `Pxl-` loops for small shapes. For large shapes, `Line` wins.
- Drawing many points: a `For(` over a list, calling `Pxl-On` per element, is fine; building a list and using broadcasted comparisons + `Pt-On` does not scale (no batch draw command).

## Running totals on color models

84+CE adds:
- `BackgroundOn n`, `BackgroundOff` : sets a color background.
- `TextColor(r, g, b)` or `TextColor(idx)` : changes default text color.
- Drawing commands accept an optional color arg.

These tokens are 2-byte and are absent on monochrome OS images : a program containing them is non-portable to mono calculators.
