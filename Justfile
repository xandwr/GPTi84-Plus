set shell := ["bash", "-cu"]

PY      := ".venv/bin/python"
PYTEST  := ".venv/bin/pytest"
MPR     := "mpremote"
SRC     := "src"

# default: list recipes
default:
    @just --list

# --- host-only ---

# Run host tests (packet framing, var headers, real codec, .8Xp parser).
test:
    {{PYTEST}} tests/ -q

# Extract the variable payload from a .8Xp to stdout (binary).
extract FILE:
    {{PY}} tools/extract_8xp.py {{FILE}}

# --- pico / mpremote ---

# Drop into the Pico REPL.
repl:
    {{MPR}} repl

# Soft-reset the Pico.
reset:
    {{MPR}} reset

# List files on the Pico filesystem.
ls:
    {{MPR}} ls

# Copy src/*.py to the Pico filesystem (one-shot deploy).
sync:
    @for f in {{SRC}}/*.py; do echo "cp $f :"; {{MPR}} cp "$f" :; done

# Run a local .py file on the Pico without installing it (one-shot).
run FILE:
    {{MPR}} run {{FILE}}

# One-shot Python eval on the Pico. Quote the expression.
# Example: just exec 'import dbus; dbus.idle()'
exec EXPR:
    {{MPR}} exec "{{EXPR}}"

# --- e2e (calc must be plugged in and idle at the home screen) ---

# Push a .8Xp program to the calc. Default is FLAPPY.
push FILE="programs/flappy_bird/FLAPPY.8xp":
    @echo "==> generating push script for {{FILE}}"
    {{PY}} tools/build_e2e.py push {{FILE}} > /tmp/ti84_e2e_push.py
    @echo "==> running on Pico"
    {{MPR}} run /tmp/ti84_e2e_push.py

# Push a .8Xp, then request it back and byte-compare on the Pico.
roundtrip FILE="programs/flappy_bird/FLAPPY.8xp":
    @echo "==> generating roundtrip script for {{FILE}}"
    {{PY}} tools/build_e2e.py roundtrip {{FILE}} > /tmp/ti84_e2e_rt.py
    @echo "==> running on Pico"
    {{MPR}} run /tmp/ti84_e2e_rt.py

# Full e2e gate: host tests + push FLAPPY + roundtrip FLAPPY + roundtrip SEX.
test-e2e: test
    just push  programs/flappy_bird/FLAPPY.8xp
    just roundtrip programs/flappy_bird/FLAPPY.8xp
    just roundtrip programs/debug/SEX.8xp
