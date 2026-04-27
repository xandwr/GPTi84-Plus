# 02 : Lexical structure

TI-BASIC source is a sequence of tokens, not characters. The on-calc program editor produces the byte stream directly; host-side tools that ingest text source must tokenize before storing.

## Token sizes

Tokens are either 1 or 2 bytes:

- **Most common operators, variables, and built-in commands**: 1 byte.
- **Less common commands and the entire two-byte prefix space (statistical vars, window/finance vars, graph format flags, plus the post-83+ additions)**: 2 bytes, with a fixed first-byte prefix and a variable second byte.

The two-byte families are organized by prefix, listed in the on-calc OS:
- User variable names (single letters as variables: counted as 2-byte tokens for the named-variable form vs. 1-byte for the single-letter ALPHA-keypress form).
- Statistical variables (`x`, `y`, `Sx`, `Sy`, `r`, ...).
- Window and finance variables (`Xmin`, `Tmin`, `N`, `I%`, ...).
- Graph format flags (`AxesOff`, `GridOn`, `ExprOff`, ...).
- Miscellaneous (the late additions: `inString(`, `length(`, ...).
- Color-model-only tokens (84+CSE / 84+CE additions: `BackgroundOn`, `TextColor(`, ...).

Programs using mostly 1-byte tokens are denser than text source of the same expression. A line of pure lowercase ASCII text inside a string is the worst case: each lowercase letter is a 2-byte token.

## Why this matters

- **Program size budgets.** Memory accounting is per-token. The optimization guidance in [10-portability.md](10-portability.md) ("replace lowercase string fragments with command tokens") is about exploiting 1-byte tokens.
- **Source-level editors that operate on text** (TI Connect, TokenIDE, the `tilp` toolchain) must round-trip through a token table to produce byte-identical `.8xp` output. Text representations may use the unicode glyph (the calc-canonical form) or an ASCII transliteration; both must map back to the same opcode.

## Character set

TI-BASIC has three distinct "character" populations:

1. **Tokens that print as a glyph but represent a command or operator.** `->`, `>=`, `pi`, `i`, `theta`, `>Frac`, `>Polar`, `^`, etc.
2. **Letter / digit / punctuation tokens that compose expressions and identifiers.** Uppercase A-Z, digits 0-9, `(`, `)`, `,`, `.`, `+`, `-`, `*`, `/`, plus the alpha-2 set: lowercase a-z (post-83+ only).
3. **String content.** Whatever appears between `"..."` (see [03-types-and-variables.md](03-types-and-variables.md)). String content is itself stored as tokens; arbitrary characters cannot appear in a string except via specific tricks (see Strings).

### Extra-character tiers (matters for portability)

The "extra characters" landed in three waves and do not exist on older calculators:

- **Tier 1 (TI-83+ baseline)**: lowercase letters, Greek letters, international characters.
- **Tier 2 (OS 1.15)**: `~ @ # $ & ; \ | _ %`.
- **Tier 3 (OS 1.16)**: `... angle ss x T <- -> up-arrow down-arrow integral sqrt` plus subscripts 0-10.

Using a Tier 2 or Tier 3 character makes the program incompatible with calculators below that OS line. See [10-portability.md](10-portability.md).

## Line structure

A program is a list of lines. Each line is either:

- A single statement (`Disp "HI"`).
- Several statements separated by `:` (`ClrHome:Disp "HI"`).
- A label line (`Lbl A`).
- An empty line (legal; no effect).

The on-calc editor renders line breaks as line wraps; the byte stream uses a line-break token. The leading `:` shown in many references is a visual prompt, not part of the syntax.

### Statement separator: `:`

Two statements on the same line, separated by `:`, are equivalent to two lines. The compiler does not care. Programmers favor multi-statement lines mostly for size: a separating newline byte vs. a `:` byte are typically the same cost, but on-calc the colon form fits more on a screen line. There is one practical difference: an `If` without `Then` only governs the **next** statement, and that next statement may follow either after `:` or on the next line.

```
:If A=1:Disp "ONE"
:If A=1
:Disp "ONE"
```
These are equivalent.

## Identifiers and names

Identifier rules vary by variable kind (full table in [03-types-and-variables.md](03-types-and-variables.md)):

- **Real / complex**: single letter `A`-`Z` or `theta`. 27 fixed names. Cannot be created or destroyed; always present, default to 0.
- **Built-in lists**: `L1`-`L6`. Six fixed names.
- **User lists**: 1 to 5 characters from `A`-`Z`, `0`-`9`, `theta`. First character must be `A`-`Z` or `theta`. Up to 5 chars.
- **Matrix**: `[A]`-`[J]`. Ten fixed names.
- **String**: `Str0`-`Str9`. Ten fixed names.
- **Picture**: `Pic0`-`Pic9`. Ten fixed names.
- **GDB**: `GDB0`-`GDB9`. Ten fixed names.
- **Program**: 1 to 8 characters from `A`-`Z`, `0`-`9`, `theta`. First character must be `A`-`Z` or `theta`.
- **AppVar**: 1 to 8 characters, similar rule (created by Asm or Flash apps; not directly creatable in TI-BASIC).

The lowercase letters are not legal in identifier names. They appear only in string contents.

## Comments

There is no comment syntax. Three patterns are used in practice:

1. **Strings on their own line.** A bare `"comment"` line stores the string into `Ans` and is otherwise inert. Cost: the string tokens. This is the conventional "documentation" idiom.
2. **`If 0:` prefix.** A statement guarded by `If 0` is never executed. Cost: 2 tokens plus the body. Heavier than a string; useful for blocks.
3. **`Lbl` for tagging.** `Lbl ZA` marks a region for human readers. Cheap (2 tokens) but pollutes the label namespace.

None of these are inert at parse time : all are real statements that the interpreter executes. There is no zero-cost comment.

## Whitespace

Spaces are tokens. Each space within a string costs a byte. Outside strings, spaces are not used : the tokenized form has no need for them, and inserting them wastes memory. The on-calc editor sometimes inserts cosmetic spaces (e.g., between `Lbl` and its argument) which are also tokens. Newline is the line break token; inside a single line, statements separate with `:` (a token).

## Quoted strings and the trailing-quote rule

Strings open with `"` and close with `"`, but **the trailing `"` is optional before the store operator and at end-of-line.** The interpreter auto-closes the open string. This is not just an editor convenience : it is a memory optimization the language exposes. Identical idiom for closing parens (see [04-expressions.md](04-expressions.md)). An open string at end of program is closed at the program-end boundary.

```
:"Hello->Str1
```
parses as `"Hello"->Str1`.

## Encoding round-trip note for tooling

Host tools that produce `.8xp` from text source must handle:

- Unicode-to-token mapping for tokens with glyphs (`->`, `>=`, etc.).
- Auto-close of trailing `"` and `)` (matching on-calc behavior, or rejecting unclosed forms).
- Two-byte token prefixes (the second byte is not interchangeable across prefixes).
- Editor-canonical spacing (some tokens always emit with a leading space; matching that is required for byte-identical output).

For a machine-readable token table, the upstream pointer is the TI Toolkit token sheets referenced by tibasicdev. We do not duplicate the table here; if the firmware-side tooling needs it, vendor or pin the token sheet rather than transcribing.
