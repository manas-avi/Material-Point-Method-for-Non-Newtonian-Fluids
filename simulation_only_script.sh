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

rm -r cuboid_snow
mkdir cuboid_snow
# ./poff_no_graph -l material/snow0.conf -s scenes/falling_cube_cylinder0.sc -e test_snow3/test -es 10 -stop 10000
./poff_no_graph -l material/snow0.conf -s scenes/column0.sc -e cuboid_snow/test -es 10 -stop 4000