"""Pico W boot entry. Runs the calc<->desktop bridge as an appliance.

To temporarily disable autoboot during dev (e.g. you want a clean REPL):
  mpremote exec 'import os; os.rename("main.py", "main.py.off")'
and put it back with the inverse.
"""

import bridge

# Filter to the AppVar CHATMSG that asm_chat (CHAT.z80) sends. If you want
# the bridge to relay any AppVar/Program transfer, change to bridge.run().
bridge.run(name="CHATMSG", expected_type=0x15)
