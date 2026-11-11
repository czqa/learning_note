#!/usr/bin/env bash
set -u

project_directory="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
node "$project_directory/build-project-data.mjs" >/dev/null 2>&1 || true
exec /usr/bin/xdg-open http://127.0.0.1:8765
