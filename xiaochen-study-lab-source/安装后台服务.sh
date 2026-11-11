#!/usr/bin/env bash
set -euo pipefail

project_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
config_base="${XDG_CONFIG_HOME:-${HOME}/.config}"
service_dir="$config_base/systemd/user"

install -d -m 700 "$service_dir"
install -m 644 "$project_dir/xiaochen-study.service" "$service_dir/xiaochen-study.service"
systemctl --user daemon-reload
systemctl --user enable xiaochen-study.service
systemctl --user restart xiaochen-study.service
systemctl --user --no-pager --full status xiaochen-study.service
