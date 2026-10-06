#!/usr/bin/env bash
# Render the Markdown manual to PDF: pandoc to standalone HTML with the project stylesheet, then a
# headless Chromium print. Usage: render.sh <manual.md> <out.pdf> [chromium binary]
set -euo pipefail
src="$1"; out="$2"; chrome="${3:-chromium}"
tmp="$(mktemp -d)"
pandoc "$src" -s --embed-resources --resource-path="$(dirname "$src"):$(dirname "$0")" --metadata title="Jane-Sixty User Manual" -c style.css -o "$tmp/manual.html"
cp "$(dirname "$0")/style.css" "$tmp/style.css"
"$chrome" --headless --no-sandbox --disable-gpu --no-pdf-header-footer --print-to-pdf="$out" "file://$tmp/manual.html" 2>/dev/null
rm -rf "$tmp"
ls -la "$out"
