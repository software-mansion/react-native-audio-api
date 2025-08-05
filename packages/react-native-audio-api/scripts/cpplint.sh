#!/bin/bash

if which cpplint >/dev/null; then
  find common/cpp android/src/main/cpp -path 'common/cpp/audioapi/libs'  -prune -o -path 'common/cpp/audioapi/external' -prune -o -name '*.cpp' -o -name '*.h' -print | xargs cpplint --linelength=230 --filter=-legal/copyright,-readability/todo,-build/namespaces,-whitespace/comments,-whitespace/parens,-build/c++17,-runtime/references --quiet
else
  echo "error: cpplint not installed, download from https://github.com/cpplint/cpplint" 1>&2
  exit 1
fi
