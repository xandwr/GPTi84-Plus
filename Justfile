set shell := ["bash", "-cu"]

PY      := ".venv/bin/python"
PYTEST  := ".venv/bin/pytest"
MPR     := "mpremote"
SRC     := "src"
SPASM   := "spasm"
SPASM_INC := "references/other_projects/spasm"

# nimble deployment targets. The relay runtime lives under the `agent`
# account; sudo (unit install, systemctl) goes through `xander`. Split
# matches the real separation: agent owns the service, xander is admin.
# Both names are ssh_config Host aliases so the right IdentityFile gets
# picked up automatically: `nimble` -> id_ed25519, `nimble-agent` ->
# id_ed25519_agent.
NIMBLE_AGENT := "nimble-agent"
NIMBLE_ADMIN := "nimble"
NIMBLE_REPO  := "/home/agent/ti84-superdeluxe"

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

# Assemble a Z80 source with spasm-ng to a sibling .8xp.
# Example: just asm programs/asm_hello/HELLO.z80
asm FILE:
    @echo "==> assembling {{FILE}}"
    {{SPASM}} -I {{SPASM_INC}} {{FILE}} {{ without_extension(FILE) }}.8xp

# Assemble a Z80 source then push the resulting .8Xp to the calc.
# Example: just push-asm programs/asm_hello/HELLO.z80
push-asm FILE:
    just asm {{FILE}}
    just push {{ without_extension(FILE) }}.8xp

# Tokenize a TI-BASIC source (.basic) into a .8Xp. NAME is the on-calc
# program name (1-8 chars, A-Z and 0-9). Defaults to the source's basename.
# Example: just basic programs/basic_deck/DECK.basic
basic FILE NAME="":
    @echo "==> tokenizing {{FILE}} as {{ if NAME == '' { uppercase(without_extension(file_name(FILE))) } else { NAME } }}"
    {{PY}} tools/bastok.py build \
        {{ if NAME == "" { uppercase(without_extension(file_name(FILE))) } else { NAME } }} \
        {{FILE}} {{ without_extension(FILE) }}.8xp

# Tokenize a BASIC source then push the resulting .8Xp to the calc.
# Example: just push-basic programs/basic_deck/DECK.basic
push-basic FILE NAME="":
    just basic {{FILE}} {{NAME}}
    just push {{ without_extension(FILE) }}.8xp

# Push a .8Xp program to the calc. Default is FLAPPY.
push FILE="programs/flappy_bird/FLAPPY.8xp":
    @echo "==> generating push script for {{FILE}}"
    {{PY}} tools/build_e2e.py push {{FILE}} > /tmp/ti84_e2e_push.py
    @echo "==> running on Pico"
    {{MPR}} run /tmp/ti84_e2e_push.py

# Listen on the Pico for a calc-initiated variable transfer. NAME filters
# by 8-byte var name (e.g. CHATMSG); TYPE filters by hex type byte (e.g. 15
# for AppVar). Both optional. Ctrl-C to stop.
# Example: just listen CHATMSG 15
listen NAME="" TYPE="":
    @echo "==> generating listen script (name={{NAME}} type={{TYPE}})"
    {{PY}} tools/build_e2e.py listen {{NAME}} {{TYPE}} > /tmp/ti84_e2e_listen.py
    @echo "==> running on Pico (Ctrl-C to stop)"
    {{MPR}} run /tmp/ti84_e2e_listen.py

# Push a .8Xp, then request it back and byte-compare on the Pico.
roundtrip FILE="programs/flappy_bird/FLAPPY.8xp":
    @echo "==> generating roundtrip script for {{FILE}}"
    {{PY}} tools/build_e2e.py roundtrip {{FILE}} > /tmp/ti84_e2e_rt.py
    @echo "==> running on Pico"
    {{MPR}} run /tmp/ti84_e2e_rt.py

# Run the desktop-side TCP relay. Reads length-prefixed frames from
# any connected client and prints them. Default port 9999.
relay PORT="9999":
    {{PY}} tools/relay_server.py --port {{PORT}}

# Run the relay in echo mode: every frame from the calc is auto-replied
# with "echo: <text>". v0 stub for the ChatGPT-on-calc loop.
relay-echo PORT="9999":
    {{PY}} tools/relay_server.py --port {{PORT}} --echo

# Run the relay in LLM mode: parses the bridge's prompt/math frame,
# calls Ollama (cloud or local) with a JSON-schema format, ASCII-clamps
# the reply to STR0_MAX_CHARS, ships it back framed. Reads OLLAMA_URL,
# OLLAMA_MODEL, OLLAMA_API_KEY from .env (gitignored) at the repo root.
# Local swap: OLLAMA_URL=http://127.0.0.1:11434/api/chat OLLAMA_MODEL=llama3.2 OLLAMA_API_KEY= just relay-llm
relay-llm PORT="9999":
    set -a; [ -f .env ] && . ./.env; set +a; \
    {{PY}} tools/relay_server.py --port {{PORT}} --llm

# Deploy the LLM-mode relay to nimble. Copies .env (gitignored, holds
# OLLAMA_API_KEY + CF service-token) into agent's repo, locks perms,
# then uses xander's sudo to install the unit, stop any old relay on
# :9999, and enable relay-llm. Idempotent. xander's sudo password
# (one prompt per run) is required on nimble.
deploy-relay-llm:
    @echo "==> shipping relay source to {{NIMBLE_AGENT}}:{{NIMBLE_REPO}}/tools/"
    scp tools/relay_server.py {{NIMBLE_AGENT}}:{{NIMBLE_REPO}}/tools/relay_server.py
    @echo "==> shipping .env to {{NIMBLE_AGENT}}:{{NIMBLE_REPO}}/.env"
    scp .env {{NIMBLE_AGENT}}:{{NIMBLE_REPO}}/.env
    ssh {{NIMBLE_AGENT}} 'chmod 600 {{NIMBLE_REPO}}/.env'
    @echo "==> shipping relay-llm.service via {{NIMBLE_ADMIN}}"
    scp deploy/systemd/relay-llm.service {{NIMBLE_ADMIN}}:/tmp/relay-llm.service
    ssh -t {{NIMBLE_ADMIN}} 'sudo /usr/bin/install -m 644 /tmp/relay-llm.service /etc/systemd/system/relay-llm.service \
        && sudo /usr/bin/systemctl daemon-reload \
        && (sudo /usr/bin/systemctl is-active --quiet relay && sudo /usr/bin/systemctl disable --now relay || true) \
        && sudo /usr/bin/systemctl reset-failed relay-llm \
        && sudo /usr/bin/systemctl enable relay-llm \
        && sudo /usr/bin/systemctl restart relay-llm \
        && sudo /usr/bin/systemctl restart wsbridge \
        && sleep 2 \
        && sudo /usr/bin/systemctl status --no-pager relay-llm | head -10 \
        && sudo /usr/bin/systemctl status --no-pager wsbridge   | head -10'

# Tail relay-llm logs from nimble. Read-only; no sudo needed.
relay-llm-logs:
    ssh -t {{NIMBLE_ADMIN}} 'journalctl -u relay-llm -f -n 100'

# Print the sudoers drop-in for the relay-llm deploy. Pipe into visudo
# on nimble to install once: `just sudoers-relay-deploy | ssh nimble \
#   'sudo tee /etc/sudoers.d/xander-relay-deploy.new >/dev/null && \
#    sudo visudo -cf /etc/sudoers.d/xander-relay-deploy.new && \
#    sudo install -m 440 /etc/sudoers.d/xander-relay-deploy.new \
#                          /etc/sudoers.d/xander-relay-deploy && \
#    sudo rm /etc/sudoers.d/xander-relay-deploy.new'`
# The `visudo -cf` validates syntax before commit so a typo can't lock
# you out of sudo. After install, deploy-relay-llm runs prompt-free.
sudoers-relay-deploy:
    @cat deploy/sudoers/xander-relay-deploy

# One-time bootstrap: install this laptop's pubkeys for both nimble
# accounts. xander gets the standard ssh-copy-id treatment. agent is a
# service account with no password (locked), so we can't ssh-copy-id
# into it directly: instead we pipe id_ed25519_agent.pub through
# xander's ssh and have xander's sudo append it to agent's
# authorized_keys (idempotent: grep -q before append).
#
# Prompts: xander's ssh password ONCE (until xander's pubkey is
# installed on the first line), then xander's sudo password ONCE for
# the agent half. After this, every deploy recipe runs prompt-free.
bootstrap-agent-ssh:
    @echo "==> installing xander pubkey on {{NIMBLE_ADMIN}}"
    ssh-copy-id -i ~/.ssh/id_ed25519.pub {{NIMBLE_ADMIN}}
    @echo "==> installing agent pubkey on agent@nimble (via xander's sudo)"
    cat ~/.ssh/id_ed25519_agent.pub | ssh -t {{NIMBLE_ADMIN}} \
        'KEY=$(cat); sudo install -d -m 700 -o agent -g agent /home/agent/.ssh \
            && sudo touch /home/agent/.ssh/authorized_keys \
            && sudo chown agent:agent /home/agent/.ssh/authorized_keys \
            && sudo chmod 600 /home/agent/.ssh/authorized_keys \
            && (sudo grep -qF "$KEY" /home/agent/.ssh/authorized_keys \
                || echo "$KEY" | sudo tee -a /home/agent/.ssh/authorized_keys >/dev/null) \
            && echo "agent authorized_keys now:" \
            && sudo wc -l /home/agent/.ssh/authorized_keys'
    @echo "==> verifying both"
    ssh -o BatchMode=yes -o ConnectTimeout=5 {{NIMBLE_ADMIN}} 'echo ok: $(whoami)@$(hostname)'
    ssh -o BatchMode=yes -o ConnectTimeout=5 {{NIMBLE_AGENT}} 'echo ok: $(whoami)@$(hostname)'

# Disable Pico autoboot of the bridge (renames main.py -> main.py.off).
# Use during dev when autoboot fights mpremote run / repl.
autoboot-off:
    {{MPR}} exec 'import os; os.rename("main.py", "main.py.off")'

# Re-enable Pico autoboot of the bridge.
autoboot-on:
    {{MPR}} exec 'import os; os.rename("main.py.off", "main.py")'

# Bring up the calc<->desktop chat bridge on the Pico. Connects to
# wifi (creds in src/secrets.py), opens a TCP socket to the desktop
# (host/port in secrets.py), and forwards calc-initiated AppVar/Program
# transfers as length-prefixed frames. Ctrl-C to stop.
# Example: just chat-bridge CHATMSG 15
chat-bridge NAME="" TYPE="":
    @echo "==> generating bridge script (name={{NAME}} type={{TYPE}})"
    {{PY}} tools/build_e2e.py bridge {{NAME}} {{TYPE}} > /tmp/ti84_e2e_bridge.py
    @echo "==> running on Pico (Ctrl-C to stop)"
    {{MPR}} run /tmp/ti84_e2e_bridge.py

# One-shot PC-master push of AppVar CHATIN to the calc with the given
# ASCII payload. Pair with programs/asm_pushtest/PUSHTEST.z80 (or any
# calc-side program polling for CHATIN via _ChkFindSym): the calc
# should render "got: PAYLOAD" on row 4 once the OS silent-link
# receive completes. Used to test the Option-A architecture (calc
# polls, PC pushes) vs the calc-master REQ path.
# Example: just pushvar "hello"
pushvar PAYLOAD="hello":
    @echo "==> generating pushvar script (payload={{PAYLOAD}})"
    {{PY}} tools/build_e2e.py pushvar "{{PAYLOAD}}" > /tmp/ti84_e2e_pushvar.py
    @echo "==> running on Pico"
    {{MPR}} run /tmp/ti84_e2e_pushvar.py

# Wire-only test for calc-master REQ. Runs listen_loop on the Pico with
# a hardcoded on_req that always serves PAYLOAD as AppVar CHATIN. Pair
# with `Asm(prgmREQTEST)` on the calc; expect the PAYLOAD text to appear
# on the calc screen on row 6 plus "12345" status markers in column 15.
# Example: just reqtest "hello there"
reqtest PAYLOAD="hello calc":
    @echo "==> generating reqtest script (payload={{PAYLOAD}})"
    {{PY}} tools/build_e2e.py reqtest "{{PAYLOAD}}" > /tmp/ti84_e2e_reqtest.py
    @echo "==> running on Pico (Ctrl-C to stop)"
    {{MPR}} run /tmp/ti84_e2e_reqtest.py

# Full e2e gate: host tests + push FLAPPY + roundtrip FLAPPY + roundtrip SEX.
test-e2e: test
    just push  programs/flappy_bird/FLAPPY.8xp
    just roundtrip programs/flappy_bird/FLAPPY.8xp
    just roundtrip programs/debug/SEX.8xp

# Calc must be powered, idle on the home screen, with the link awake
# (same precondition as `just push`). Rebuilds CHAT/DECK/VIEW from
# source, syncs the Pico bridge runtime, soft-resets the Pico, then
# pushes all three programs to the calc. One-shot chat deploy.
deploy:
    @echo "==> [1/4] rebuilding chat artifacts"
    just asm   programs/asm_chat/CHAT.z80
    just basic programs/basic_deck/DECK.basic
    just basic programs/basic_view/VIEW.basic
    @echo "==> [2/4] syncing src/ to Pico"
    just sync
    @echo "==> [3/4] resetting Pico"
    {{MPR}} reset
    @sleep 1
    @echo "==> [4/4] pushing CHAT + DECK + VIEW to calc"
    just push programs/asm_chat/CHAT.8xp
    just push programs/basic_deck/DECK.8xp
    just push programs/basic_view/VIEW.8xp
    @echo "==> restarting Pico bridge (calc pushes preempted it)"
    {{MPR}} reset
    @echo "==> deploy complete"
