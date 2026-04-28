"""Make src/ importable on the host and stub out the MicroPython-only
`machine` module so wire.py's `from machine import Pin` works under CPython.

The wire bit-bang code itself is not exercised by these tests; they cover
the pure-data layers (packet framing, var headers, real/list codecs,
.8Xp parsing). Only the import side-effect of constructing two Pin
objects at module load needs to succeed.
"""

import sys
import types
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SRC = ROOT / "src"
TOOLS = ROOT / "tools"
for p in (SRC, TOOLS):
    sp = str(p)
    if sp not in sys.path:
        sys.path.insert(0, sp)

if "machine" not in sys.modules:
    machine = types.ModuleType("machine")

    class _Pin:
        IN = 0
        OUT = 1
        PULL_UP = 2

        def __init__(self, *args, **kwargs):
            pass

        def init(self, *args, **kwargs):
            pass

        def value(self, *args):
            return 1

    machine.Pin = _Pin
    sys.modules["machine"] = machine
