#!/bin/bash

files=$(find iex -type f \( -iname \*.cc -o -iname \*.h \))

cpplint --quiet $files
lint=$?

clang-format -n -Werror $files
format=$?

[[ $lint == 0 && $format == 0 ]]