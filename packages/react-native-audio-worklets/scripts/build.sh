#!/bin/bash

pnpm --filter react-native-audio-api build

bob build

echo '{"type": "module"}' > lib/module/package.json
