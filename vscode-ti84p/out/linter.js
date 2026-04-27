"use strict";
/**
 * Diagnostic checks for .ti84p source files.
 *
 * All checks operate on the plain-text source representation (ASCII transliteration).
 * The binary tokenization step is a separate concern.
 */
Object.defineProperty(exports, "__esModule", { value: true });
exports.lint = lint;
// Commands that only exist on 84+CSE / 84+CE. Warn when seen in a .ti84p file.
const CE_ONLY_COMMANDS = new Set([
    "BackgroundOn", "BackgroundOff", "BorderColor",
    "TextColor(", "toString(", "Wait",
    "piecewise(", "Asm84CEPrgm", "Asm84CPrgm",
]);
// Block-opening tokens and their expected closers.
const BLOCK_OPENERS = {
    "Then": "End",
    "For(": "End",
    "While": "End",
    "Repeat": "End",
    "Else": "End", // Else is mid-block but resets the stack level; tracked separately
};
const BLOCK_OPENER_RE = /\b(Then|For\(|While|Repeat)\b/g;
const BLOCK_END_RE = /\bEnd\b/g;
const ELSE_RE = /\bElse\b/g;
const IF_RE = /^[:\s]*If\b/;
// Match Lbl / Goto targets
const LBL_RE = /\bLbl\s+([A-Z][A-Z0-9]?)\b/g;
const GOTO_RE = /\bGoto\s+([A-Z][A-Z0-9]?)\b/g;
const MENU_RE = /Menu\([^)]*"([A-Z][A-Z0-9]?)"[^)]*\)/g;
// CE-only command detection
const CE_CMD_RE = new RegExp(`\\b(${[...CE_ONLY_COMMANDS].map((c) => c.replace(/[()]/g, "\\$&")).join("|")})\\b`, "g");
function lint(source) {
    const diagnostics = [];
    const lines = source.split(/\r?\n/);
    // --- Pass 1: collect all Lbl targets ---
    const definedLabels = new Set();
    for (const line of lines) {
        for (const m of line.matchAll(LBL_RE)) {
            definedLabels.add(m[1]);
        }
    }
    // --- Pass 2: per-line checks + block stack ---
    const stack = [];
    let prevLineIsIf = false;
    for (let i = 0; i < lines.length; i++) {
        const raw = lines[i];
        // Strip string contents so we don't match tokens inside strings.
        const stripped = stripStrings(raw);
        // CE-only commands
        for (const m of stripped.matchAll(CE_CMD_RE)) {
            diagnostics.push({
                line: i,
                col: m.index,
                endCol: m.index + m[0].length,
                message: `'${m[0]}' is 84+CE/CSE-only and will not run on TI-84 Plus / 84+SE.`,
                severity: "warning",
            });
        }
        // Block openers: Then / For( / While / Repeat
        for (const m of stripped.matchAll(BLOCK_OPENER_RE)) {
            stack.push({
                opener: m[1],
                line: i,
                hasIf: m[1] === "Then" && prevLineIsIf,
            });
        }
        // Else: must have a matching Then on the stack
        for (const m of stripped.matchAll(ELSE_RE)) {
            const top = stack[stack.length - 1];
            if (!top || top.opener !== "Then") {
                diagnostics.push({
                    line: i,
                    col: m.index,
                    endCol: m.index + m[0].length,
                    message: "`Else` without a matching `If/Then`.",
                    severity: "error",
                });
            }
            else {
                // Replace Then with Else so the End pairs with Else
                stack[stack.length - 1] = { opener: "Else", line: i, hasIf: top.hasIf };
            }
        }
        // End: pop the stack
        for (const m of stripped.matchAll(BLOCK_END_RE)) {
            if (stack.length === 0) {
                diagnostics.push({
                    line: i,
                    col: m.index,
                    endCol: m.index + m[0].length,
                    message: "`End` without a matching `For(`/`While`/`Repeat`/`If-Then`.",
                    severity: "error",
                });
            }
            else {
                stack.pop();
            }
        }
        // Goto -> undefined label
        for (const m of stripped.matchAll(GOTO_RE)) {
            if (!definedLabels.has(m[1])) {
                diagnostics.push({
                    line: i,
                    col: m.index,
                    endCol: m.index + m[0].length + m[1].length + 1,
                    message: `\`Goto ${m[1]}\` references undefined label \`Lbl ${m[1]}\`.`,
                    severity: "error",
                });
            }
        }
        // Goto inside a loop: memory leak warning
        if (stripped.includes("Goto")) {
            const inLoop = stack.some((f) => ["For(", "While", "Repeat"].includes(f.opener));
            if (inLoop) {
                const gotoMatch = /\bGoto\b/.exec(stripped);
                if (gotoMatch) {
                    diagnostics.push({
                        line: i,
                        col: gotoMatch.index,
                        endCol: gotoMatch.index + 4,
                        message: "`Goto` out of a loop skips `End` and leaks the loop's stack frame, " +
                            "consuming RAM permanently until the program exits. Consider restructuring.",
                        severity: "warning",
                    });
                }
            }
        }
        // Menu( -> check label refs
        for (const m of raw.matchAll(MENU_RE)) {
            // Extract all label names from the Menu call
            const menuBody = m[0];
            const labelRefs = [...menuBody.matchAll(/"([A-Z][A-Z0-9]?)"/g)].slice(1); // skip title
            // Actually extract properly: Menu("TITLE","OPT1",L1,"OPT2",L2,...)
            // Label args are the identifier args (non-string args after "TITLE")
            const menuArgRe = /,\s*([A-Z][A-Z0-9]?)\s*(?=[,)])/g;
            for (const la of menuBody.matchAll(menuArgRe)) {
                if (!definedLabels.has(la[1])) {
                    diagnostics.push({
                        line: i,
                        col: m.index + la.index,
                        endCol: m.index + la.index + la[0].length,
                        message: `\`Menu(\` references undefined label \`Lbl ${la[1]}\`.`,
                        severity: "error",
                    });
                }
            }
        }
        prevLineIsIf = IF_RE.test(stripped);
    }
    // Unclosed blocks at EOF
    for (const frame of stack) {
        diagnostics.push({
            line: frame.line,
            col: 0,
            endCol: lines[frame.line].length,
            message: `\`${frame.opener}\` block is never closed with \`End\`.`,
            severity: "error",
        });
    }
    return diagnostics;
}
/**
 * Replace the contents of string literals with spaces so token-matching
 * regexes don't fire on text inside strings.
 */
function stripStrings(line) {
    return line.replace(/"[^"]*"/g, (m) => " ".repeat(m.length));
}
//# sourceMappingURL=linter.js.map