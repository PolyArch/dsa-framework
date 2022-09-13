#/bin/bash
fdp -T svg -O $1
eog $1.svg
