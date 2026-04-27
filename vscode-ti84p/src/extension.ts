import * as vscode from "vscode";
import { lint } from "./linter";

// ---------------------------------------------------------------------------
// Completion items: built from the command index
// ---------------------------------------------------------------------------

const COMPLETIONS: Array<[string, vscode.CompletionItemKind, string]> = [
  // Control flow
  ["If", vscode.CompletionItemKind.Keyword, "If condition\ncommand"],
  ["Then", vscode.CompletionItemKind.Keyword, "If condition\nThen\n  \nEnd"],
  ["Else", vscode.CompletionItemKind.Keyword, "Else"],
  ["End", vscode.CompletionItemKind.Keyword, "End"],
  ["For(", vscode.CompletionItemKind.Keyword, "For(var, start, end[, step])\n  \nEnd"],
  ["While", vscode.CompletionItemKind.Keyword, "While condition\n  \nEnd"],
  ["Repeat", vscode.CompletionItemKind.Keyword, "Repeat condition\n  \nEnd"],
  ["Lbl", vscode.CompletionItemKind.Keyword, "Lbl NAME"],
  ["Goto", vscode.CompletionItemKind.Keyword, "Goto NAME"],
  ["Return", vscode.CompletionItemKind.Keyword, "Return"],
  ["Stop", vscode.CompletionItemKind.Keyword, "Stop"],
  ["IS>(", vscode.CompletionItemKind.Keyword, "IS>(var, value)"],
  ["DS<(", vscode.CompletionItemKind.Keyword, "DS<(var, value)"],
  ["Menu(", vscode.CompletionItemKind.Keyword, 'Menu("TITLE","OPT1",L1,"OPT2",L2)'],
  ["prgm", vscode.CompletionItemKind.Keyword, "prgmNAME"],
  ["Pause", vscode.CompletionItemKind.Keyword, "Pause [value]"],

  // I/O
  ["Disp", vscode.CompletionItemKind.Function, "Disp value"],
  ["Input", vscode.CompletionItemKind.Function, 'Input "PROMPT", variable'],
  ["Prompt", vscode.CompletionItemKind.Function, "Prompt A, B"],
  ["Output(", vscode.CompletionItemKind.Function, "Output(row, col, value)"],
  ["ClrHome", vscode.CompletionItemKind.Function, "ClrHome"],
  ["getKey", vscode.CompletionItemKind.Function, "getKey"],
  ["DispGraph", vscode.CompletionItemKind.Function, "DispGraph"],
  ["DispTable", vscode.CompletionItemKind.Function, "DispTable"],

  // Graphics
  ["ClrDraw", vscode.CompletionItemKind.Function, "ClrDraw"],
  ["Line(", vscode.CompletionItemKind.Function, "Line(x1,y1,x2,y2[,draw])"],
  ["Circle(", vscode.CompletionItemKind.Function, "Circle(x,y,r)"],
  ["Text(", vscode.CompletionItemKind.Function, "Text(row,col,value)"],
  ["Pt-On(", vscode.CompletionItemKind.Function, "Pt-On(x,y[,mark])"],
  ["Pt-Off(", vscode.CompletionItemKind.Function, "Pt-Off(x,y[,mark])"],
  ["Pt-Change(", vscode.CompletionItemKind.Function, "Pt-Change(x,y)"],
  ["Pxl-On(", vscode.CompletionItemKind.Function, "Pxl-On(row,col)"],
  ["Pxl-Off(", vscode.CompletionItemKind.Function, "Pxl-Off(row,col)"],
  ["Pxl-Change(", vscode.CompletionItemKind.Function, "Pxl-Change(row,col)"],
  ["pxl-Test(", vscode.CompletionItemKind.Function, "pxl-Test(row,col)"],
  ["StorePic", vscode.CompletionItemKind.Function, "StorePic Pic1"],
  ["RecallPic", vscode.CompletionItemKind.Function, "RecallPic Pic1"],
  ["StoreGDB", vscode.CompletionItemKind.Function, "StoreGDB GDB1"],
  ["RecallGDB", vscode.CompletionItemKind.Function, "RecallGDB GDB1"],
  ["DrawF", vscode.CompletionItemKind.Function, "DrawF expr"],
  ["DrawInv", vscode.CompletionItemKind.Function, "DrawInv expr"],
  ["Shade(", vscode.CompletionItemKind.Function, "Shade(lower,upper)"],
  ["Horizontal", vscode.CompletionItemKind.Function, "Horizontal y"],
  ["Vertical", vscode.CompletionItemKind.Function, "Vertical x"],
  ["ZStandard", vscode.CompletionItemKind.Function, "ZStandard"],
  ["ZSquare", vscode.CompletionItemKind.Function, "ZSquare"],
  ["AxesOn", vscode.CompletionItemKind.Function, "AxesOn"],
  ["AxesOff", vscode.CompletionItemKind.Function, "AxesOff"],
  ["GridOn", vscode.CompletionItemKind.Function, "GridOn"],
  ["GridOff", vscode.CompletionItemKind.Function, "GridOff"],
  ["PlotsOff", vscode.CompletionItemKind.Function, "PlotsOff"],
  ["FnOff", vscode.CompletionItemKind.Function, "FnOff"],

  // Math
  ["abs(", vscode.CompletionItemKind.Function, "abs(value)"],
  ["round(", vscode.CompletionItemKind.Function, "round(value[,digits])"],
  ["iPart(", vscode.CompletionItemKind.Function, "iPart(value)"],
  ["fPart(", vscode.CompletionItemKind.Function, "fPart(value)"],
  ["int(", vscode.CompletionItemKind.Function, "int(value)"],
  ["min(", vscode.CompletionItemKind.Function, "min(a,b)"],
  ["max(", vscode.CompletionItemKind.Function, "max(a,b)"],
  ["sqrt(", vscode.CompletionItemKind.Function, "sqrt(value)"],
  ["ln(", vscode.CompletionItemKind.Function, "ln(value)"],
  ["log(", vscode.CompletionItemKind.Function, "log(value)"],
  ["sin(", vscode.CompletionItemKind.Function, "sin(angle)"],
  ["cos(", vscode.CompletionItemKind.Function, "cos(angle)"],
  ["tan(", vscode.CompletionItemKind.Function, "tan(angle)"],
  ["nDeriv(", vscode.CompletionItemKind.Function, "nDeriv(expr,var,value)"],
  ["fnInt(", vscode.CompletionItemKind.Function, "fnInt(expr,var,lo,hi)"],
  ["solve(", vscode.CompletionItemKind.Function, "solve(expr,var,guess)"],
  ["seq(", vscode.CompletionItemKind.Function, "seq(expr,var,lo,hi[,step])"],
  ["sum(", vscode.CompletionItemKind.Function, "sum(list[,lo,hi])"],
  ["prod(", vscode.CompletionItemKind.Function, "prod(list[,lo,hi])"],
  ["dim(", vscode.CompletionItemKind.Function, "dim(list or matrix)"],
  ["augment(", vscode.CompletionItemKind.Function, "augment(L1,L2)"],
  ["cumSum(", vscode.CompletionItemKind.Function, "cumSum(list)"],
  ["Fill(", vscode.CompletionItemKind.Function, "Fill(value,list)"],
  ["sortA(", vscode.CompletionItemKind.Function, "sortA(list)"],
  ["sortD(", vscode.CompletionItemKind.Function, "sortD(list)"],
  ["randInt(", vscode.CompletionItemKind.Function, "randInt(lo,hi[,n])"],
  ["randNorm(", vscode.CompletionItemKind.Function, "randNorm(mean,stddev[,n])"],
  ["expr(", vscode.CompletionItemKind.Function, "expr(string)"],
  ["inString(", vscode.CompletionItemKind.Function, "inString(str,substr[,start])"],
  ["length(", vscode.CompletionItemKind.Function, "length(str)"],
  ["sub(", vscode.CompletionItemKind.Function, "sub(str,start,len)"],
  ["det(", vscode.CompletionItemKind.Function, "det([A])"],
  ["identity(", vscode.CompletionItemKind.Function, "identity(n)"],

  // Memory
  ["DelVar", vscode.CompletionItemKind.Keyword, "DelVar variable"],
  ["Archive", vscode.CompletionItemKind.Keyword, "Archive variable"],
  ["UnArchive", vscode.CompletionItemKind.Keyword, "UnArchive variable"],
  ["GarbageCollect", vscode.CompletionItemKind.Keyword, "GarbageCollect"],
  ["ClrList", vscode.CompletionItemKind.Function, "ClrList L1"],
  ["ClrAllLists", vscode.CompletionItemKind.Function, "ClrAllLists"],
  ["SetUpEditor", vscode.CompletionItemKind.Function, "SetUpEditor L.NAME"],

  // Constants
  ["pi", vscode.CompletionItemKind.Constant, "pi (3.14159...)"],
  ["e", vscode.CompletionItemKind.Constant, "e (2.71828...)"],
  ["i", vscode.CompletionItemKind.Constant, "i (imaginary unit)"],
  ["theta", vscode.CompletionItemKind.Variable, "theta variable"],
  ["Ans", vscode.CompletionItemKind.Variable, "Last computed value"],

  // Built-in lists
  ["L1", vscode.CompletionItemKind.Variable, "Built-in list 1"],
  ["L2", vscode.CompletionItemKind.Variable, "Built-in list 2"],
  ["L3", vscode.CompletionItemKind.Variable, "Built-in list 3"],
  ["L4", vscode.CompletionItemKind.Variable, "Built-in list 4"],
  ["L5", vscode.CompletionItemKind.Variable, "Built-in list 5"],
  ["L6", vscode.CompletionItemKind.Variable, "Built-in list 6"],

  // Strings
  ["Str0", vscode.CompletionItemKind.Variable, "String variable 0"],
  ["Str1", vscode.CompletionItemKind.Variable, "String variable 1"],
  ["Str2", vscode.CompletionItemKind.Variable, "String variable 2"],
  ["Str3", vscode.CompletionItemKind.Variable, "String variable 3"],
  ["Str4", vscode.CompletionItemKind.Variable, "String variable 4"],
  ["Str5", vscode.CompletionItemKind.Variable, "String variable 5"],
  ["Str6", vscode.CompletionItemKind.Variable, "String variable 6"],
  ["Str7", vscode.CompletionItemKind.Variable, "String variable 7"],
  ["Str8", vscode.CompletionItemKind.Variable, "String variable 8"],
  ["Str9", vscode.CompletionItemKind.Variable, "String variable 9"],

  // Operators
  ["->", vscode.CompletionItemKind.Operator, "Store operator (STO>)"],
  ["and", vscode.CompletionItemKind.Operator, "Logical AND"],
  ["or", vscode.CompletionItemKind.Operator, "Logical OR"],
  ["xor", vscode.CompletionItemKind.Operator, "Logical XOR"],
  ["not(", vscode.CompletionItemKind.Operator, "not(value)"],
];

// ---------------------------------------------------------------------------
// Extension activation
// ---------------------------------------------------------------------------

export function activate(context: vscode.ExtensionContext): void {
  const collection = vscode.languages.createDiagnosticCollection("ti84p");
  context.subscriptions.push(collection);

  // Run linter on open + change
  if (vscode.window.activeTextEditor) {
    runLinter(vscode.window.activeTextEditor.document, collection);
  }

  context.subscriptions.push(
    vscode.window.onDidChangeActiveTextEditor((editor) => {
      if (editor) runLinter(editor.document, collection);
    }),
    vscode.workspace.onDidChangeTextDocument((e) => {
      runLinter(e.document, collection);
    }),
    vscode.workspace.onDidCloseTextDocument((doc) => {
      collection.delete(doc.uri);
    })
  );

  // Completion provider
  const completionProvider = vscode.languages.registerCompletionItemProvider(
    { language: "ti84p" },
    {
      provideCompletionItems(
        document: vscode.TextDocument,
        position: vscode.Position
      ): vscode.CompletionItem[] {
        return COMPLETIONS.map(([label, kind, detail]) => {
          const item = new vscode.CompletionItem(label, kind);
          item.detail = detail;
          return item;
        });
      },
    }
  );
  context.subscriptions.push(completionProvider);
}

export function deactivate(): void {}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

function runLinter(
  document: vscode.TextDocument,
  collection: vscode.DiagnosticCollection
): void {
  if (document.languageId !== "ti84p") return;

  const diags = lint(document.getText());
  collection.set(
    document.uri,
    diags.map((d) => {
      const range = new vscode.Range(
        new vscode.Position(d.line, d.col),
        new vscode.Position(d.line, d.endCol)
      );
      const severity =
        d.severity === "error"
          ? vscode.DiagnosticSeverity.Error
          : d.severity === "warning"
          ? vscode.DiagnosticSeverity.Warning
          : vscode.DiagnosticSeverity.Information;
      return new vscode.Diagnostic(range, d.message, severity);
    })
  );
}
