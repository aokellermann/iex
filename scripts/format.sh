#!/bin/bash

files=$(find lib include tests -type f \( -iname \*.cc -o -iname \*.h \))

if [[ $1 == "check" ]]; then
  # Dry run
  clang-format -n -Werror $files
else
  # In place format
  clang-format -i $files
fi
