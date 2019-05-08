#!/bin/bash

cur_dir=`pwd`
#for i in ./gemm/blocked ./md/knn ./spmv/crs ./spmv/ellpack ./stencil/stencil2d ./stencil/stencil3d; do
for i in ./gemm/blocked ./md/knn; do
echo $i
cd $i
./sched.py
cd $cur_dir
done

