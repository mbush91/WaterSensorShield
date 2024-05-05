#!/bin/bash
set -euo pipefail
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
csv=$DIR/../cfg/scheduler_map.csv
if column -V >/dev/null; then
    sed 's/^ *//; s/ *,/,/g; s/, */,/g' "$csv" | column -t -s, -o, > "$csv.formatted"
    mv "$csv.formatted" "$csv"
else
    echo "please get a better column, brew install util-linux"
    exit 1
fi
