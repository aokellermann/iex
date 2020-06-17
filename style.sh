#!/bin/bash

# Get all .cc and .h files in directory iex.
files=$(find iex iex/api -maxdepth 1 -type f \( -iname \*.cc -o -iname \*.h \))

compile_commands=$(find -type f -name compile_commands.json | head -n 1)
if [ -z $compile_commands ]; then
  echo "compile_commands.json file not found (run cmake to generate)"
  exit 1
fi

# Only print if errors are encountered.
clang-tidy -p $compile_commands --quiet $files
lint=$?

# Only print if errors are encountered and treat all nonconformities as errors.
clang-format -n -Werror $files
format=$?

# Return 1 if either code style test failed.
[[ $lint == 0 && $format == 0 ]]
