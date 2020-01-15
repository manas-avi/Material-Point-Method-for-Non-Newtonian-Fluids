#!/bin/bash
#
#$ -S /bin/bash
#$ -N omptest
#$ -pe openmp 16
#$ -l h_vmem=2G
make clean
qmake poff_no_graph.pro
make -j8
export OMP_NUM_THREADS=7  # gets number from -pe
### for performance reasons, you might want to use some agressive scheduling ###
export OMP_WAIT_POLICY=active
export OMP_DYNAMIC=false
export OMP_PROC_BIND=true

rm -r results/bouncy6
mkdir results/bouncy6
./poff_no_graph -l material/bouncy.conf -s scenes/falling_sphere.sc -e results/bouncy6/test -es 10 -stop 6000