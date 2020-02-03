#!/bin/bash

export OMP_NUM_THREADS=64  # number of threads used for the simulation

## for performance reasons, you might want to use some agressive scheduling ###
#export OMP_WAIT_POLICY=active
#export OMP_DYNAMIC=false
#export OMP_PROC_BIND=true

module load mitsuba

#simulation 
mkdir test/
./poff_no_graph -l mat_sparkle.conf -s columnd2.sc -e test/c -em  test_mitsuba/c -stop 20000 -es 10

#rendring
mkdir test_mitsuba/
./poff_no_graph -s columnd2.sc -i test/c -em test_mitsuba/m -es 2 -stop 2000
cd test_mitsuba/
mitsuba -p 10 m00[0123]*.xml &  # -p <number of cores used>
mitsuba -p 10 m00[456]*.xml &
mitsuba -p 10 m00[789]*.xml &
wait
#convert exr into png
ls -1 m000?.exr | xargs -n 1 bash -c 'convert "$0" -colorspace RGB -colorspace sRGB "${0%.exr}.png"' &
ls -1 m001?.exr | xargs -n 1 bash -c 'convert "$0" -colorspace RGB -colorspace sRGB "${0%.exr}.png"' &
ls -1 m002?.exr | xargs -n 1 bash -c 'convert "$0" -colorspace RGB -colorspace sRGB "${0%.exr}.png"' &
ls -1 m003?.exr | xargs -n 1 bash -c 'convert "$0" -colorspace RGB -colorspace sRGB "${0%.exr}.png"' &
ls -1 m004?.exr | xargs -n 1 bash -c 'convert "$0" -colorspace RGB -colorspace sRGB "${0%.exr}.png"' &
ls -1 m005?.exr | xargs -n 1 bash -c 'convert "$0" -colorspace RGB -colorspace sRGB "${0%.exr}.png"' &
ls -1 m006?.exr | xargs -n 1 bash -c 'convert "$0" -colorspace RGB -colorspace sRGB "${0%.exr}.png"' &
ls -1 m007?.exr | xargs -n 1 bash -c 'convert "$0" -colorspace RGB -colorspace sRGB "${0%.exr}.png"' &
ls -1 m008?.exr | xargs -n 1 bash -c 'convert "$0" -colorspace RGB -colorspace sRGB "${0%.exr}.png"' &
ls -1 m009?.exr | xargs -n 1 bash -c 'convert "$0" -colorspace RGB -colorspace sRGB "${0%.exr}.png"' &
wait
cd ..
