#!/bin/bash

if [ $# -ne 2 ]; then
    echo "Usage: ./waitOutput <file> <pattern regex> "
    exit 1
fi

file_input="$1"
pattern="$2"
found=0
grep_output=""

while [ $found -eq 0 ]
do
  #Search for pattern in output
  grep_output=$(cat "$file_input" | grep -E "$pattern")
  if [ "$grep_output" != "" ]; then
    found=1
  fi
done
