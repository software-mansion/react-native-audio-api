#!/bin/bash

if which cpplint >/dev/null; then
  find common/cpp android/src/main/cpp -path 'common/cpp/audioapi/libs'  -prune -o -path 'common/cpp/audioapi/external' -prune -o -name '*.cpp' -o -name '*.h' -print | xargs cpplint --linelength=80 --filter=-legal/copyright,-readability/todo,-build/namespaces,-build/include_order,-whitespace,-build/c++20,-runtime/references,-runtime/string --quiet --recursive "$@"
else
  echo "error: cpplint not installed, download from https://github.com/cpplint/cpplint" 1>&2
  exit 1
fi
