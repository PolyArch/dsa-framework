#/bin/bash
neato -Goverlap=false -Gstart=self -Gepsilon=.0000001 -Tsvg -o $1.svg $1
eog $1.svg
