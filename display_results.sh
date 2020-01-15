#!/bin/bash
#
#$ -S /bin/bash
#$ -N omptest
#$ -pe openmp 16
#$ -l h_vmem=2G
make clean
qmake poff.pro
make -j8
export OMP_NUM_THREADS=7  # gets number from -pe
### for performance reasons, you might want to use some agressive scheduling ###
export OMP_WAIT_POLICY=active
export OMP_DYNAMIC=false
export OMP_PROC_BIND=true

./poff -l material/bouncy.conf -s scenes/falling_sphere.sc -i results/bouncy4/test -r 