#!/bin/bash

yarn install --immutable
yarn bob build

npm pack

echo "Done!"
