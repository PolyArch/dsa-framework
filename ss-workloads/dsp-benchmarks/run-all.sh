#!/bin/bash

declare -a arr=("cholesky" "qr" "svd" "centro-fir" "gemm" "fft" "solver")

for i in "${arr[@]}"
do
	echo $i
	cd ./$i
	./run.py
	cd ..
done
