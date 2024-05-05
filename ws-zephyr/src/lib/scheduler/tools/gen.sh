#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")"

if [[ ! -d .venv ]]; then
    python3 -m venv .venv
    .venv/bin/pip install -r requirements.txt
fi
.venv/bin/python SchedulerGen.py "$@"
