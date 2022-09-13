#!/bin/bash

if [ -z $1 ]; then
  repo=`git rev-parse --show-toplevel`
  find $repo \( -iname \*.cc -o -iname \*.h -o -iname \*.cpp -o -iname \*.hh \) | xargs -P`nproc` clang-format -i
else
  clang-format -i $1
fi
