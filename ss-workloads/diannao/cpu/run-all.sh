#!/bin/bash

for trial in ""; do
  for i in conv1p conv2p conv3p conv4p conv5p pool1p pool3p pool5p class1p class3p; do
    echo -n "$i $args "
    out=`$i $args`
    ticks=`echo $out | cut -d: -f2`
    echo $ticks
  done
done
