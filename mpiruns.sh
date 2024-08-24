#!/bin/bash

# Define parameter ranges or lists
N_list=(500 1000 2000 3000 4000 6000 8000)
policy_list=(1)
nworkers_list=(1 2 4 6 8 10 12 14 16)
tileSize_list=(16)

# Loop over all combinations of parameters
for N in "${N_list[@]}"; do
    for policy in "${policy_list[@]}"; do
        for nworkers in "${nworkers_list[@]}"; do
            for tileSize in "${tileSize_list[@]}"; do
                echo "Running with parameters: N=$N, nworkers=$nworkers, tileSize=$tileSize"
                mpirun -np $nworkers ./UTWBPWMPI $N $tileSize $policy
            done
        done
    done
done
