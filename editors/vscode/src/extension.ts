import * as vscode from "vscode";
import * as path from "path";
import * as fs from "fs";

// ---------------------------------------------------------------------------
// WASM module types
// ---------------------------------------------------------------------------

interface TGFSyntaxHighlighterModule {
  syntax_highlighter: { new (grammarSource: string): TGFSyntaxHighlighter };
  get_token_types(): { size(): number; get(i: number): string };
  get_token_modifiers(): { size(): number; get(i: number): string };
}

interface TGFSyntaxHighlighter {
  good(): boolean;
  diagnostics(): string;
  get_tokens(source: string): { size(): number; get(i: number): number; delete(): void };
  get_nt_name(id: number): string;
  get_nt_type(id: number): string;
  nt_count(): number;
  delete(): void;
}

// ---------------------------------------------------------------------------
// Grammar cache
// ---------------------------------------------------------------------------

interface CachedGrammar {
  hl: TGFSyntaxHighlighter;
  tokenTypes: string[];
  tokenModifiers: string[];
}

interface GrammarEntry {
  path: string;
  also?: string[];
}

type GrammarsConfig = Record<string, GrammarEntry>;

const grammarCache = new Map<string, CachedGrammar>();
let statusBar: vscode.StatusBarItem;
let output: vscode.OutputChannel;

// ---------------------------------------------------------------------------
// Activation
// ---------------------------------------------------------------------------

export async function activate(context: vscode.ExtensionContext) {
  output = vscode.window.createOutputChannel("TGF Syntax Highlighter");
  context.subscriptions.push(output);
  output.appendLine("=== activate ===");

  statusBar = vscode.window.createStatusBarItem(
    vscode.StatusBarAlignment.Left, 1000);
  statusBar.name = "TGF Syntax Highlighter";
  statusBar.text = "$(sync~spin) TGF: loading...";
  statusBar.show();
  context.subscriptions.push(statusBar);

  try {
    await doActivate(context);
  } catch (e) {
    output.appendLine("FATAL: " + e);
    statusBar.text = "$(error) TGF: activation failed";
    statusBar.tooltip = String(e);
    vscode.window.showErrorMessage("TGF Syntax Highlighter activation failed: " + e);
  }
}

async function doActivate(context: vscode.ExtensionContext) {
  // Load WASM (bust require cache safely).
  const wasmPath = path.join(context.extensionPath, "wasm", "syntax_highlighter.js");
  output.appendLine("wasm path: " + wasmPath);
  try { delete require.cache[require.resolve(wasmPath)]; } catch (_) {}
  output.appendLine("requiring wasm...");
  const factory = require(wasmPath);
  output.appendLine("wasm required, awaiting factory...");
  const HL: TGFSyntaxHighlighterModule = await factory();
  output.appendLine("wasm factory returned");

  // Build legend.
  const rawTypes = HL.get_token_types();
  const rawMods = HL.get_token_modifiers();
  const tokenTypes: string[] = [];
  const tokenModifiers: string[] = [];
  for (let i = 0; i < rawTypes.size(); i++) tokenTypes.push(rawTypes.get(i));
  for (let i = 0; i < rawMods.size(); i++) tokenModifiers.push(rawMods.get(i));
  output.appendLine("legend: " + tokenTypes.join(", "));

  const legend = new vscode.SemanticTokensLegend(tokenTypes, tokenModifiers);

  // grammars.json lists the language ids and their "also" aliases.
  const grammarsJsonPath = path.join(context.extensionPath, "grammars.json");
  let grammarsConfig: GrammarsConfig = {};
  try {
    grammarsConfig = JSON.parse(fs.readFileSync(grammarsJsonPath, "utf8"));
  } catch (e) {
    output.appendLine(`FATAL: could not read/parse grammars.json at ${grammarsJsonPath}: ${e}`);
    vscode.window.showErrorMessage(
      `TGF Syntax Highlighter: could not read grammars.json: ${e}`);
  }

  let loaded = 0;
  for (const [langId, entry] of Object.entries(grammarsConfig)) {
    const grammarPath = path.join(context.extensionPath, "grammars", `${langId}.tgf`);
    let src: string;
    try {
      src = fs.readFileSync(grammarPath, "utf8");
    } catch (e) {
      output.appendLine(`${langId}: could not read grammar file ${grammarPath}: ${e}`);
      continue;
    }
    let hl: TGFSyntaxHighlighter;
    try {
      output.appendLine(`compiling ${langId} from ${grammarPath} (${src.length} bytes)...`);
      hl = new HL.syntax_highlighter(src);
    } catch (e) {
      output.appendLine(`${langId}: EXCEPTION compiling ${grammarPath}: ${e}`);
      continue;
    }
    if (!hl.good()) {
      output.appendLine(`${langId}: FAILED to compile ${grammarPath} - ${hl.diagnostics()}`);
      vscode.window.showErrorMessage(
        `TGF Syntax Highlighter: failed to compile grammar for ${langId}. See output panel.`);
      hl.delete();
      continue;
    }

    const cached: CachedGrammar = { hl, tokenTypes, tokenModifiers };
    grammarCache.set(langId, cached);
    context.subscriptions.push(
      vscode.languages.registerDocumentSemanticTokensProvider(
        { language: langId },
        new TGFTokenProvider(cached, legend, output),
        legend
      )
    );
    output.appendLine(`${langId}: OK, provider registered`);
    loaded++;

    for (const alsoId of entry.also ?? []) {
      grammarCache.set(alsoId, cached);
      context.subscriptions.push(
        vscode.languages.registerDocumentSemanticTokensProvider(
          { language: alsoId },
          new TGFTokenProvider(cached, legend, output),
          legend
        )
      );
      output.appendLine(`${langId}: also registered for ${alsoId}`);
    }
  }

  statusBar.text = loaded > 0
    ? `$(check) TGF: ${loaded} grammar(s)`
    : `$(error) TGF: no grammars loaded`;
  output.appendLine(`activated, ${loaded} grammars loaded`);

  // Debug command: show all tokens for current document.
  context.subscriptions.push(
    vscode.commands.registerCommand("tgf-syntax.showTokens", async () => {
      const editor = vscode.window.activeTextEditor;
      if (!editor) return;
      const cached = grammarCache.get(editor.document.languageId);
      if (!cached) {
        vscode.window.showInformationMessage("No grammar loaded");
        return;
      }
      const text = editor.document.getText();
      const raw = cached.hl.get_tokens(text);
      const count = raw.size();
      output.show();
      output.appendLine(`=== all tokens (${count / 5}) ===`);
      let line = 0, col = 0;
      for (let i = 0; i < count; i += 5) {
        const dl = raw.get(i), dc = raw.get(i + 1);
        const len = raw.get(i + 2), ty = raw.get(i + 3);
        if (dl === 0) col += dc; else { line += dl; col = dc; }
        const tname = ty < cached.tokenTypes.length
          ? cached.tokenTypes[ty] : "?";
        const t = text.substring(
          offsetForLineCol(text, line, col),
          offsetForLineCol(text, line, col) + len);
        output.appendLine(`  L${line+1}:${col+1} [${tname}] ${JSON.stringify(t)}`);
      }
      raw.delete();
    })
  );
  context.subscriptions.push(
    vscode.commands.registerCommand("tgf-syntax.inspectTokens", async () => {
      const editor = vscode.window.activeTextEditor;
      if (!editor) return;
      const langId = editor.document.languageId;
      const cached = grammarCache.get(langId);
      if (!cached) {
        vscode.window.showInformationMessage(`No grammar loaded for ${langId}`);
        return;
      }
      const text = editor.document.getText();
      const raw = cached.hl.get_tokens(text);
      const count = raw.size();
      // Find tokens covering the cursor.
      const pos = editor.selection.active;
      const offset = editor.document.offsetAt(pos);
      let line = 0, col = 0, byteOffset = 0;
      const matches: string[] = [];
      for (let i = 0; i < count; i += 5) {
        const dl = raw.get(i), dc = raw.get(i + 1);
        const len = raw.get(i + 2), ty = raw.get(i + 3);
        if (dl === 0) col += dc; else { line += dl; col = dc; }
        // Compute byte offset for this token.
        const tokenOffset = offsetForLineCol(text, line, col);
        if (tokenOffset <= offset && offset < tokenOffset + len) {
          const tname = ty < cached.tokenTypes.length
            ? cached.tokenTypes[ty] : "?";
          const tokenText = text.substring(tokenOffset, tokenOffset + len);
          matches.push(`[${tname}] "${tokenText}" @ L${line+1}:${col+1}`);
        }
      }
      raw.delete();
      if (matches.length > 0) {
        output.appendLine("inspect: " + matches.join("; "));
        vscode.window.showInformationMessage(
          `Tokens at cursor: ${matches.join(", ")}`);
      } else {
        vscode.window.showInformationMessage(
          `No token at cursor (L${pos.line+1}:${pos.character+1})`);
      }
    })
  );

  // Debug command: show classification map for loaded languages.
  context.subscriptions.push(
    vscode.commands.registerCommand("tgf-syntax.showMap", async () => {
      output.show();
      const seen = new Set<object>();
      for (const [langId, cached] of grammarCache) {
        if (seen.has(cached)) continue;
        seen.add(cached);
        output.appendLine(`=== ${langId} ===`);
        let printed = 0;
        const ntCount = cached.hl.nt_count();
        for (let i = 0; i < ntCount && printed < 200; i++) {
          const name = cached.hl.get_nt_name(i);
          if (!name) continue;
          // Skip internal/transparent nonterminals.
          if (name.startsWith("__E_") || name.startsWith("__B_")
            || name.startsWith("__N_") || name === "_"
            || name === "__" || name === "ws")
            continue;
          const type = cached.hl.get_nt_type(i);
          output.appendLine(`  nt[${i}] "${name}" -> ${type}`);
          printed++;
        }
        output.appendLine(`  (${printed} nonterminals)`);
      }
    })
  );
}

function offsetForLineCol(text: string, line: number, col: number): number {
  let offset = 0;
  for (let l = 0; l < line; l++) {
    const nl = text.indexOf("\n", offset);
    if (nl < 0) return text.length;
    offset = nl + 1;
  }
  return offset + col;
}

export function deactivate() {
  const seen = new Set<CachedGrammar>();
  for (const cached of grammarCache.values()) {
    if (seen.has(cached)) continue;
    seen.add(cached);
    cached.hl.delete();
  }
  grammarCache.clear();
}

// ---------------------------------------------------------------------------
// Token provider
// ---------------------------------------------------------------------------

class TGFTokenProvider implements vscode.DocumentSemanticTokensProvider {
  private debounceTimer: ReturnType<typeof setTimeout> | null = null;
  private pendingResolve:
    ((v: vscode.SemanticTokens | null) => void) | null = null;
  private debounceMs = 200;

  constructor(
    private cached: CachedGrammar,
    private legend: vscode.SemanticTokensLegend,
    private output: vscode.OutputChannel
  ) {}

  async provideDocumentSemanticTokens(
    document: vscode.TextDocument,
    token: vscode.CancellationToken
  ): Promise<vscode.SemanticTokens | null> {
    // Supersede any in-flight debounced request so it resolves to null.
    if (this.debounceTimer) {
      clearTimeout(this.debounceTimer);
      this.debounceTimer = null;
      if (this.pendingResolve) { this.pendingResolve(null); }
      this.pendingResolve = null;
    }

    return new Promise((resolve) => {
      this.pendingResolve = resolve;
      const timer = setTimeout(() => {
        if (this.debounceTimer !== timer) return;
        this.debounceTimer = null;
        this.pendingResolve = null;
        resolve(token.isCancellationRequested
          ? null : this.computeTokens(document));
      }, this.debounceMs);
      this.debounceTimer = timer;

      token.onCancellationRequested(() => {
        if (this.debounceTimer !== timer) return;
        clearTimeout(timer);
        this.debounceTimer = null;
        this.pendingResolve = null;
        resolve(null);
      });
    });
  }

  private computeTokens(
    document: vscode.TextDocument
  ): vscode.SemanticTokens | null {
    try {
      const text = document.getText();
      this.output.appendLine(
        `computeTokens: ${document.languageId} ${text.length} bytes`);
      const raw = this.cached.hl.get_tokens(text);
      const count = raw.size();
      this.output.appendLine(`  -> ${count / 5} tokens`);

      if (count === 0) {
        raw.delete();
        return null;
      }

      const data = new Uint32Array(count);
      for (let i = 0; i < count; i++) data[i] = raw.get(i);
      raw.delete();

      return new vscode.SemanticTokens(data, undefined);
    } catch (e) {
      this.output.appendLine(`  ERROR: ${e}`);
      return null;
    }
  }
}
