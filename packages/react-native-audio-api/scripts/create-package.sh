#!/bin/bash
set -euo pipefail

yarn install --immutable

if [ $# -ge 1 ] && [ "$1" = "generate_nightly_version" ]; then
    cp package.json package.json.bak
    trap 'mv package.json.bak package.json' EXIT

    VERSION=$(jq -r '.version' package.json)
    IFS='.' read -r MAJOR MINOR PATCH <<< "$VERSION"
    GIT_COMMIT=$(git rev-parse HEAD)
    DATE=$(date +%Y%m%d)
    NIGHTLY_UNIQUE_NAME="${GIT_COMMIT:0:7}-$DATE"
    if [[ "$OSTYPE" == "darwin"* ]]; then
        sed -i '' "3s/.*/  \"version\": \"$MAJOR.$MINOR.$PATCH-nightly-$NIGHTLY_UNIQUE_NAME\",/" package.json
    else
        sed -i "3s/.*/  \"version\": \"$MAJOR.$MINOR.$PATCH-nightly-$NIGHTLY_UNIQUE_NAME\",/" package.json
    fi
fi

yarn build

npm pack

echo "Done!"
