"""Live debug console for the ti84-superdeluxe relay.

Listens on 127.0.0.1:8081. Cloudflared on the same host routes everything
except /ws and /ws/* to this process; ws_bridge keeps /ws. Cloudflare
Access gates the public hostname so only the GitHub-SSO'd human or the
Pico's service token reach us.

Endpoints:
  GET  /                : single-file HTML console (light/dark toggle, SSE tail)
  GET  /api/events      : recent events as JSON (?since=<id>&limit=<n>)
  GET  /api/stream      : SSE stream of new events
  POST /api/ingest      : ws_bridge writes one event per WS frame here
  GET  /api/healthz     : liveness probe

Storage: SQLite at $TI84_CONSOLE_DB (default /var/lib/ti84-console/events.db).
"""

import argparse
import asyncio
import json
import os
import sqlite3
import sys
import time
from contextlib import asynccontextmanager

from fastapi import FastAPI, HTTPException, Query, Request
from fastapi.responses import HTMLResponse, JSONResponse, StreamingResponse


DEFAULT_DB = "/var/lib/ti84-console/events.db"
SCHEMA = """
CREATE TABLE IF NOT EXISTS events (
    id           INTEGER PRIMARY KEY AUTOINCREMENT,
    ts_ms        INTEGER NOT NULL,
    direction    TEXT    NOT NULL,
    length       INTEGER NOT NULL,
    preview_hex  TEXT    NOT NULL,
    preview_ascii TEXT   NOT NULL,
    conn_id      TEXT    NOT NULL,
    note         TEXT
);
CREATE INDEX IF NOT EXISTS events_ts ON events(ts_ms);
"""


class Store:
    def __init__(self, path):
        self.path = path
        os.makedirs(os.path.dirname(path), exist_ok=True)
        self._conn = sqlite3.connect(path, check_same_thread=False, isolation_level=None)
        self._conn.execute("PRAGMA journal_mode=WAL")
        self._conn.execute("PRAGMA synchronous=NORMAL")
        self._conn.executescript(SCHEMA)
        self._lock = asyncio.Lock()
        self._waiters = set()

    async def insert(self, ev):
        async with self._lock:
            cur = self._conn.execute(
                "INSERT INTO events(ts_ms, direction, length, preview_hex, preview_ascii, conn_id, note) "
                "VALUES (?, ?, ?, ?, ?, ?, ?)",
                (ev["ts_ms"], ev["direction"], ev["length"], ev["preview_hex"],
                 ev["preview_ascii"], ev["conn_id"], ev.get("note")),
            )
            ev["id"] = cur.lastrowid
        for q in list(self._waiters):
            try:
                q.put_nowait(ev)
            except asyncio.QueueFull:
                pass
        return ev["id"]

    def recent(self, since_id, limit):
        cur = self._conn.execute(
            "SELECT id, ts_ms, direction, length, preview_hex, preview_ascii, conn_id, note "
            "FROM events WHERE id > ? ORDER BY id ASC LIMIT ?",
            (since_id, limit),
        )
        cols = [d[0] for d in cur.description]
        return [dict(zip(cols, row)) for row in cur.fetchall()]

    def subscribe(self):
        q = asyncio.Queue(maxsize=1024)
        self._waiters.add(q)
        return q

    def unsubscribe(self, q):
        self._waiters.discard(q)


@asynccontextmanager
async def lifespan(app):
    app.state.store = Store(app.state.db_path)
    yield


app = FastAPI(lifespan=lifespan)


@app.get("/api/healthz")
async def healthz():
    return {"ok": True, "ts_ms": int(time.time() * 1000)}


@app.post("/api/ingest")
async def ingest(req: Request):
    if req.client and req.client.host not in ("127.0.0.1", "::1"):
        raise HTTPException(status_code=403, detail="ingest is loopback-only")
    body = await req.json()
    required = ("ts_ms", "direction", "length", "preview_hex", "preview_ascii", "conn_id")
    if not all(k in body for k in required):
        raise HTTPException(status_code=400, detail="missing fields")
    if body["direction"] not in ("c2s", "s2c", "open", "close", "info"):
        raise HTTPException(status_code=400, detail="bad direction")
    ev_id = await req.app.state.store.insert(body)
    return {"id": ev_id}


@app.get("/api/events")
async def events(since: int = Query(0, ge=0), limit: int = Query(200, ge=1, le=2000)):
    rows = app.state.store.recent(since, limit)
    return JSONResponse(rows)


@app.get("/api/stream")
async def stream(request: Request):
    q = app.state.store.subscribe()

    async def gen():
        try:
            yield "retry: 2000\n\n"
            while True:
                if await request.is_disconnected():
                    return
                try:
                    ev = await asyncio.wait_for(q.get(), timeout=15.0)
                    yield "data: " + json.dumps(ev) + "\n\n"
                except asyncio.TimeoutError:
                    yield ": keepalive\n\n"
        finally:
            app.state.store.unsubscribe(q)

    return StreamingResponse(gen(), media_type="text/event-stream", headers={
        "Cache-Control": "no-cache",
        "X-Accel-Buffering": "no",
    })


INDEX_HTML = r"""<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<title>ti84-superdeluxe console</title>
<meta name="viewport" content="width=device-width,initial-scale=1">
<style>
  :root {
    --bg: #0e1116;
    --fg: #d6dbe1;
    --muted: #6b7480;
    --accent: #7cc4ff;
    --c2s: #84d18d;
    --s2c: #f0a868;
    --info: #c4a8ff;
    --row: #161b22;
    --border: #232a33;
  }
  :root.light {
    --bg: #f6f7f8;
    --fg: #1a1f24;
    --muted: #5a6470;
    --accent: #1a66c4;
    --c2s: #1f7a32;
    --s2c: #a55700;
    --info: #6a3fb0;
    --row: #ffffff;
    --border: #d8dde2;
  }
  * { box-sizing: border-box; }
  html, body { margin: 0; padding: 0; background: var(--bg); color: var(--fg);
    font-family: ui-monospace, SFMono-Regular, "JetBrains Mono", Menlo, Consolas, monospace;
    font-size: 13px; line-height: 1.45; }
  header { display: flex; align-items: center; gap: 12px; padding: 10px 14px;
    border-bottom: 1px solid var(--border); position: sticky; top: 0; background: var(--bg); z-index: 1; }
  header h1 { font-size: 13px; font-weight: 600; margin: 0; letter-spacing: 0.02em; }
  header .spacer { flex: 1; }
  header .status { color: var(--muted); }
  header .status.live { color: var(--c2s); }
  header .status.dead { color: var(--s2c); }
  button { background: transparent; border: 1px solid var(--border); color: var(--fg);
    font-family: inherit; font-size: 12px; padding: 4px 10px; border-radius: 4px; cursor: pointer; }
  button:hover { border-color: var(--accent); color: var(--accent); }
  main { padding: 8px 14px; }
  table { width: 100%; border-collapse: collapse; }
  th, td { text-align: left; padding: 4px 8px; border-bottom: 1px solid var(--border); vertical-align: top; }
  th { color: var(--muted); font-weight: 500; font-size: 11px; text-transform: uppercase; letter-spacing: 0.05em; position: sticky; top: 41px; background: var(--bg); }
  tbody tr { background: var(--row); }
  td.dir-c2s { color: var(--c2s); }
  td.dir-s2c { color: var(--s2c); }
  td.dir-info, td.dir-open, td.dir-close { color: var(--info); }
  td.preview { white-space: pre; overflow: hidden; text-overflow: ellipsis; max-width: 60ch; }
  td.ts, td.len, td.conn { color: var(--muted); }
  .empty { color: var(--muted); padding: 20px; text-align: center; }
</style>
</head>
<body>
<header>
  <h1>ti84-superdeluxe / relay console</h1>
  <span class="status" id="status">connecting...</span>
  <span class="spacer"></span>
  <button id="pause">pause</button>
  <button id="clear">clear</button>
  <button id="theme">light</button>
</header>
<main>
  <table id="tbl">
    <thead>
      <tr>
        <th style="width:90px">time</th>
        <th style="width:60px">dir</th>
        <th style="width:60px">len</th>
        <th style="width:120px">conn</th>
        <th>preview (ascii)</th>
        <th>hex</th>
      </tr>
    </thead>
    <tbody id="rows"></tbody>
  </table>
  <div class="empty" id="empty">no events yet : send something from the calc</div>
</main>
<script>
(() => {
  const rowsEl = document.getElementById("rows");
  const emptyEl = document.getElementById("empty");
  const statusEl = document.getElementById("status");
  const pauseBtn = document.getElementById("pause");
  const clearBtn = document.getElementById("clear");
  const themeBtn = document.getElementById("theme");

  let paused = false;
  let pending = [];
  const MAX_ROWS = 500;

  const fmtTs = (ms) => {
    const d = new Date(ms);
    const hh = String(d.getHours()).padStart(2, "0");
    const mm = String(d.getMinutes()).padStart(2, "0");
    const ss = String(d.getSeconds()).padStart(2, "0");
    const mss = String(d.getMilliseconds()).padStart(3, "0");
    return hh + ":" + mm + ":" + ss + "." + mss;
  };

  const append = (ev) => {
    if (paused) { pending.push(ev); return; }
    if (emptyEl) emptyEl.style.display = "none";
    const tr = document.createElement("tr");
    const dir = (ev.direction || "info");
    tr.innerHTML =
      '<td class="ts">' + fmtTs(ev.ts_ms) + '</td>' +
      '<td class="dir-' + dir + '">' + dir + '</td>' +
      '<td class="len">' + (ev.length ?? "") + '</td>' +
      '<td class="conn">' + (ev.conn_id || "") + '</td>' +
      '<td class="preview"></td>' +
      '<td class="preview"></td>';
    tr.children[4].textContent = ev.preview_ascii || ev.note || "";
    tr.children[5].textContent = ev.preview_hex || "";
    rowsEl.appendChild(tr);
    while (rowsEl.children.length > MAX_ROWS) rowsEl.removeChild(rowsEl.firstChild);
    window.scrollTo(0, document.body.scrollHeight);
  };

  pauseBtn.addEventListener("click", () => {
    paused = !paused;
    pauseBtn.textContent = paused ? "resume" : "pause";
    if (!paused && pending.length) {
      const flush = pending; pending = [];
      flush.forEach(append);
    }
  });

  clearBtn.addEventListener("click", () => {
    rowsEl.innerHTML = "";
    if (emptyEl) emptyEl.style.display = "";
  });

  const setTheme = (light) => {
    document.documentElement.classList.toggle("light", light);
    themeBtn.textContent = light ? "dark" : "light";
    try { localStorage.setItem("ti84-console-theme", light ? "light" : "dark"); } catch (e) {}
  };
  let stored = "dark";
  try { stored = localStorage.getItem("ti84-console-theme") || "dark"; } catch (e) {}
  setTheme(stored === "light");
  themeBtn.addEventListener("click", () => setTheme(!document.documentElement.classList.contains("light")));

  const setStatus = (txt, cls) => {
    statusEl.textContent = txt;
    statusEl.className = "status " + (cls || "");
  };

  const connect = async () => {
    try {
      const r = await fetch("/api/events?limit=200");
      const events = await r.json();
      events.forEach(append);
    } catch (e) { /* ignore, SSE will catch up */ }

    const es = new EventSource("/api/stream");
    es.onopen = () => setStatus("live", "live");
    es.onerror = () => setStatus("reconnecting...", "dead");
    es.onmessage = (m) => {
      try { append(JSON.parse(m.data)); } catch (e) {}
    };
  };
  connect();
})();
</script>
</body>
</html>
"""


@app.get("/", response_class=HTMLResponse)
async def index():
    return HTMLResponse(INDEX_HTML)


def main(argv=None):
    ap = argparse.ArgumentParser()
    ap.add_argument("--host", default="127.0.0.1")
    ap.add_argument("--port", type=int, default=8081)
    ap.add_argument("--db", default=os.environ.get("TI84_CONSOLE_DB", DEFAULT_DB))
    args = ap.parse_args(argv)

    import uvicorn
    app.state.db_path = args.db
    uvicorn.run(app, host=args.host, port=args.port, log_level="info", access_log=False)


if __name__ == "__main__":
    sys.exit(main())
