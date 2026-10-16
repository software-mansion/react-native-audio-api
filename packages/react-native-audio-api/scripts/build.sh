#!/bin/bash
set -euo pipefail

bob build

echo '{"type": "module"}' > lib/module/package.json
