#!/bin/bash

# Get all .cc and .h files in directory iex.
files=$(find iex -type f \( -iname \*.cc -o -iname \*.h \))

# Only print if errors are encountered.
cpplint --quiet $files
lint=$?

# Only print if errors are encountered and treat all nonconformities as errors.
clang-format -n -Werror $files
format=$?

# Return 1 if either code style test failed.
[[ $lint == 0 && $format == 0 ]]
