#!/bin/bash

yarn install --immutable

if [ $# -ge 1 ] && [ "$1" = "generate_nightly_version" ]; then
    VERSION=$(jq -r '.version' package.json)
    IFS='.' read -r MAJOR MINOR PATCH <<< "$VERSION" 
    GIT_COMMIT=$(git rev-parse HEAD)
    DATE=$(date +%Y%m%d)
    NIGHTLY_UNIQUE_NAME="${GIT_COMMIT:0:7}-$DATE"
    sed -i '' "3s/.*/  \"version\": \"$MAJOR.$MINOR.$PATCH-nightly-$NIGHTLY_UNIQUE_NAME\",/" package.json
fi

yarn bob build

npm pack

if [ $# -ge 1 ] && [ "$1" = "generate_nightly_version" ]; then
    sed -i '' "3s/.*/  \"version\": \"$MAJOR.$MINOR.$PATCH\",/" package.json
fi

echo "Done!"
