"""Host-side coverage for the relay's page-layout logic.

The relay is responsible for taking whatever shape the model returned
(structured {'lines': [...]} dicts under the new schema, or legacy
single-string entries) and producing fixed PAGE_ROWS*PAGE_COLS-char
page bodies. The calc-side BASIC pager paints each row with
Output(R, 1, sub(StrP, 1+(R-1)*PAGE_COLS, PAGE_COLS)), which assumes
every page is exactly that constant length: any deviation crashes the
pager with an ERR:DOMAIN. So we test the boundary aggressively here.
"""

import relay_server as relay


COLS = relay.PAGE_COLS  # 16
ROWS = relay.PAGE_ROWS  # 7
PCHARS = COLS * ROWS    # 112


# ---- _rewrap_line ---------------------------------------------------

def test_rewrap_short_line_passes_through():
    assert relay._rewrap_line("hi", COLS) == ["hi"]


def test_rewrap_empty_line_yields_single_empty():
    assert relay._rewrap_line("", COLS) == [""]


def test_rewrap_breaks_on_word_boundary():
    # 16-col limit: "the quick brown" is 15 chars, "fox" makes 19, must split.
    out = relay._rewrap_line("the quick brown fox jumps", COLS)
    assert all(len(s) <= COLS for s in out)
    assert " ".join(out) == "the quick brown fox jumps"


def test_rewrap_hard_splits_oversize_word():
    # 20-char identifier must be cut at 16, even though that breaks the word.
    out = relay._rewrap_line("supercalifragilisticexpialidocious", COLS)
    assert all(len(s) <= COLS for s in out)
    # Concatenating the chunks should give the original word back.
    assert "".join(out) == "supercalifragilisticexpialidocious"


def test_rewrap_preserves_word_when_exactly_cols():
    # Exactly-16-char word fits on its own line; should not be split.
    word = "X" * COLS
    assert relay._rewrap_line(word, COLS) == [word]


# ---- _layout_pages --------------------------------------------------

def _split_grid(grid):
    """Split a fixed-grid page back into its ROWS rows for assertions."""
    assert len(grid) == PCHARS, ("expected %d-char grid, got %d" %
                                 (PCHARS, len(grid)))
    return [grid[i * COLS:(i + 1) * COLS] for i in range(ROWS)]


def test_layout_single_page_pads_to_full_grid():
    pages = relay._layout_pages([{"lines": ["hello"]}])
    assert len(pages) == 1
    rows = _split_grid(pages[0])
    assert rows[0] == "hello".ljust(COLS)
    # All other rows are pure padding spaces.
    for r in rows[1:]:
        assert r == " " * COLS


def test_layout_respects_model_page_breaks():
    # Two pages, one line each. Output must be two grids, not one merged.
    pages = relay._layout_pages([
        {"lines": ["page one"]},
        {"lines": ["page two"]},
    ])
    assert len(pages) == 2
    assert _split_grid(pages[0])[0] == "page one".ljust(COLS)
    assert _split_grid(pages[1])[0] == "page two".ljust(COLS)


def test_layout_overflow_spills_into_new_page():
    # 10 lines on one page must spill: 7 on page 1, 3 on page 2.
    lines = ["L%d" % i for i in range(10)]
    pages = relay._layout_pages([{"lines": lines}])
    assert len(pages) == 2
    rows1 = _split_grid(pages[0])
    rows2 = _split_grid(pages[1])
    assert [r.rstrip() for r in rows1] == lines[:7]
    assert [r.rstrip() for r in rows2[:3]] == lines[7:]
    assert all(r == " " * COLS for r in rows2[3:])


def test_layout_rewraps_overlong_line_from_model():
    # Belt-and-braces: model violated the schema and emitted a 30-char
    # line. The relay must rewrap it, not ship it raw.
    long_line = "this line is far too long for one row"
    pages = relay._layout_pages([{"lines": [long_line]}])
    rows = _split_grid(pages[0])
    # First non-blank rows reconstruct the original when joined with spaces.
    used = [r.rstrip() for r in rows if r.strip()]
    assert " ".join(used) == long_line
    assert all(len(r.rstrip()) <= COLS for r in rows)


def test_layout_legacy_string_input_is_wrapped_and_paginated():
    # Pre-schema fallback shape: list of plain strings. The relay must
    # still produce fixed grids by splitting on embedded newlines and
    # word-wrapping each paragraph.
    text = "alpha beta gamma\ndelta epsilon zeta eta theta"
    pages = relay._layout_pages([text])
    assert len(pages) >= 1
    for p in pages:
        assert len(p) == PCHARS


def test_layout_caps_at_max_pages():
    # 100 single-char lines in one page request -> would fit ROWS per page,
    # producing ceil(100/ROWS) pages. _layout_pages must clamp to MAX_PAGES.
    lines = ["x"] * 100
    pages = relay._layout_pages([{"lines": lines}])
    assert len(pages) == relay.MAX_PAGES
    # Every page is still a full fixed grid.
    for p in pages:
        assert len(p) == PCHARS


def test_layout_empty_input_yields_one_blank_page():
    pages = relay._layout_pages([])
    assert len(pages) == 1
    assert pages[0] == " " * PCHARS


def test_layout_blank_lines_in_model_output_are_preserved():
    # Model emits a blank line between paragraphs to visually separate
    # them. That row should remain blank on the calc, not get collapsed.
    pages = relay._layout_pages([{"lines": ["alpha", "", "beta"]}])
    rows = _split_grid(pages[0])
    assert rows[0].rstrip() == "alpha"
    assert rows[1] == " " * COLS
    assert rows[2].rstrip() == "beta"


# ---- _llm_reply_pages framing --------------------------------------

def test_llm_reply_frame_shape_is_fixed_grids_separated_by_nul(monkeypatch):
    # Stub the model call so this test stays offline. _call_ollama
    # returning a list of {'lines':...} dicts is the new schema-compliant
    # shape.
    def fake_call(prompt, math):
        return [
            {"lines": ["page1 line1", "page1 line2"]},
            {"lines": ["page2 only"]},
        ]
    monkeypatch.setattr(relay, "_call_ollama", fake_call)

    frame = relay._llm_reply_pages("prompt:hi\nmath:\n")
    # Header.
    assert frame.startswith(b"pages:2\n")
    body = frame[len(b"pages:2\n"):]
    chunks = body.split(b"\x00")
    assert len(chunks) == 2
    # Each page body is exactly PCHARS bytes -- the whole point.
    for c in chunks:
        assert len(c) == PCHARS
    # First page row 1 is "page1 line1" left-padded right-spaced.
    assert chunks[0][:COLS].decode("ascii") == "page1 line1".ljust(COLS)


def test_llm_reply_frame_handles_call_failure_gracefully(monkeypatch):
    # Network/timeout/etc -> caller path catches, builds a one-page error
    # reply. The frame must still be well-formed: header + one PCHARS body.
    def boom(prompt, math):
        import urllib.error
        raise urllib.error.URLError("simulated")
    monkeypatch.setattr(relay, "_call_ollama", boom)

    frame = relay._llm_reply_pages("prompt:hi\nmath:\n")
    assert frame.startswith(b"pages:1\n")
    body = frame[len(b"pages:1\n"):]
    assert len(body) == PCHARS
    # The leading text of the page should mention "err:" so the user
    # sees what went wrong rather than a blank screen.
    assert body[:4] == b"err:"
