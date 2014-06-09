#!/bin/bash

iter_start=1
iter_end=5

for seed in $(seq $iter_start $iter_end)
do
	for thre in 200 800 1400 2000
	do
		for sta in 1 2 3 5 10 20 50
		do
		echo "====================================================================="
		echo "  iteration = $seed      FragThreshold = $thre        nSTA= $sta       "
		echo "====================================================================="
		./waf --run "scratch/randompac -Seed=$seed -nSTA=$sta -FragThre=$thre -simulFlag=1"

		done
		done
		done
